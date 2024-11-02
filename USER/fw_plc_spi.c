#include "fw_plc_spi.h"

#define DummyByte 0xFF

#define SPI_NSS_LOW() GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define SPI_NSS_HIGH() GPIO_SetBits(GPIOB, GPIO_Pin_12)

u8 SPI2_SendByte(u8 byte);

void FW_PLC_SPI_Config(void)
{
	SPI_InitTypeDef SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
    SPI_NSS_HIGH();
	
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);
	
    SPI_Cmd(SPI2, ENABLE);
}

u32 FW_PLC_SPI_Read32Bits(u8 cmd)
{
	u32 byte_0;
	u32 byte_1;
	u32 byte_2;
	u32 byte_3;
	u32 byte_cb;
	
	byte_0 = 0;
	byte_1 = 0;
	byte_2 = 0;
	byte_3 = 0;
	byte_cb = 0;
    
	SPI_NSS_LOW();
	
	SPI2_SendByte(cmd & 0x7F); // Set MSB to Low, as Read Operation
	
	byte_3 = SPI2_SendByte(DummyByte);
	byte_2 = SPI2_SendByte(DummyByte);
	byte_1 = SPI2_SendByte(DummyByte);
	byte_0 = SPI2_SendByte(DummyByte);
	
	SPI_NSS_HIGH();
    
	byte_cb = (byte_3 << 24) | (byte_2 << 16) | (byte_1 << 8) | byte_0;
	
	return byte_cb;
}

void FW_PLC_SPI_Write32Bits(u8 cmd, u32 data)
{
	SPI_NSS_LOW();
	
	SPI2_SendByte(cmd | 0x80); // Set MSB to High, as Write Operation
	
	SPI2_SendByte((data & 0xFF000000) >> 24);
	SPI2_SendByte((data & 0xFF0000) >> 16);
	SPI2_SendByte((data & 0xFF00) >> 8);
	SPI2_SendByte(data & 0xFF);
	
	SPI_NSS_HIGH();
}

u8 SPI2_SendByte(u8 byte)
{
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI2, byte);
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI2);
}
