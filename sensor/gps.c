#include <string.h>
#include <FreeRTOS.h>
#include <hw_gpio.h>
#include <hw_uart.h>
#include <task.h>
#include "hw/hw.h"
#include "lmic/oslmic.h"
#include "gps.h"

//#define DEBUG

#ifdef DEBUG
#include <stdio.h>
#include <unistd.h>
#endif

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

extern long long	strtonum(const char *numstr, long long minval,
    long long maxval, const char **errstrp);

PRIVILEGED_DATA static char	rxbuf[128];
PRIVILEGED_DATA static int	rxlen;
PRIVILEGED_DATA static osjob_t	rxjob;

/* $GPGGA,155058.000,,,,,0,0,,,M,,M,,*44 */
/* $GPGGA,135704.000,5231.1618,N,01324.2888,E,1,3,5.64,105.3,M,44.7,M,,*59 */
enum {
	GPGGA_MSGID,		/* Message ID */
	GPGGA_TIME,		/* UTC time */
	GPGGA_LAT,		/* Latitude */
	GPGGA_NS,		/* N/S Indicator */
	GPGGA_LON,		/* Longitude */
	GPGGA_EW,		/* E/W Indicator */
	GPGGA_FIX,		/* Position Fix Indicator */
	GPGGA_SAT,		/* Satellites Used */
	GPGGA_HDOP,		/* HDOP, Hotizontal Dilution of Precision */
	GPGGA_MSL_ALT,		/* MSL Altitude */
	GPGGA_MSL_UNITS,	/* Units ("M") */
	GPGGA_GEOSEP,		/* Geoid Separation */
	GPGGA_GEOSEP_UNITS,	/* Units ("M") */
	GPGGA_AGE_DIFF,		/* Age of Diff. Corr. */
	GPGGA_STATION_ID,	/* Diff. Ref. Station ID */
};
#define MAX_FIELDS	(GPGGA_STATION_ID + 1)

struct datapart {
	char	*s;
	int	len;
};

struct gps_fix {
	uint8_t	fix;
	int32_t	lat;	/* Positive: North */
	int32_t	lon;	/* Positive: East */
} __attribute__((packed));

PRIVILEGED_DATA static struct gps_fix	last_fix;
PRIVILEGED_DATA static bool		fix_found;

static const char	hex[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};
static const char	crlf[] = { '\r', '\n', };
static const char	gpgga[] = "$GPGGA";

static bool
valid_crc(char *msg, int len)
{
	char	*p;
	int	 ccrc, icrc; /* computed and indicated CRCs */

	if (*msg != '$')
		return false;
	/* String must end with "*XX\r\n", where XX are two hex digits. */
	p = memchr(msg, '*', len);
	if (p != msg + len - 5)
		return false;
	if (memcmp(msg + len - sizeof(crlf), crlf, sizeof(crlf)) != 0)
		return false;
	if ((p = memchr(hex, msg[len - 4], sizeof(hex))) == NULL)
		return false;
	icrc = (p - hex) << 4;
	if ((p = memchr(hex, msg[len - 3], sizeof(hex))) == NULL)
		return false;
	icrc |= p - hex;
	ccrc = 0;
	for (p = msg + 1; p < msg + len - 5; p++) {
		ccrc ^= *p;
	}
	return ccrc == icrc;
}

#define MAXLAT	9000
#define MAXLON	18000
static int32_t
parse_latlon(char *s, int max, bool negate)
{
	const char	*errstr;
	char		*frac;
	int32_t		 d, m, f;	/* degrees, minutes, fraction */
	int		 i;

	if ((frac = strchr(s, '.')) == NULL)
		return 0;
	*frac++ = '\0';
	m = strtonum(s, 0, max, &errstr);
	if (errstr)
		return 0;
	d = m / 100;
	m %= 100;
	f = strtonum(frac, 0, 9999, &errstr);
	if (errstr)
		return 0;
	for (i = 0; i < 4 - strlen(frac); i++)
		f *= 10;
	return ((d * 60 + m) * 10000 + f) * (negate ? -1 : 1);
}

static void
proc_gpgga(char *data[], int sz)
{
	struct gps_fix	 fix = {};
	const char	*errstr;

#ifdef DEBUG
	printf("gga\r\n");
#endif
	fix.fix = strtonum(data[GPGGA_FIX], 0, 6, &errstr);
	if (fix.fix) {
		fix.lat = parse_latlon(data[GPGGA_LAT], MAXLAT,
		    data[GPGGA_NS][0] == 'S');
		fix.lon = parse_latlon(data[GPGGA_LON], MAXLON,
		    data[GPGGA_EW][0] == 'W');
		memcpy(&last_fix, &fix, sizeof(fix));
	}
}

static bool
msgproc(char *msg, int len)
{
	char	*data[MAX_FIELDS];
	char	*s, *p;
	int	 i;

#ifdef DEBUG
	write(1, msg, len);
#endif
	if (!valid_crc(msg, len))
		return false;
	msg[len - 5] = '\0';
	i = 0;
	for (s = msg; *s != '\0'; s = p + 1) {
		data[i++] = s;
		if (i == ARRAY_SIZE(data) || (p = strchr(s, ',')) == NULL)
			break;
		*p = '\0';
	}
	if (i && strcmp(data[0], gpgga) == 0) {
		proc_gpgga(data, i);
		return true;
	}
	return false;
}

static void
rx(osjob_t *job)
{
#ifdef DEBUG
	printf("rx: ");
#endif
	while (!hw_uart_read_buf_empty(HW_UART2)) {
		uint8_t	c;

		if (rxlen >= sizeof(rxbuf)) {
			rxlen = 0;
			continue;
		}
		c = hw_uart_read(HW_UART2);
#ifdef DEBUG
		printf("%c", c);
#endif
		if ((rxbuf[rxlen++] = c) == '\n') {
			if (msgproc(rxbuf, rxlen))
				fix_found = true;
			rxlen = 0;
		}
	}
	if (!fix_found)
		os_setTimedCallback(job, os_getTime() + ms2osticks(10), rx);
#ifdef DEBUG
	printf("\r\n");
#endif
}

void
gps_init()
{
	const uart_config	uart2_cfg = {
		.baud_rate		= HW_UART_BAUDRATE_9600,
		.data			= HW_UART_DATABITS_8,
		.parity			= HW_UART_PARITY_NONE,
		.stop			= HW_UART_STOPBITS_1,
		.auto_flow_control	= 0,
		.use_dma		= 0,
		.use_fifo		= 1,
	};

	hw_gpio_set_pin_function(HW_SENSOR_UART_TX_PORT, HW_SENSOR_UART_TX_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART2_TX);
	hw_gpio_set_pin_function(HW_SENSOR_UART_RX_PORT, HW_SENSOR_UART_RX_PIN,
	    HW_GPIO_MODE_INPUT,  HW_GPIO_FUNC_UART2_RX);
	hw_uart_init(HW_UART2, &uart2_cfg);
}

void
gps_prepare()
{
	rxlen = 0;
	memset(&last_fix, 0, sizeof(last_fix));
	fix_found = false;
	while (!hw_uart_read_buf_empty(HW_UART2))
		hw_uart_read(HW_UART2);
	os_setCallback(&rxjob, rx);
}

ostime_t
gps_data_ready()
{
	return fix_found ? 0 : ms2osticks(100);
}

int
gps_read(char *buf, int len)
{
	os_clearCallback(&rxjob);
	if (len < sizeof(last_fix) || last_fix.fix == 0)
		return 0;
	memcpy(buf, &last_fix, sizeof(last_fix));
	return sizeof(last_fix);
}
