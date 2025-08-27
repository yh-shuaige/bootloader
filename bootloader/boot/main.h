#ifndef __MAIN_H
#define __MAIN_H


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "low_level.h"
#include "stm32f4xx.h"
#include "usart.h"
#include "utils.h"
#include "led.h"
#include "button.h"
#include "elog.h"
#include "ringbuffer8.h"
#include "crc32.h"
#include "flash_layout.h"
#include "norflash.h"
#include "arginfo.h"
#define TRY_CALL(f, ...) \
if (f != NULL)           \
    f(__VA_ARGS__)

#define CHECK_RET(x)    \
do {                    \
    if (!(x)) {	        \
        return;         \
    }                   \
} while (0)

#define CHECK_RETX(x,r) \
do {                    \
    if (!(x)) {	        \
        return (r);     \
    }                   \
} while (0)

#define CHECK_GO(x,e)   \
do {                    \
    if (!(x)) {	        \
        goto e;         \
    }                   \
} while (0)



#endif /* __MAIN_H */




