#ifndef __FW_UART485_H
#define __FW_UART485_H

#include "stm32f10x.h"

typedef struct {
    u8 baudrate; // 2 bits
	u8 is8b;
	u8 parity; // 2 bits
} uart_config;

void FW_UART485_Config(u8 baudrate, u8 is8b, u8 parity);
void FW_UART485_SendByte(u8 data);

#endif
