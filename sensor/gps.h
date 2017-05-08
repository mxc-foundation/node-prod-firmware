#ifndef __GPS_H__
#define __GPS_H__

void		gps_init(void);
ostime_t	gps_prepare(void);
int		gps_read(char *buf, int len);

#endif /* __GPS_H__ */
