#ifndef __LED_H__
#define __LED_H__

#define LED_OFF		0x0
#define LED_BLINK	0x1
#define LED_BREATH	0x2
#define LED_FUNC_MASK	0x3

#define LED_GREEN	0
#define LED_RED		2

void	led_init(void);
void	led_set(uint8_t led, uint8_t action);

#endif /* __LED_H__ */
