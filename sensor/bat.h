#ifndef __BAT_H__
#define __BAT_H__

#ifdef FEATURE_BATTERY

uint8_t	bat_level(void);

#else

#define bat_level()	0

#endif

#endif /* __BAT_H__ */
