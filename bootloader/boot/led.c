#include "main.h"

static bool led_state;

void bl_led_init(void)
{
    GPIO_InitTypeDef GPIO_Initstrtuct;
    GPIO_Initstrtuct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Initstrtuct.GPIO_OType = GPIO_OType_PP;
    GPIO_Initstrtuct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_Initstrtuct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Initstrtuct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE,&GPIO_Initstrtuct);

}

void led_set(bool on)
{
    led_state = on;

    GPIO_WriteBit(GPIOE,GPIO_Pin_5,led_state ? Bit_RESET : Bit_SET);
    GPIO_WriteBit(GPIOE,GPIO_Pin_6,led_state ? Bit_RESET : Bit_SET);
}

void led_on(void)
{
    led_state = true;
    led_set(true);
}

void led_off(void)
{
    led_state = false;
    led_set(false);
}

void led_toggle(void)
{
    led_state = !led_state;
    led_set(led_state);
}