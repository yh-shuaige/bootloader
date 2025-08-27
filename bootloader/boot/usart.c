#include "main.h"

static bl_uart_recv_cb_t bl_uart_recv_cb;

void usart1_init(void)
{    
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

void bl_usart_init(void)
{
    GPIO_InitTypeDef GPIO_Initstruct;
    USART_InitTypeDef USART_Initstruct;
    NVIC_InitTypeDef NVIC_INitstruct;


    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);

    GPIO_Initstruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Initstruct.GPIO_OType = GPIO_OType_PP;
    GPIO_Initstruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Initstruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Initstruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA,&GPIO_Initstruct);

    USART_Initstruct.USART_BaudRate = 115200;
    USART_Initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Initstruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Initstruct.USART_Parity = USART_Parity_No;
    USART_Initstruct.USART_StopBits = USART_StopBits_1;
    USART_Initstruct.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2,&USART_Initstruct);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);


    NVIC_INitstruct.NVIC_IRQChannel = USART2_IRQn;
    NVIC_INitstruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_INitstruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_INitstruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_INitstruct);

    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART2,ENABLE);

}

void bl_usart_deinit(void)
{
    USART_DeInit(USART2);
    USART_Cmd(USART2,DISABLE);
}

void bl_usart_write(uint8_t *data,uint32_t len)
{
    USART_ClearFlag(USART2,USART_FLAG_TC);

    for(uint32_t i = 0;i < len;i++)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, data[i]);
    }
    while(USART_GetFlagStatus(USART2,USART_FLAG_TC) == RESET);
}


void bl_uart_recv_register(bl_uart_recv_cb_t callback)
{
    bl_uart_recv_cb = callback;
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t data = USART_ReceiveData(USART2);
        if (bl_uart_recv_cb)
        {
            bl_uart_recv_cb(&data, 1);
        }
    }
}


void bl_uart_deinit(void)
{
    USART_Cmd(USART2, DISABLE);
    USART_DeInit(USART2);
}


/// @brief 重定向fputc
int fputc(int ch, FILE *f)
{
    while((USART1->SR & 0X40) == 0); //ѭ������,ֱ���������
    USART1->DR = (uint8_t)ch;
    return ch;
}
