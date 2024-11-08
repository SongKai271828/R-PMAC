#include "fw_uart485.h"

void FW_UART485_Config(u8 baudrate, u8 is8b, u8 parity)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	switch (baudrate) {
		case 0:
			USART_InitStructure.USART_BaudRate = 1200; break;
		case 1:
			USART_InitStructure.USART_BaudRate = 2400; break;
		case 2:
			USART_InitStructure.USART_BaudRate = 4800; break;
		default: break;
	}
	
	if (is8b == 1)
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	else
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	
	switch (parity) {
		case 0:
			USART_InitStructure.USART_Parity = USART_Parity_No; break;
		case 1:
			USART_InitStructure.USART_Parity = USART_Parity_Even; break;
		case 2:
			USART_InitStructure.USART_Parity = USART_Parity_Odd; break;
		default: break;
	}
	
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
    
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    USART_Cmd(USART1, ENABLE);
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void FW_UART485_SendByte(u8 data)
{
    USART_SendData(USART1, data);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}
