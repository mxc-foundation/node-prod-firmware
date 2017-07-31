#ifndef __GPS_H__
#define __GPS_H__

void		gps_init(void);
void		gps_prepare(void);
ostime_t	gps_data_ready(void);
int		gps_read(char *, int);

#endif /* __GPS_H__ */
