#include "main.h"

static volatile uint32_t ticks;


void bl_delay_init(void)
{
    SysTick_Config(SystemCoreClock / 1000);
}

void bl_delay_ms(uint32_t ms)
{
    uint32_t start = ticks;
    while (ticks - start < ms);
}

uint32_t bl_now(void)
{
    return ticks;
}

void SysTick_Handler(void)
{
    ticks++;
}
