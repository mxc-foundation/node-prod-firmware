#ifndef __SENSOR_H__
#define __SENSOR_H__

void		sensor_init(void);
ostime_t	sensor_prepare(void);
size_t		sensor_get_data(char *buf, int len);

#endif /* __SENSOR_H__ */
