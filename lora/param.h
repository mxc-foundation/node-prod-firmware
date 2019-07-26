#ifndef __PARAM_H__
#define __PARAM_H__

#define PARAM_DEV_EUI		    0
#define PARAM_APP_EUI		    1
#define PARAM_DEV_KEY		    2
#define PARAM_SENSOR_PERIOD	3
#define PARAM_MIN_SF		    4
#define PARAM_LORA_REGION	  5
#define PARAM_SUOTA		      6

#define PARAM_MAX_LEN	16	/* sizeof(devkey) */

void	param_init(void);
int	param_get(int idx, uint8_t *data, uint8_t len);
int	param_set(int idx, uint8_t *data, uint8_t len);

#endif /* __PARAM_H__ */
