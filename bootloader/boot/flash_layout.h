#ifndef __BL_FLASH_LAYOUT_H
#define __BL_FLASH_LAYOUT_H


#define FLASH_BOOT_ADDRESS          0x08000000
#define FLASH_BOOT_SIZE             48 * 1024

#define FLASH_ARG_ADDRESS           0x0800C000
#define FLASH_ARG_SIZE              16 * 1024

#define FLASH_APP_ADDRESS           0x08010000
#define FLASH_APP_SIZE              192 * 1024


#define FLASH_BACKUP_ADDRESS        0x08040000
#define FLASH_BACKUP_SIZE           192 * 1024


#define FLASH_BACKUP_CRC_ADDRESS    0x0800C800

#endif /* __BL_FLASH_LAYOUT_H */
