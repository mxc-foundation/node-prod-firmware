#ifndef __ACCEL_H__
#define __ACCEL_H__

#ifdef FEATURE_SENSOR_GPS_ACCEL

void		accel_init(void);
int		accel_status(void);

#else

#define accel_init()
#define accel_status()	1

#endif

#endif /* __ACCEL_H__ */
