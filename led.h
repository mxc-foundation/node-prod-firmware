#ifndef __LED_H__
#define __LED_H__

#define LED_OFF		0x0
#define LED_BLINK	0x1
#define LED_BREATH	0x2
#define LED_FUNC_MASK	0x3

#define LED_GREEN	(1 << 0)
#define LED_RED		(1 << 2)
#define LED_YELLOW	(LED_RED | LED_GREEN)
#define LED_ALL		(LED_RED | LED_GREEN)

void	led_init(void);
void	led_set_status(uint8_t s);

#define LED_ACTION(led, action)	((action) * (led))
#define LED_SET(led, action)	led_set_status(LED_ACTION(led, action))

#endif /* __LED_H__ */
