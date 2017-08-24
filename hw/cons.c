#include <stdio.h>
#include <unistd.h>

#include <hw_gpio.h>
#include <hw_uart.h>
#include <resmgmt.h>

#include "hw/hw.h"
#include "hw/cons.h"
#include "lmic/lmic.h"
#include "lora/ad_lora.h"
#include "lora/lora.h"

#define CONSOLE_INPUT

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

extern void		hal_uart_rx(char c);
extern long long	strtonum(const char *numstr, long long minval,
    long long maxval, const char **errstrp);

#ifdef CONFIG_CUSTOM_PRINT

int
_write(int fd, const void *ptr, int len)
{
	(void)fd;
	hw_uart_write_buffer(HW_UART1, ptr, len);
	return len;
}

#ifdef CONSOLE_INPUT

#define writech(c)	do {	\
	char	buf = c;	\
	_write(1, &buf, 1);	\
} while (0)

#define isasciispace(c)	((uint8_t)(c) <= ' ')

#define CTRL(x)	((x) ^ 0x40)
#define ESC	CTRL('[')

static const char	BELL[]		= { '\a' };
static const char	BACKSPACE[]	= { '\b', ' ', '\b' };
static const char	CRLF[]		= { '\r', '\n' };
static const char	CTRL_C_CRLF[]	= { '^', 'C', '\r', '\n' };
static const char	CTRL_R_CRLF[]	= { '^', 'R', '\r', '\n' };
static const char	CR_ERASE_LINE[]	= { '\r', ESC, '[', '2', 'K' };

static PRIVILEGED_DATA char	cons_buf[128];
static PRIVILEGED_DATA uint8_t	cons_len;

static inline void
backspace()
{
	_write(1, BACKSPACE, sizeof(BACKSPACE));
	cons_len--;
}

static void
cmd_reset(int argc, char **argv)
{
	(void)argv;
	(void)argc;
	printf("Rebooting...\r\n");
	hw_cpm_reboot_system();
}

struct command {
	const char	*cmd;
	const char	 minargs, maxargs;
	void		(*handler)(int, char **);
};

static const struct command	cmd[] = {
	{ "reset", 1, 1, cmd_reset },
};

static int
tokenise(char **tokv, int toklen)
{
	char	*s;
	int	 tokc;

	if (cons_len >= sizeof(cons_buf))
		return -1;
	cons_buf[cons_len] = '\0';
	tokc = 0;
	for (s = cons_buf; *s != '\0' && isasciispace(*s); s++)
		;
	while (*s != '\0') {
		tokv[tokc++] = s;
		if (tokc == toklen)
			break;
		while (*s != '\0' && !isasciispace(*s))
			s++;
		while (*s != '\0' && isasciispace(*s))
			*s++ = '\0';
	}
	return tokc;
}

static void
runcmd(const struct command *c, int tokc, char **tokv) {
	if (tokc < c->minargs || tokc > c->maxargs) {
		printf("%s: needs between %d and %d args\r\n",
		    c->cmd, c->minargs - 1,
		    c->maxargs - 1);
		return;
	}
	c->handler(tokc, tokv);
}

static void
handle_line()
{
	char	*tokv[8];
	int	 tokc, i, cand;

	tokc = tokenise(tokv, (int)ARRAY_SIZE(tokv));
	if (tokc <= 0)
		return;
	cand = -1;
	for (i = 0; i < (int)ARRAY_SIZE(cmd); i++) {
		if (strcmp(cmd[i].cmd, tokv[0]) == 0) {
			runcmd(cmd + i, tokc, tokv);
			return;
		}
		if (strncmp(cmd[i].cmd, tokv[0], strlen(tokv[0])) == 0) {
			switch (cand) {
			case -1:
				cand = i;
				break;
			default:
				cand = -2;
				break;
			}
		}
	}
	if (cand >= 0) {
		runcmd(cmd + cand, tokc, tokv);
		return;
	}
	printf("%s: command not found\r\n", tokv[0]);
}

void
cons_rx(uint8_t c)
{
	switch (c) {
	case CTRL('C'):
		_write(1, CTRL_C_CRLF, sizeof(CTRL_C_CRLF));
		cons_len = 0;
		break;
	case CTRL('H'):
	case CTRL('?'):
		if (cons_len)
			backspace();
		break;
	case CTRL('U'):
		_write(1, CR_ERASE_LINE, sizeof(CR_ERASE_LINE));
		cons_len = 0;
		break;
	case CTRL('W'):
		while (cons_len && isasciispace(cons_buf[cons_len - 1]))
			backspace();
		while (cons_len && !isasciispace(cons_buf[cons_len - 1]))
			backspace();
		break;
	case CTRL('R'):
		_write(1, CTRL_R_CRLF, sizeof(CTRL_R_CRLF));
		_write(1, cons_buf, cons_len);
		break;
	case '\r':
	case '\n':
		_write(1, CRLF, sizeof(CRLF));
		handle_line();
		cons_len = 0;
		break;
	default:
		if (c < 0x20 || c >= 0x80)
			break;
		if (cons_len >= sizeof(cons_buf) - 1) {
			_write(1, BELL, sizeof(BELL));
			break;
		}
		cons_buf[cons_len++] = c;
		_write(1, &c, 1);
	}
	ad_lora_suspend_sleep(LORA_SUSPEND_CONSOLE, sec2osticks(2));
}

static void
uart_isr()
{
	HW_UART_INT	int_id;

	int_id = hw_uart_get_interrupt_id(HW_UART1);
	switch (int_id) {
	case HW_UART_INT_RECEIVED_AVAILABLE:
		while (hw_uart_is_data_ready(HW_UART1)) {
			hal_uart_rx(hw_uart_rxdata_getf(HW_UART1));
		}
		break;
	default:
		break;
	}
}

#else /* !CONSOLE_INPUT */

void
cons_rx(uint8_t c)
{
	(void)c;
}

#endif /* CONSOLE_INPUT */

static const uart_config	uart_cfg = {
	HW_UART_BAUDRATE_115200,
	HW_UART_DATABITS_8,
	HW_UART_PARITY_NONE,
	HW_UART_STOPBITS_1,
	0,
	0,
	1,
	HW_DMA_CHANNEL_1,
	HW_DMA_CHANNEL_0,
};

static inline void uart_enable_rx_int(void)
{
        NVIC_DisableIRQ(UART_IRQn);
        HW_UART_REG_SETF(HW_UART1, IER_DLH, ERBFI_dlh0, true);
        NVIC_EnableIRQ(UART_IRQn);
}

static void
uart_pin_init()
{
	/* Pull RX up momentarily; otherwise the board hangs if there's
	 * no UART adapter connected to it.  */
	hw_gpio_configure_pin(HW_CONSOLE_UART_RX_PORT,
	    HW_CONSOLE_UART_RX_PIN, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, 1);
	hw_gpio_set_pin_function(HW_CONSOLE_UART_TX_PORT,
	    HW_CONSOLE_UART_TX_PIN, HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_UART_TX);
#ifdef CONSOLE_INPUT
	hw_gpio_set_pin_function(HW_CONSOLE_UART_RX_PORT,
	    HW_CONSOLE_UART_RX_PIN, HW_GPIO_MODE_INPUT,  HW_GPIO_FUNC_UART_RX);
#endif
}

void
cons_init()
{
	uart_pin_init();
	hw_uart_init(HW_UART1, &uart_cfg);
#ifdef CONSOLE_INPUT
	hw_uart_set_isr(HW_UART1, uart_isr);
	uart_enable_rx_int();
#endif
}

void
cons_reinit()
{
	uart_pin_init();
	hw_uart_reinit(HW_UART1, &uart_cfg);
#ifdef CONSOLE_INPUT
	uart_enable_rx_int();
#endif
}

#else /* !CONFIG_CUSTOM_PRINT */

void	cons_init() {}
void	cons_reinit() {}
void	cons_rx(uint8_t c) { (void)c; }

#endif /* CONFIG_CUSTOM_PRINT */
