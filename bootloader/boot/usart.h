#ifndef __USART_H
#define __USART_H

#include "stdint.h"


typedef void (*bl_uart_recv_cb_t)(uint8_t *data, uint32_t len);


void bl_usart_init(void);
void usart1_init(void);

void bl_usart_deinit(void);

void bl_usart_write(uint8_t *data,uint32_t len);


void bl_uart_recv_register(bl_uart_recv_cb_t callback);
void bl_uart_deinit(void);

#endif /* __MAIN_H */

