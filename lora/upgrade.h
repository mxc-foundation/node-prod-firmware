#ifndef __UPGRADE_H__
#define __UPGRADE_H__

#define UPGRADE_DEFAULT	0x81

void	upgrade_reboot(uint8_t flags);
void	upgrade_init(void);

#endif /* __UPGRADE_H__ */
