#include "fw_led_gpio.h"

u8 rstr_en;

void FW_LED_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
	
    GPIO_SetBits(GPIOC, GPIO_Pin_5);
	
	// read first
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	rstr_en = 0; // default not restoring
	if ((GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) & 0x01) == 0x00) {
		rstr_en = 1;
		return;
	}
    
	// then switch to output
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_SetBits(GPIOC, GPIO_Pin_4);
}

void FW_LED_GPIO_Red(u8 ison)
{
	if (ison != 0)
		GPIO_ResetBits(GPIOC, GPIO_Pin_5);
	else
		GPIO_SetBits(GPIOC, GPIO_Pin_5);
}

void FW_LED_GPIO_Green(u8 ison)
{
	if (ison != 0)
		GPIO_ResetBits(GPIOC, GPIO_Pin_4);
	else
		GPIO_SetBits(GPIOC, GPIO_Pin_4);
}
