#ifndef __LED_H__
#define __LED_H__

#define LED_STATE_IDLE		0x00
#define LED_STATE_BOOTING	0x01
#define LED_STATE_JOINING	0x02
#define LED_STATE_SENDING	0x03

void	led_init(void);
void	led_notify(uint8_t s);

#endif /* __LED_H__ */
