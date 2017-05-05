#ifndef __AD_LORA_H__
#define __AD_LORA_H__

#define LORA_SUSPEND_NORESET	0 /* Tasks not calling ad_lora_allow_sleep() */
#define LORA_SUSPEND_LORA	1 /* LoRa main task */
#define LORA_SUSPENDS		2

void	ad_lora_init(void);
void	ad_lora_suspend_sleep(int id, ostime_t period);
void	ad_lora_allow_sleep(int id);

#endif /* __AD_LORA_H__ */
