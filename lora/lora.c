/* LoRa task */

#include <stdint.h>
#include <stdio.h>
#include "lmic/lmic.h"
#include "ble.h"
#include "led.h"
#include "lora/lora.h"
#include "lora/param.h"
#include "lora/proto.h"
#include "sensor/sensor.h"

#define DEBUG
//#define HELLO
//#define BLE_ALWAYS_ON

#define STATUS_JOINED		0x01
static uint8_t	status;

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
	case EV_JOINING:
		LED_SET(LED_RED, LED_BREATH);
		break;
	case EV_JOINED:
#ifdef DEBUG
		printf("netid = %lu\r\n", LMIC.netid);
#endif
		status |= STATUS_JOINED;
		LED_SET(LED_GREEN, LED_BREATH);
		proto_send_periodic_data(&sensor_job);
		break;
	case EV_TXSTART:
		proto_txstart();
		if (status & STATUS_JOINED)
			LED_SET(LED_GREEN, LED_BLINK);
		else
			LED_SET(LED_RED, LED_BREATH);
		break;
	case EV_TXCOMPLETE:
		if (LMIC.dataLen != 0) {
			proto_handle(LMIC.frame[LMIC.dataBeg - 1],
			    LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
		}
		LED_SET(LED_GREEN, LED_BREATH);
		break;
	default:
		break;
	}
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
	param_init();
	sensor_init();
	os_init();
	os_setCallback(&init_job, lora_init);
#ifdef HELLO
	os_setCallback(&hello_job, say_hi); // XXX
#endif
	os_runloop();
}
