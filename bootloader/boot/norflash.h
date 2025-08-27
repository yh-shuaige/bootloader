#ifndef __BL_NORFLASH_H
#define __BL_NORFLASH_H


#include <stdint.h>


void bl_norflash_lock(void);
void bl_norflash_unlock(void);
void bl_norflash_erase(uint32_t address, uint32_t size);
void bl_norflash_write(uint32_t address, uint8_t *data, uint32_t size);

bool bl_norflash_write_backup(void);
#endif /* __BL_NORFLASH_H */

