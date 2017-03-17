/* LoRa MatchX protocol */

#include <stdint.h>
#include <stdio.h>
#include "lmic/lmic.h"
#include "ble-suota.h"
#include "sensor.h"

#define DEBUG

#define PORT		0x01

#define CMD_MASK	0xf0
#define CMD_SHIFT	4
#define LEN_MASK	0x0f

#define LONG_LEN_MASK	0x3f

#define SENSOR_PERIOD	sec2osticks(10)

typedef enum {
	INFO_SENSOR_DATA	= 0x00,
	INFO_CMD_RESPONSE	= 0x10,
} uplink_info;

/* CMD_REBOOT_UPGRADE */
#define UPGRADE_BIT		0x80
#define REBOOT_TIMEOUT_MASK	0x07

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

#define STATUS_TX_PENDING	0x01
#define STATUS_BLE_ON		0x02
static uint8_t	status;

static const ostime_t	reboot_timeouts[] = {
	0,
	sec2osticks(5 * 60),
	sec2osticks(15 * 60),
	sec2osticks(30 * 60),
	sec2osticks(60 * 60),
	sec2osticks(2 * 60 * 60),
	sec2osticks(4 * 60 * 60),
	/* TBD */
};

static uint8_t	pend_tx_data[MAX_LEN_PAYLOAD];
static uint8_t	pend_tx_len;

static void
do_reboot(osjob_t *job)
{
	static uint8_t	cnt;

	/* If SUOTA is active, delay reboot, but force it after
	 * 256 * 5s (21 min 20 sec). */
	if (is_suota_ongoing() && --cnt) {
		os_setTimedCallback(job, hal_ticks() + sec2osticks(5),
		    do_reboot);
		return;
	}
	hw_cpm_reboot_system();
}

static void
ble_on(void)
{
	static OS_TASK	ble_handle;

	if (status & STATUS_BLE_ON)
		return;
	OS_TASK_CREATE("BLE & SUOTA", ble_task_func, (void *)0,
	    1024, OS_TASK_PRIORITY_NORMAL + 1, ble_handle);
	status |= STATUS_BLE_ON;
}

static void
handle_get_params(uint8_t *data, uint8_t len)
{
}

static void
handle_set_params(uint8_t *data, uint8_t len)
{
}

static void
handle_reboot_upgrade(uint8_t *data, uint8_t len)
{
	static osjob_t	reboot_job;
	uint8_t		tmout;

	switch (len) {
	case 0:
		os_setCallback(&reboot_job, do_reboot);
		break;
	case 1:
		tmout = data[0] & REBOOT_TIMEOUT_MASK;
		if (tmout >= ARRAY_SIZE(reboot_timeouts))
			break; // XXX
		os_setTimedCallback(&reboot_job,
		    hal_ticks() + reboot_timeouts[tmout], do_reboot);
		if (data[0] & UPGRADE_BIT)
			ble_on();
		break;
	default:
		break;
	}
}

typedef enum {
	CMD_GET_PARAMS		= 0x0,
	CMD_SET_PARAMS		= 0x1,
	CMD_REBOOT_UPGRADE	= 0x2,
} downlink_cmd;

static void	(* const downlink_handlers[])(uint8_t *, uint8_t) = {
	[CMD_GET_PARAMS]	= handle_get_params,
	[CMD_SET_PARAMS]	= handle_set_params,
	[CMD_REBOOT_UPGRADE]	= handle_reboot_upgrade,
};

static void
proto_handle(uint8_t port, uint8_t *data, uint8_t len)
{
	uint8_t	cmd, plen;

#ifdef DEBUG
	printf("rx:");
	for (int i = 0; i < len; i++)
		printf(" %02x", data[i]);
	printf("\r\n");
#endif
	if (port != PORT)
		return;
	while (len > 0) {
		cmd = *data++;
		len--;
		plen = cmd & LEN_MASK;
		if (plen == 0x0f) {
			if (!len)
				break;
			plen = *data++ & LONG_LEN_MASK;
			len--;
		}
		if (plen > len)
			break;
		cmd >>= CMD_SHIFT;
		if (cmd < ARRAY_SIZE(downlink_handlers) &&
		    downlink_handlers[cmd]) {
			(*downlink_handlers[cmd])(data, plen);
		}
		data += plen;
		len -= plen;
	}
}

static void
proto_send_sensor_data(osjob_t *job)
{
	size_t	len;

	os_setTimedCallback(job, hal_ticks() + SENSOR_PERIOD,
	    proto_send_sensor_data);
	if (ARRAY_SIZE(pend_tx_data) - pend_tx_len < 2)
		return;
	len = sensor_get_data(pend_tx_data + pend_tx_len + 1,
	    MIN(ARRAY_SIZE(pend_tx_data) - pend_tx_len - 1, LEN_MASK));
	if (len == 0)
		return;
	pend_tx_data[pend_tx_len] = INFO_SENSOR_DATA | (len & LEN_MASK);
	pend_tx_len += len + 1;
#ifdef DEBUG
	printf("set tx data:");
	for (int i = 0; i < pend_tx_len; i++)
		printf(" %02x", pend_tx_data[i]);
	printf("\r\n");
#endif
	LMIC_setTxData2(PORT, pend_tx_data, pend_tx_len, 0);
	status |= STATUS_TX_PENDING;
}

#ifdef DEBUG
static void
debug_event(int ev)
{
	static const char* evnames[] = {
		[EV_SCAN_TIMEOUT]   = "SCAN_TIMEOUT",
		[EV_BEACON_FOUND]   = "BEACON_FOUND",
		[EV_BEACON_MISSED]  = "BEACON_MISSED",
		[EV_BEACON_TRACKED] = "BEACON_TRACKED",
		[EV_JOINING]        = "JOINING",
		[EV_JOINED]         = "JOINED",
		[EV_RFU1]           = "RFU1",
		[EV_JOIN_FAILED]    = "JOIN_FAILED",
		[EV_REJOIN_FAILED]  = "REJOIN_FAILED",
		[EV_TXCOMPLETE]     = "TXCOMPLETE",
		[EV_LOST_TSYNC]     = "LOST_TSYNC",
		[EV_RESET]          = "RESET",
		[EV_RXCOMPLETE]     = "RXCOMPLETE",
		[EV_LINK_DEAD]      = "LINK_DEAD",
		[EV_LINK_ALIVE]     = "LINK_ALIVE",
		[EV_SCAN_FOUND]     = "SCAN_FOUND",
		[EV_TXSTART]        = "EV_TXSTART",
	};
	printf("%s\r\n", (ev < sizeof(evnames)/sizeof(evnames[0])) ? evnames[ev] : "EV_UNKNOWN" );
}
#else
#define debug_event(ev)
#endif

void
onEvent(ev_t ev)
{
	static osjob_t	sensor_job;

	debug_event(ev);
	switch(ev) {
	case EV_JOINED:
		printf("netid = %lu\r\n", LMIC.netid);
		proto_send_sensor_data(&sensor_job);
		break;
	case EV_TXSTART:
		status &= ~STATUS_TX_PENDING;
		pend_tx_len = 0;
		break;
	case EV_TXCOMPLETE:
		if (LMIC.dataLen != 0) {
			proto_handle(LMIC.frame[LMIC.dataBeg - 1],
			    LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
		}
		break;
	default:
		break;
	}
}
