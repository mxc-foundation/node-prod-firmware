/* LoRa MatchX protocol */

#include <stdint.h>
#include <stdio.h>
#include <ad_nvparam.h>
#include <platform_nvparam.h>
#include "lmic/lmic.h"
#include "ble-suota.h"
#include "proto.h"
#include "sensor.h"

#define DEBUG
//#define HELLO
//#define BLE_ALWAYS_ON

#define PORT		0x01

#define CMD_MASK	0xf0
#define CMD_SHIFT	4
#define LEN_MASK	0x0f

#define LONG_LEN_MASK	0x3f

#define SENSOR_PERIOD	sec2osticks(60)

typedef enum {
	INFO_SENSOR_DATA	= 0x00,
	INFO_PARAM		= 0x10,
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

/* EUI-48: 78af58040000  EUI-64: 78af58fffe040000 */
static u1_t	deveui[6] = { 0x00, 0x00, 0x04, 0x58, 0xaf, 0x78, };
/* 78af580000040000 */
static u1_t	appeui[8] = { 0x00, 0x00, 0x04, 0x00, 0x00, 0x58, 0xaf, 0x78, };
/* df89dc73d9f52c0609edb2185efa4a34 */
static u1_t	devkey[16] = {
	0xdf, 0x89, 0xdc, 0x73, 0xd9, 0xf5, 0x2c, 0x06,
	0x09, 0xed, 0xb2, 0x18, 0x5e, 0xfa, 0x4a, 0x34,
};

/* NVPARAM "ble_platform" */
#define PARAM_DEV_EUI_OFF	TAG_BLE_PLATFORM_BD_ADDRESS
#define PARAM_DEV_EUI_LEN	sizeof(deveui)

/* VES */
#define PARAM_APP_EUI_OFF	0
#define PARAM_APP_EUI_LEN	sizeof(appeui)
#define PARAM_DEV_KEY_OFF	(PARAM_APP_EUI_OFF + PARAM_APP_EUI_LEN)
#define PARAM_DEV_KEY_LEN	sizeof(devkey)

#define PARAM_FLAG_BLE_NV	0x01	/* Stored in BLE NVPARAM area */
#define PARAM_FLAG_REVERSE	0x02	/* Reversed in protocol */
#define PARAM_FLAG_WRITE_ONLY	0x04	/* "Get param" disallowed */

#define PARAM_MAX_LEN		16	/* sizeof(devkey) */

struct param_def {
	void		*mem;		/* Location in memory */
	uint16_t	 offset;	/* Location in permanent storage */
	uint8_t		 len;		/* Length */
	uint8_t		 flags;		/* Flags */
};

static const struct param_def	params[] = {
	{
		.mem	= deveui,
		.offset	= PARAM_DEV_EUI_OFF,
		.len	= PARAM_DEV_EUI_LEN,
		.flags	= PARAM_FLAG_BLE_NV | PARAM_FLAG_REVERSE,
	},
	{
		.mem	= appeui,
		.offset	= PARAM_APP_EUI_OFF,
		.len	= PARAM_APP_EUI_LEN,
		.flags	= PARAM_FLAG_REVERSE,
	},
	{
		.mem	= devkey,
		.offset	= PARAM_DEV_KEY_OFF,
		.len	= PARAM_DEV_KEY_LEN,
		.flags	= PARAM_FLAG_WRITE_ONLY,
	},
};

static inline void
reverse_memcpy(void *dest, void *src, size_t len)
{
	char	*d = dest, *s = src;

	do {
		d[--len] = *s++;
	} while (len);
}

void
os_getArtEui(u1_t *buf)
{
	memcpy(buf, appeui, sizeof(appeui));
}

void
os_getDevEui(u1_t *buf)
{
	memcpy(buf, deveui, 3);
	buf[3] = 0xfe;
	buf[4] = 0xff;
	memcpy(buf + 5, deveui + 3, 3);
}

void
os_getDevKey(u1_t *buf)
{
	memcpy(buf, devkey, sizeof(devkey));
}

/* Read param from permanent storage into memory */
static void
read_param(const struct param_def *param)
{
	uint8_t	buf[PARAM_MAX_LEN];

	if (param->flags & PARAM_FLAG_BLE_NV) {
		nvparam_t	nvparam;
		uint16_t	param_len;
		uint8_t		valid;

		nvparam = ad_nvparam_open("ble_platform");
		param_len = ad_nvparam_get_length(nvparam, param->offset, NULL);
		OS_ASSERT(param_len == param->len + 1);
		ad_nvparam_read_offset(nvparam, param->offset,
		    param_len - sizeof(valid), sizeof(valid), &valid);
		if (valid != 0x00)
			return;
		ad_nvparam_read(nvparam, param->offset, param->len, param->mem);
	} else {
		nvms_t	nvms;
		int	i;

		OS_ASSERT(sizeof(buf) >= param->len);
		nvms = ad_nvms_open(NVMS_GENERIC_PART);
		ad_nvms_read(nvms, param->offset, buf, param->len);
		for (i = 0; i < param->len; i++) {
			if (buf[i] != 0xff) {
				memcpy(param->mem, buf, param->len);
				return;
			}
		}
	}
}

/* Set param in memory and write it to permanent storage */
static void
write_param(const struct param_def *param, void *data)
{
	uint8_t		buf[PARAM_MAX_LEN + 1];

	OS_ASSERT(param->len <= sizeof(buf));
	if (param->flags & PARAM_FLAG_REVERSE)
		reverse_memcpy(buf, data, param->len);
	else
		memcpy(buf, data, param->len);
	memcpy(param->mem, buf, param->len);
	if (param->flags & PARAM_FLAG_BLE_NV) {
		nvparam_t	nvparam;
		uint16_t	param_len;

		nvparam = ad_nvparam_open("ble_platform");
		param_len = ad_nvparam_get_length(nvparam, param->offset, NULL);
		OS_ASSERT(param_len == param->len + 1);
		OS_ASSERT(param_len <= sizeof(buf));
		buf[param->len] = 0x00;
		ad_nvparam_write(nvparam, param->offset, param->len + 1, buf);
		//ad_nvms_flush(ad_nvms_open(NVMS_PARAM_PART), 0);
	} else {
		nvms_t		nvms;

		nvms = ad_nvms_open(NVMS_GENERIC_PART);
		ad_nvms_write(nvms, param->offset, buf, param->len);
	}
}

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
tx_enqueue(uint8_t cmd, int len, void *data)
{
	if (ARRAY_SIZE(pend_tx_data) - pend_tx_len < len + 1 + (len > LEN_MASK))
		return;
	if (len <= LEN_MASK)
		pend_tx_data[pend_tx_len++] = cmd | len;
	else {
		pend_tx_data[pend_tx_len++] = cmd | LEN_MASK;
		pend_tx_data[pend_tx_len++] = len & LONG_LEN_MASK;
	}
	memcpy(pend_tx_data + pend_tx_len, data, len);
	pend_tx_len += len;
#ifdef DEBUG
	printf("set tx data:");
	for (int i = 0; i < pend_tx_len; i++)
		printf(" %02x", pend_tx_data[i]);
	printf("\r\n");
#endif
	LMIC_setTxData2(PORT, pend_tx_data, pend_tx_len, 0);
	status |= STATUS_TX_PENDING;
}

static void
handle_params(uint8_t *data, uint8_t len)
{
	uint8_t	buf[PARAM_MAX_LEN + 1];
	uint8_t	idx;

	if (len == 0)
		return;
	idx = *data++;
	len--;
	if (len == 0) {
		/* get */
		if (!(params[idx].flags & PARAM_FLAG_WRITE_ONLY)) {

			buf[0] = idx;
			if (params[idx].flags & PARAM_FLAG_REVERSE) {
				reverse_memcpy(buf + 1, params[idx].mem,
				    params[idx].len);
			} else {
				memcpy(buf + 1, params[idx].mem,
				    params[idx].len);
			}
			tx_enqueue(INFO_PARAM, params[idx].len + 1, buf);
		}
	} else {
		/* set */
		if (idx >= ARRAY_SIZE(params) || params[idx].len != len)
			return;
		write_param(params + idx, data);
	}
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
	CMD_GET_SET_PARAMS	= 0x0,
	CMD_REBOOT_UPGRADE	= 0x1,
} downlink_cmd;

static void	(* const downlink_handlers[])(uint8_t *, uint8_t) = {
	[CMD_GET_SET_PARAMS]	= handle_params,
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
	char	buf[MAX_LEN_PAYLOAD];
	size_t	len;

	os_setTimedCallback(job, hal_ticks() + SENSOR_PERIOD,
	    proto_send_sensor_data);
	if (ARRAY_SIZE(pend_tx_data) - pend_tx_len < 2)
		return;
	len = sensor_get_data(buf, sizeof(buf));
	if (len == 0)
		return;
	tx_enqueue(INFO_SENSOR_DATA, len, buf);
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
#ifdef DEBUG
		printf("netid = %lu\r\n", LMIC.netid);
#endif
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

static void
param_init(void)
{
	int	i;

	for (i = 0; i < ARRAY_SIZE(params); i++) {
		read_param(params + i);
#ifdef DEBUG
		for (int j = 0; j < params[i].len; j++)
			printf("%02x", ((uint8_t *)params[i].mem)[j]);
		printf("\r\n");
#endif
	}
#ifdef DEBUG
	uint8_t buf[8];
	os_getDevEui(buf);
	for (int j = 0; j < 8; j++)
		printf("%02x", buf[j]);
	printf("\r\n");
#endif
}

#ifdef HELLO
static void
say_hi(osjob_t *job)
{
	static int	n;

	os_setTimedCallback(job, os_getTime() + sec2osticks(1), say_hi);
	printf("Hello #%u @ %u (%ld), %04x\r\n",
	    n++, os_getTimeSecs(), os_getTime(), *(uint16_t*)0x5000000a);
}
#endif

static void
lora_init(osjob_t* j)
{
	LMIC_reset();
	LMIC_startJoining();
}

void
lora_task_func(void *param)
{
	osjob_t	init_job;
#ifdef HELLO
	osjob_t	hello_job;
#endif

#ifdef BLE_ALWAYS_ON
	ble_on();
#endif
	param_init();
	sensor_init();
	os_init();
	os_setCallback(&init_job, lora_init);
#ifdef HELLO
	os_setCallback(&hello_job, say_hi); // XXX
#endif
	os_runloop();
}
