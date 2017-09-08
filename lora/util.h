#ifndef __LORA_UTIL_H__
#define __LORA_UTIL_H__

#define BARRIER()	asm volatile ("":::"memory")
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

long long	strtonum(const char *, long long, long long, const char **);

#endif /* __LORA_UTIL_H__ */
