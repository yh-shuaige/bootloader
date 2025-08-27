#define LOG_TAG    "flash"

#include "main.h"


#define flash_base_addr 0x08000000


typedef struct
{
    uint32_t sector;
    uint32_t size;
    /* data */
}sector_desc_t;

static const sector_desc_t sector_descs[] =
{
    {FLASH_Sector_0, 16 * 1024},
    {FLASH_Sector_1, 16 * 1024},
    {FLASH_Sector_2, 16 * 1024},
    {FLASH_Sector_3, 16 * 1024},
    {FLASH_Sector_4, 64 * 1024},
    {FLASH_Sector_5, 128 * 1024},
    {FLASH_Sector_6, 128 * 1024},
    {FLASH_Sector_7, 128 * 1024},
    {FLASH_Sector_8, 128 * 1024},
    {FLASH_Sector_9, 128 * 1024},
    {FLASH_Sector_10, 128 * 1024},
    {FLASH_Sector_11, 128 * 1024},
};

void bl_norflash_unlock(void)
{
    log_i("flash unlock");
    FLASH_Unlock();
}

void bl_norflash_lock(void)
{
    log_i("flash lock");
    FLASH_Lock();
}

void bl_norflash_erase(uint32_t address,uint32_t size)
{
    uint32_t addr = flash_base_addr;

    for(uint32_t i = 0;i < sizeof(sector_descs) / sizeof(sector_desc_t);i++)
    {
        if(addr >= address && addr < address + size)
        {
            log_i("erase sector %u, addr: 0x%08x, size: %u", i, addr, sector_descs[i].size);
            if(FLASH_EraseSector(sector_descs[i].sector,VoltageRange_3) != FLASH_COMPLETE)
            {
                log_w("erase sector %u failed", i);
            }
        }
        addr += sector_descs[i].size;
    }
}



void bl_norflash_write(uint32_t address,uint8_t *data,uint32_t size)
{
    for(uint32_t i = 0;i < size;i += 4)
    {
        if(FLASH_ProgramWord(address + i,*(uint32_t *)(data + i)) != FLASH_COMPLETE)
        {
            log_w("write data failed, addr: 0x%08x", address + i);
        }
    }
}

bool bl_norflash_write_backup(void)
{
    uint32_t app_addr = FLASH_APP_ADDRESS;
    /*擦除备份区域flash*/
     log_i("Erasing backup area (0x%08X - 0x%08X)", 
          FLASH_BACKUP_ADDRESS, 
          FLASH_BACKUP_ADDRESS + FLASH_BACKUP_SIZE);
    bl_norflash_erase(FLASH_BACKUP_ADDRESS,FLASH_BACKUP_SIZE);
    /*将app中flash写入备份flash中*/
    for(uint32_t offset = 0;offset <= FLASH_APP_SIZE;offset += 4)
    {
        uint32_t app_data = *(uint32_t *)(offset + FLASH_APP_ADDRESS);
        if(FLASH_ProgramWord(FLASH_BACKUP_ADDRESS + offset,app_data) != FLASH_COMPLETE)
        {
            log_w("write data failed, addr: 0x%08x", FLASH_BACKUP_ADDRESS + offset);
        }
    }
    /*验证备份是否成功*/
    log_i("Verifying backup...");
    // 1. 计算应用程序区域的 CRC
    uint32_t app_crc = crc32_update(0, (uint8_t *)FLASH_APP_ADDRESS, FLASH_APP_SIZE);
    log_d("App CRC: 0x%08X", app_crc);
    // 2. 计算备份区域的 CRC
    uint32_t backup_crc = crc32_update(0, (uint8_t *)FLASH_BACKUP_ADDRESS, FLASH_APP_SIZE);
    log_d("Backup CRC: 0x%08X", backup_crc);
    // 3. 比较两个 CRC 值
    if(app_crc == backup_crc) 
    {
    log_i("Backup verified successfully (CRC: 0x%08X)", app_crc);
    FLASH_ProgramWord(FLASH_BACKUP_CRC_ADDRESS,app_crc);
    return true;
    } 
    else 
    {
    log_w("Backup verification failed! App CRC: 0x%08X, Backup CRC: 0x%08X", 
          app_crc, backup_crc);
    return false;
    }
}