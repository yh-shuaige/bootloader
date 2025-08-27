#include "crc32.h"


#define CRC32_TABLE_SIZE    0x100


static uint8_t inited = 0;
static uint32_t crc32_table[CRC32_TABLE_SIZE];


static uint32_t crc32_for_byte(uint32_t r)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        r = (r & 1 ? 0 : (uint32_t)0xedb88320) ^ r >> 1;
    }

    return r ^ (uint32_t)0xff000000;
}

void crc32_init(void)
{
    for (uint32_t i = 0; i < CRC32_TABLE_SIZE; ++i)
    {
        crc32_table[i] = crc32_for_byte(i);
    }

    inited = 1;
}

uint32_t crc32_update(uint32_t crc, uint8_t *data, uint32_t len)
{
    if (!inited)
    {
        crc32_init();
    }

    for (uint32_t i = 0; i < len; i++)
    {
        crc = crc32_table[(uint8_t)crc ^ ((uint8_t *)data)[i]] ^ crc >> 8;
    }

    return crc;
}
