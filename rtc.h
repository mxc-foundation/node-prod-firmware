#ifndef __RTC_H__
#define __RTC_H__

extern uint32_t	lastfinetime, intcnt; // XXX

void		rtc_init(void);
uint32_t	rtc_get(void);
uint32_t	rtc_get_fromISR(void);

#endif /* __RTC_H__ */
