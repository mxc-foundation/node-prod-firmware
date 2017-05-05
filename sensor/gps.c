#include <string.h>
#include <hw_gpio.h>
#include <hw_uart.h>
#include "hw/hw.h"
#include "gps.h"

#include <FreeRTOS.h>
#include <task.h>

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

extern long long	strtonum(const char *numstr, long long minval,
			    long long maxval, const char **errstrp);

static char	gps_buf[128];
static int	gps_len;

/* $GPGGA,155058.000,,,,,0,0,,,M,,M,,*44 */
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

struct datapart {
	char	*s;
	int	len;
};

struct gps_fix {
	uint8_t	fix;
	int32_t	lat;	/* Positive: North */
	int32_t	lon;	/* Positive: East */
} __attribute__((packed));

static struct gps_fix	last_fix;

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
	struct gps_fix	 fix;
	const char	*errstr;

	fix.fix = strtonum(data[GPGGA_FIX], 0, 6, &errstr);
	if (fix.fix) {
		fix.lat = parse_latlon(data[GPGGA_LAT], MAXLAT,
		    data[GPGGA_NS][0] == 'S');
		fix.lon = parse_latlon(data[GPGGA_LON], MAXLON,
		    data[GPGGA_EW][0] == 'W');
		taskENTER_CRITICAL();
		memcpy(&last_fix, &fix, sizeof(fix));
		taskEXIT_CRITICAL();
	}
}

static void
msgproc(char *msg, int len)
{
	char		*data[13];
	char		*s, *p;
	int		i;

	if (!valid_crc(msg, len))
		return;
	len -= 5;
	msg[len] = '\0';
	i = 0;
	for (s = msg; len > 0; s = p + 1) {
		p = strchr(s, ',');
		data[i] = s;
		i++;
		if (!p || i == ARRAY_SIZE(data))
			break;
		*p = '\0';
	}
	if (i && strcmp(data[0], gpgga) == 0)
		proc_gpgga(data, i);
}

static void
rx_callback(void *data, uint16_t len)
{
	static char	 rxbuf[256];
	int		 start, i;

	start = 0;
	i = 0;
	while (i < len) {
		if (rxbuf[i++] == '\n') {
			if (gps_len + i - start <= sizeof(gps_buf)) {
				if (gps_len) {
					memcpy(gps_buf + gps_len, rxbuf + start,
					    i - start);
					gps_len += i - start;
					msgproc(gps_buf, gps_len);
					gps_len = 0;
				} else {
					msgproc(rxbuf + start, i - start);
				}
			}
			start = i;
		}
	}
	if (gps_len + i - start > sizeof(gps_buf)) {
		gps_len = 0;
		return;
	}
	memcpy(gps_buf + gps_len, rxbuf + start, i - start);
	gps_len += i - start;
	hw_uart_receive(HW_UART2, rxbuf, sizeof(rxbuf), rx_callback, NULL);
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
		.tx_dma_channel		= HW_DMA_CHANNEL_1,
		.rx_dma_channel		= HW_DMA_CHANNEL_0,
	};

	hw_gpio_set_pin_function(HW_SENSOR_UART_TX_PORT, HW_SENSOR_UART_TX_PIN,
	    HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART2_TX);
	hw_gpio_set_pin_function(HW_SENSOR_UART_RX_PORT, HW_SENSOR_UART_RX_PIN,
	    HW_GPIO_MODE_INPUT,  HW_GPIO_FUNC_UART2_RX);
	hw_uart_init(HW_UART2, &uart2_cfg);
	rx_callback(NULL, 0);
}

int
gps_read(char *buf, int len)
{
	if (len < sizeof(last_fix))
		return 0;
	taskENTER_CRITICAL();
	memcpy(buf, &last_fix, sizeof(last_fix));
	taskEXIT_CRITICAL();
	if (((struct gps_fix *)buf)->fix == 0)
		return 0;
	return sizeof(last_fix);
}
