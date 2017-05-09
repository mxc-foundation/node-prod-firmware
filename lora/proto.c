/* LoRa MatchX protocol */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "hw/led.h"
#include "lmic/lmic.h"
#include "lora/lora.h"
#include "lora/param.h"
#include "lora/proto.h"
#include "lora/upgrade.h"
#include "sensor/bat.h"
#include "sensor/sensor.h"

#define DEBUG

#define PORT		0x01

#define CMD_MASK	0xf0
#define CMD_SHIFT	4
#define LEN_MASK	0x0f

#define LONG_LEN_MASK	0x3f

typedef enum {
	INFO_PARAM		= 0x00,
	INFO_SENSOR_DATA	= 0x10,
	INFO_BATTERY		= 0x20,
} uplink_info;

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

#define STATUS_TX_PENDING	0x01
PRIVILEGED_DATA static uint8_t	status;

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

#define MAX_PAYLOAD_LEN		51
#define MAX_SENSOR_DATA_LEN	16
#define MAX_BATTERY_DATA_LEN	2

PRIVILEGED_DATA static uint8_t	pend_tx_data[MAX_PAYLOAD_LEN];
PRIVILEGED_DATA static uint8_t	sensor_data[MAX_SENSOR_DATA_LEN];
PRIVILEGED_DATA static uint8_t	battery_data[MAX_BATTERY_DATA_LEN];
PRIVILEGED_DATA static uint8_t	pend_tx_len, sensor_len, battery_len;

#define LEN_LEN(len)	(1 + ((len) >= LEN_MASK))

static void
tx_enqueue(uint8_t *dest, uint8_t *dlen, uint8_t maxlen,
    uint8_t cmd, int len, void *data)
{
	if (*dlen + LEN_LEN(len) + len > maxlen)
		return;
	if (len < LEN_MASK)
		dest[(*dlen)++] = cmd | len;
	else {
		dest[(*dlen)++] = cmd | LEN_MASK;
		dest[(*dlen)++] = len & LONG_LEN_MASK;
	}
	memcpy(dest + *dlen, data, len);
	*dlen += len;
}

#define ADD_TX(x)	do {						\
	if (total_len + x ## _len <= ARRAY_SIZE(pend_tx_data)) {	\
		memcpy(pend_tx_data + total_len, x ## _data, x ## _len);\
		total_len += x ## _len;					\
	}								\
} while (0)

static void
set_tx_data(void)
{
	int	total_len = pend_tx_len;

	ADD_TX(battery);
	ADD_TX(sensor);
#ifdef DEBUG
	char	buf[4];
	write(1, "set tx data:", 12);
	for (int i = 0; i < total_len; i++) {
		write(1, buf, snprintf(buf, sizeof(buf), " %02x",
			    pend_tx_data[i]));
	}
	write(1, "\r\n", 2);
#endif
	if (total_len) {
		LMIC_setTxData2(PORT, pend_tx_data, total_len, 0);
		status |= STATUS_TX_PENDING;
	}
}

#define TX_CLEAR(x)	do {						\
	x ## _len = 0;							\
} while (0)

#define TX_SET(x, cmd, len, data)	do {				\
	TX_CLEAR(x);							\
	tx_enqueue(x ## _data, &x ## _len, ARRAY_SIZE(x ## _data),	\
	    cmd, len, data);						\
} while (0)

#define TX_ENQUEUE(cmd, len, data)	TX_SET(pend_tx, cmd, len, data)


static void
handle_params(uint8_t *data, uint8_t len)
{
	uint8_t	buf[PARAM_MAX_LEN + 1];
	uint8_t	idx, plen;

	if (len == 0)
		return;
	idx = *data++;
	len--;
	if (len == 0) {
		/* get */
		if ((plen = param_get(idx, buf + 1, sizeof(buf) - 1)) != 0) {
			buf[0] = idx;
			TX_ENQUEUE(INFO_PARAM, plen + 1, buf);
		}
	} else {
		/* set */
		param_set(idx, data, len);
	}
}

static void
handle_reboot_upgrade(uint8_t *data, uint8_t len)
{
	switch (len) {
	case 0:
		upgrade_reboot(0);
		break;
	case 1:
		upgrade_reboot(data[0]);
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

void
proto_handle(uint8_t port, uint8_t *data, uint8_t len)
{
	uint8_t	cmd, plen;

#ifdef DEBUG
	char	buf[4];

	write(1, "rx:", 3);
	for (int i = 0; i < len; i++)
		write(1, buf, snprintf(buf, sizeof(buf), " %02x", data[i]));
	write(1, "\r\n", 2);
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
	set_tx_data();
}

void
proto_send_data(void)
{
	PRIVILEGED_DATA static uint8_t	last_bat_level;
	char				buf[MAX_LEN_PAYLOAD];
	size_t				len;
	uint8_t				cur_bat_level;

	cur_bat_level = bat_level();
	if (cur_bat_level != last_bat_level) {
		last_bat_level = cur_bat_level;
		TX_SET(battery, INFO_BATTERY, 1, &cur_bat_level);
	}
	len = sensor_get_data(buf, sizeof(buf));
	if (len != 0)
		TX_SET(sensor, INFO_SENSOR_DATA, len, buf);
	set_tx_data();
}

void
proto_txstart(void)
{
	status &= ~STATUS_TX_PENDING;
	TX_CLEAR(pend_tx);
	TX_CLEAR(sensor);
	TX_CLEAR(battery);
}
