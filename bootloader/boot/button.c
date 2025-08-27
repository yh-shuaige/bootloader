#include "main.h"


void bl_button_init(void)
{
    GPIO_InitTypeDef GPIO_Initstruct;
    GPIO_Initstruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Initstruct.GPIO_OType = GPIO_OType_PP;
    GPIO_Initstruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_Initstruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Initstruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOE,&GPIO_Initstruct);


}


bool bl_button_pressed(void)
{
    return GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_0) == Bit_RESET;
}
