#include "drv_uart485.h"

#include "app_config.h"
#include "fw_delay.h"
#include "fw_uart485.h"

u8 uart485_start; // recv start indication
u8 uart485_recvd; // recv end indication

u8 uart485_index; // index for recvd byte, from 0
u8 uart485_len; // length of recvd bytes

u8 uart485_enFE;
u8 uart485_nohdr;
u8 uart485_lenpos;
u8 uart485_lenmult;
u8 uart485_lenadj;

u8 uart485_buf[UARTBufferLength];

void restart_uart485(void)
{
	uart485_start = 0;
	uart485_recvd = 0;
	
	uart485_len = 0;
	uart485_index = 0;
}

u8 start_uart485(u32 tot)
{
	// process uart data
	StartIdle(tot);
	while (CheckIdle() && uart485_recvd == 0);
	ClearIdle();
	
	// timeout and not recvd, restart processing uart bytes
	// invalid command
	if (uart485_recvd == 0) {
		restart_uart485();
		return 0;
	}
	
	return uart485_len;
}

void uart485_host_recvbyte(u8 data)
{
	u8 i;
	u8 uart_crc;
	
	// ignore if recvd bytes not processed
	if (uart485_recvd == 1)
        return;
	
	// ignore initial 0xFE(s)
	if (data == 0xFE && uart485_index == 0)
		return;
	
	// start saving to buffer if header 0x68 appear
	if (data == 0x68 && uart485_index == 0) {
		uart485_start = 1;
		uart485_len = 0;
	}
	
	// save length information on index == 1
	if (uart485_index == 1) {
		if (data == 0 || data > UARTBufferLength) {
			restart_uart485();
			return;
		}
		
        uart485_len = data;
	}
	
	uart485_buf[uart485_index] = data;
	
	// process the last byte
	if (uart485_index > uart485_len && uart485_len != 0) {
		// crc checking
		// ignore if end with 0xFE
        uart_crc = 0;
		for (i = 0; i < uart485_len; i++)
			uart_crc += uart485_buf[1 + i];
        if (uart_crc != data && data != 0xFE) {
			restart_uart485();
			return;
		}
		
        uart485_recvd = 1;
    }
    else
        uart485_index++;
}

void uart485_node_recvbyte(u8 data)
{
	// ignore if recvd bytes not processed
	if (uart485_recvd == 1)
        return;
	
	// ignore initial 0xFE(s), only if no xFE headers
	if (data == 0xFE && uart485_index == 0 && uart485_enFE == 1)
		return;
	
	// start saving to buffer if header 0x68 appear, or no header required
	if ((data == 0x68 || uart485_nohdr == 1) && uart485_index == 0) {
		uart485_start = 1;
		uart485_len = 0;
	}
	
	// save length information by length position
	if (uart485_index == uart485_lenpos) {
		if (data == 0 || data > UARTBufferLength) {
			restart_uart485();
			return;
		}
		
        uart485_len = uart485_lenmult * data + uart485_lenadj;
	}
	
	uart485_buf[uart485_index] = data;
	
	// process the last byte
	if (uart485_index >= (uart485_len - 1) && uart485_len != 0) {
        uart485_recvd = 1;
    }
    else
        uart485_index++;
}

void uart485_send(u8* buf, u8 len)
{
    u8 i;
	
	if (len == 0 || len > UARTBufferLength)
		return;
    
    for (i = 0; i < len; i++)
        FW_UART485_SendByte(buf[i]);
}
