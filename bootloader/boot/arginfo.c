#include <stdbool.h>
#include <stdint.h>
#include "flash_layout.h"
#include "arginfo.h"


#define ARGINFO_MAGIC   0x1A2B3C4D


bool bl_arginfo_read(uint32_t *size, uint32_t *crc)
{
    uint32_t *arginfo = (uint32_t *)FLASH_ARG_ADDRESS;

    if (arginfo[0] != ARGINFO_MAGIC)
    {
        return false;
    }
    if (size)
    {
        *size = arginfo[1];
    }
    if (crc)
    {
        *crc = arginfo[2];
    }

    return true;
}
