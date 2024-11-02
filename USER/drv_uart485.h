#ifndef __DRV_UART485_H
#define __DRV_UART485_H

#include "stm32f10x.h"

void restart_uart485(void);
u8 start_uart485(u32 tot);
void uart485_host_recvbyte(u8 data);
void uart485_node_recvbyte(u8 data);
void uart485_send(u8* buf, u8 len);

#endif
