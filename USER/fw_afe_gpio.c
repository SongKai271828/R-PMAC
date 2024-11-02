#include "fw_afe_gpio.h"

#define DummyByte 0xFF

#define GPIO_NSS_LOW() GPIO_ResetBits(GPIOC, GPIO_Pin_0)
#define GPIO_NSS_HIGH() GPIO_SetBits(GPIOC, GPIO_Pin_0)

u8 GPIO_SendByte(u8 byte);

void FW_AFE_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // RESET
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; // NSS
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; // SCK
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; // MOSI
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; // MISO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOC, GPIO_Pin_9); // RESET
  
    GPIO_SetBits(GPIOC, GPIO_Pin_0); // NSS
    GPIO_SetBits(GPIOC, GPIO_Pin_1); // SCK
    GPIO_SetBits(GPIOC, GPIO_Pin_3); // MOSI
}

u8 FW_AFE_GPIO_Read(u8 addr)
{
	u8 data;
	
	GPIO_NSS_LOW();
	GPIO_SendByte(addr | 0x80);
	data = GPIO_SendByte(DummyByte);
	GPIO_NSS_HIGH();
	
	return data;
	
}

void FW_AFE_GPIO_Write(u8 addr, u8 data)
{
	GPIO_NSS_LOW();
	GPIO_SendByte(addr & 0x7F);
	GPIO_SendByte(data);
	GPIO_NSS_HIGH();
}

u8 GPIO_SendByte(u8 byte)
{
    u8 i;
    u8 tmp = 0x00;
	
    unsigned char SDI; 
	
    for (i = 0; i < 8; i++)
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_1); // SCK
        if (byte & 0x80)
            GPIO_SetBits(GPIOC, GPIO_Pin_3); // MOSI
        else
            GPIO_ResetBits(GPIOC, GPIO_Pin_3); // MOSI
        byte <<= 1;  
        GPIO_SetBits(GPIOC, GPIO_Pin_1); // SCK
        SDI = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2); // MISO
        tmp <<= 1;
        if (SDI)
            tmp++;
    }
    
    return tmp;
}
