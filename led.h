#ifndef __LED_H__
#define __LED_H__

#define LED_BLINK_RED	0
#define LED_BLINK_GREEN	1

void	led_init(void);
void	led_set_status(uint8_t s);

#endif /* __LED_H__ */
