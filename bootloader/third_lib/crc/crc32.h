#ifndef __CRC32_H
#define __CRC32_H


#include <stdint.h>


void     crc32_init(void);
uint32_t crc32_update(uint32_t crc, uint8_t *data, uint32_t len);


#endif /*__CRC32_H */
