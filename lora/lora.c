/* LoRa task */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ble.h"
#include "hw/led.h"
#include "lmic/lmic.h"
#include "lora/ad_lora.h"
#include "lora/lora.h"
#include "lora/param.h"
#include "lora/proto.h"
#include "lora/upgrade.h"
#include "sensor/sensor.h"

#define DEBUG
//#define HELLO
//#define BLE_ALWAYS_ON

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

#define STATUS_JOINED		0x01
PRIVILEGED_DATA static uint8_t	status;

#ifdef DEBUG
static void
debug_event(int ev)
{
	static const char	*evnames[] = {
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
	const char		*s;

	s = ev < ARRAY_SIZE(evnames) ? evnames[ev] : "EV_UNKNOWN";
	write(1, s, strlen(s));
	write(1, "\r\n", 2);
}
#else
#define debug_event(ev)
#endif

void
onEvent(ev_t ev)
{
	debug_event(ev);
	switch(ev) {
	case EV_JOINING:
		led_notify(LED_STATE_JOINING);
		break;
	case EV_JOINED:
#ifdef DEBUG
		{
			char	buf[32];

			write(1, buf, snprintf(buf, sizeof(buf),
				    "netid = %lu\r\n", LMIC.netid));
		}
#endif
		status |= STATUS_JOINED;
		led_notify(LED_STATE_IDLE);
		proto_send_data();
		break;
	case EV_TXSTART:
		ad_lora_suspend_sleep(LORA_SUSPEND_LORA, sec2osticks(7));
		proto_txstart();
		if (status & STATUS_JOINED)
			led_notify(LED_STATE_SENDING);
		else
			led_notify(LED_STATE_JOINING);
		break;
	case EV_TXCOMPLETE:
		if (LMIC.dataLen != 0) {
			proto_handle(LMIC.frame[LMIC.dataBeg - 1],
			    LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
		}
		led_notify(LED_STATE_IDLE);
		ad_lora_allow_sleep(LORA_SUSPEND_LORA);
		break;
	default:
		break;
	}
}

#ifdef HELLO
static void
say_hi(osjob_t *job)
{
	PRIVILEGED_DATA static int	n;
	char				buf[64];

	os_setTimedCallback(job, os_getTime() + sec2osticks(1), say_hi);
	write(1, buf, snprintf(buf, sizeof(buf), "Hello #%u @ %u (%ld)\r\n",
		    n++, os_getTimeSecs(), os_getTime()));
}
#endif

static void
lora_init(osjob_t *j)
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
	led_notify(LED_STATE_BOOTING);
	param_init();
	sensor_init();
	ad_lora_init();
	os_init();
	upgrade_init();
	os_setCallback(&init_job, lora_init);
#ifdef HELLO
	os_setCallback(&hello_job, say_hi);
#endif
	os_runloop();
}
