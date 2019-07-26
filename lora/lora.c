/* LoRa task */

#include <stdint.h>
#include <stdio.h>

#include "ble.h"
#include "hw/hw.h"
#include "hw/iox.h"
#include "hw/led.h"
#include "lmic/lmic.h"
#include "lora/ad_lora.h"
#include "lora/lora.h"
#include "lora/param.h"
#include "lora/proto.h"
#include "lora/upgrade.h"
#include "lora/util.h"
#include "sensor/sensor.h"

#define DEBUG
#define DEBUG_TIME
//#define HELLO
//#define BLE_ALWAYS_ON

/* State of the sampling/sending state machine */
#define STATE_IDLE		0
#define STATE_SAMPLING_SENSOR	1
#define STATE_SENDING		2
PRIVILEGED_DATA static uint8_t	state;

/* Link status */
#define STATUS_JOINED		0x01
#define STATUS_LINK_UP		0x02
PRIVILEGED_DATA static uint8_t	status;

#define MAX_SENSOR_SAMPLE_TIME	sec2osticks(2)
PRIVILEGED_DATA static ostime_t	sampling_since;

#define JOIN_TIMEOUT		sec2osticks(2 * 60 * 60)
#define REJOIN_TIMEOUT		sec2osticks(15 * 60)
#define TX_TIMEOUT		sec2osticks(12)
#define TX_PERIOD_TIMEOUT	sec2osticks(10 * 60)
#define ALIVE_TX_PERIOD		sec2osticks(60)
#define SEND_RETRY_TIME		sec2osticks(10)

#define MAX_RESETS		8

#ifdef DEBUG

#ifdef DEBUG_TIME
static void
debug_time(void)
{
	uint32_t	now = os_getTime();

	printf("%lu:%02lu.%03lu+%02lu ", osticks2ms(now) / 60000,
	    osticks2ms(now) / 1000 % 60, osticks2ms(now) % 1000,
	    now - ms2osticks(osticks2ms(now)));
}
#else
#define debug_time()
#endif

static void
debug_event(int ev)
{
	static const char	*evnames[] = {
		[EV_SCAN_TIMEOUT]	= "SCAN_TIMEOUT",
		[EV_BEACON_FOUND]	= "BEACON_FOUND",
		[EV_BEACON_MISSED]	= "BEACON_MISSED",
		[EV_BEACON_TRACKED]	= "BEACON_TRACKED",
		[EV_JOINING]		= "JOINING",
		[EV_JOINED]		= "JOINED",
		[EV_RFU1]		= "RFU1",
		[EV_JOIN_FAILED]	= "JOIN_FAILED",
		[EV_REJOIN_FAILED]	= "REJOIN_FAILED",
		[EV_TXCOMPLETE]		= "TXCOMPLETE",
		[EV_LOST_TSYNC]		= "LOST_TSYNC",
		[EV_RESET]		= "RESET",
		[EV_RXCOMPLETE]		= "RXCOMPLETE",
		[EV_LINK_DEAD]		= "LINK_DEAD",
		[EV_LINK_ALIVE]		= "LINK_ALIVE",
		[EV_SCAN_FOUND]		= "SCAN_FOUND",
		[EV_TXSTART]		= "TXSTART",
	};

	debug_time();
	printf("%s\r\n",
	    ev < (int)ARRAY_SIZE(evnames) ? evnames[ev] : "UNKNOWN");
}
#else /* !DEBUG */
#define debug_event(ev)
#endif /* DEBUG */

#ifdef HW_IOX_I2C_ADDR

#define PIN_BIT0_0	0x0f
#define PIN_BIT0_IN	0x0d
#define PIN_BIT0_1	0x0b

#define PIN_BIT1_0	0x0e
#define PIN_BIT1_IN	0x0c
#define PIN_BIT1_1	0x0a

#define PIN_BIT2_0	0x09
#define PIN_BIT2_IN	0x07
#define PIN_BIT2_1	0x05

#define PIN_BIT3_0	0x08
#define PIN_BIT3_IN	0x06
#define PIN_BIT3_1	0x04

#define PINS_INPUT	((1<<PIN_BIT0_IN) | (1<<PIN_BIT1_IN) | \
			 (1<<PIN_BIT2_IN) | (1<<PIN_BIT3_IN))
#define PINS_ONES	((1<<PIN_BIT0_1) | (1<<PIN_BIT1_1) | \
			 (1<<PIN_BIT2_1) | (1<<PIN_BIT3_1))
#define PINS_NOTCONF	((1<<0x00) | (1<<0x01) | (1<<0x02) | (1<<0x03))

static uint8_t
lora_autodetect_region(void)
{
	int	pins;
	uint8_t	region;

	if (iox_setconf(PINS_INPUT | PINS_NOTCONF)) {
#ifdef DEBUG
		printf("IOX: cannot set configuration\r\n");
#endif
		return 0xff;
	}
	if (iox_setpins(PINS_ONES)) {
#ifdef DEBUG
		printf("IOX: cannot set pins\r\n");
#endif
		return 0xff;
	}
	if ((pins = iox_getpins()) == -1) {
#ifdef DEBUG
		printf("IOX: cannot get pins\r\n");
#endif
		return 0xff;
	}
	region = 0;
	if (pins & (1 << PIN_BIT0_IN))
		region |= 0x01;
	if (pins & (1 << PIN_BIT1_IN))
		region |= 0x02;
	if (pins & (1 << PIN_BIT2_IN))
		region |= REGION_WIDEBAND;
	if (pins & (1 << PIN_BIT3_IN))
		region |= REGION_FULL;
	return region;
}

#else /* !HW_IOX_I2C_ADDR */

#define lora_autodetect_region()	REGION_EU

#endif /* HW_IOX_I2C_ADDR */

static uint8_t
lora_get_region(void)
{
	uint8_t	region;

	param_get(PARAM_LORA_REGION, &region, sizeof(region));
	if (region == 0xff)
		region = lora_autodetect_region();
#ifdef DEBUG
	printf("region %02x\r\n", region);
#endif
	return region;
}

static void
lora_start_joining(osjob_t *job)
{
	if (!cm_lp_clk_is_avail()) {
		os_setTimedCallback(job, os_getTime() + sec2osticks(1),
		    lora_start_joining);
		return;
	}
	LMIC_startJoining();
}

static void
lora_reset(osjob_t *job)
{
	PRIVILEGED_DATA static uint8_t	reset_count;

#ifdef DEBUG
	debug_time();
	printf("lora reset #%d\r\n", reset_count);
#endif
	if (++reset_count > MAX_RESETS)
		hal_failed();
	status = 0;
	if (LMIC_reset(lora_get_region()) == -1)
		return;
	os_setCallback(job, lora_start_joining);
}

static void
lora_reset_after(ostime_t delay)
{
	PRIVILEGED_DATA static osjob_t	reset_job;

	os_setTimedCallback(&reset_job, os_getTime() + delay, lora_reset);
}

#define lora_init()	lora_reset_after(sec2osticks(1))
#define lora_reinit()	lora_reset_after(0)

static void	lora_send_init(osjob_t *job);

static void
lora_schedule_next_send(osjob_t *job, ostime_t delay)
{
	os_setTimedCallback(job, os_getTime() + delay + os_getRndU2(),
	    lora_send_init);
}

static void
lora_send_wait(osjob_t *job)
{
	ostime_t	delay;

	if (os_getTime() < sampling_since + MAX_SENSOR_SAMPLE_TIME &&
	    (delay = sensor_data_ready()) != 0) {
		os_setTimedCallback(job, os_getTime() + delay,
		    lora_send_wait);
		ad_lora_suspend_sleep(LORA_SUSPEND_LORA, delay + 64);
	} else {
		state = STATE_IDLE;
		led_notify(LED_STATE_IDLE);
		proto_send_data();
		lora_schedule_next_send(job, sensor_period());
	}
}

static void
lora_send_init(osjob_t *job)
{
#ifdef DEBUG
	debug_time();
	printf("lora_send_init: state %d\r\n", state);
#endif
	switch (state) {
	case STATE_IDLE:
		if (status & STATUS_LINK_UP) {
			state = STATE_SAMPLING_SENSOR;
			sampling_since = os_getTime();
			led_notify(LED_STATE_SAMPLING_SENSOR);
			sensor_prepare();
			lora_send_wait(job);
			return;
		} else if (status & STATUS_JOINED) {
			LMIC_sendAlive();
		}
		lora_schedule_next_send(job, ALIVE_TX_PERIOD);
		break;
	case STATE_SAMPLING_SENSOR:
		break;
	case STATE_SENDING:
		lora_schedule_next_send(job, SEND_RETRY_TIME);
		break;
	}
}

void
lora_send(void)
{
	PRIVILEGED_DATA static osjob_t	sensor_job;

	lora_send_init(&sensor_job);
}

void
onEvent(ev_t ev)
{
	debug_event(ev);
	switch(ev) {
	case EV_LINK_DEAD:
		status &= ~STATUS_LINK_UP;
		lora_send();
		/* NO BREAK FALLTHROUGH */
	case EV_JOINING:
		state = STATE_IDLE;
		led_notify(LED_STATE_JOINING);
		lora_reset_after(JOIN_TIMEOUT);
		break;
	case EV_JOINED:
#ifdef DEBUG
		printf("netid = %06lx\r\n", LMIC.netid);
#endif
		/* NO BREAK FALLTHROUGH */
	case EV_LINK_ALIVE:
		status |= STATUS_JOINED | STATUS_LINK_UP;
		state = STATE_IDLE;
		lora_reset_after(TX_PERIOD_TIMEOUT);
		lora_send();
		break;
	case EV_JOIN_FAILED:
		lora_reinit();
		break;
	case EV_REJOIN_FAILED:
		lora_reset_after(REJOIN_TIMEOUT);
		ad_lora_allow_sleep(LORA_SUSPEND_LORA);
		break;
	case EV_TXSTART:
		ad_lora_suspend_sleep(LORA_SUSPEND_LORA, TX_TIMEOUT);
		proto_txstart();
		if (status & STATUS_LINK_UP) {
			state = STATE_SENDING;
			led_notify(LED_STATE_SENDING);
			lora_reset_after(TX_TIMEOUT);
		} else {
			led_notify(LED_STATE_JOINING);
		}
		break;
	case EV_TXCOMPLETE:
		if (status & STATUS_LINK_UP) {
			ostime_t	delay;

			delay = sensor_period() + sec2osticks(5);
			if (delay < TX_PERIOD_TIMEOUT)
				delay = TX_PERIOD_TIMEOUT;
			lora_reset_after(delay);
			led_notify(LED_STATE_IDLE);
		}
		if (LMIC.dataLen != 0) {
			proto_handle(LMIC.frame[LMIC.dataBeg - 1],
			    LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
		}
		state = STATE_IDLE;
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
	printf("Hello #%u @ %u (%ld)\r\n", n++, os_getTimeSecs(), os_getTime());
}
#endif

void
lora_task_func(void *param)
{
#ifdef HELLO
	osjob_t	hello_job;
#endif
	(void)param;
	param_init();
	ad_lora_init();
	os_init();
	led_notify(LED_STATE_BOOTING);
	// check if the suota upgrade bit was set before reboot.
	upgrade_init();
#ifdef BLE_ALWAYS_ON
	ble_on();
#endif
#ifdef HELLO
	os_setCallback(&hello_job, say_hi);
#endif
	// initialize lora structures.
	lora_init();
	// start main loop of lmic os.
	os_runloop();
	// should never reach here.
}
