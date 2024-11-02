#include "fw_flash_spi.h"

#define FL_PageSize 256

#define FL_JedecID 0x9F
#define FL_WriteEnable 0x06
#define FL_ReadStatus 0x05
#define FL_SectorErase 0x20
#define FL_PageProgram 0x02 
#define FL_ReadData 0x03 

#define WIPFlag 0x01

#define DummyByte 0xFF

#define SPI_NSS_LOW() GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define SPI_NSS_HIGH() GPIO_SetBits(GPIOA, GPIO_Pin_4)

u8 SPI1_SendByte(u8 byte);

void FW_FLASH_SPI_Config(void)
{
    SPI_InitTypeDef SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_NSS_HIGH();

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; //*
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}

void FW_FLASH_SPI_WriteEnable(void)
{
    SPI_NSS_LOW();
    
	SPI1_SendByte(FL_WriteEnable);
	
    SPI_NSS_HIGH();
}

void FW_FLASH_SPI_HoldTillWriteEnd(void)
{
	u8 status;
	
	SPI_NSS_LOW();
	
	SPI1_SendByte(FL_ReadStatus);
	do {
		status = SPI1_SendByte(DummyByte);
	} while ((status & WIPFlag) == SET);
	
	SPI_NSS_HIGH();
}

u32 FW_FLASH_SPI_ID(void)
{
	u32 byte_0;
	u32 byte_1;
	u32 byte_2;
	u32 byte_cb;
	
	byte_0 = 0;
	byte_1 = 0;
	byte_2 = 0;
	byte_cb = 0;

	SPI_NSS_LOW();
	
	SPI1_SendByte(FL_JedecID);
	byte_2 = SPI1_SendByte(DummyByte);
	byte_1 = SPI1_SendByte(DummyByte);
	byte_0 = SPI1_SendByte(DummyByte);
	
	SPI_NSS_HIGH();

	byte_cb = (byte_2 << 16) | (byte_1 << 8) | byte_0;
	
	return byte_cb;
}

void FW_FLASH_SPI_SectorErase(u32 saddr)
{
    FW_FLASH_SPI_WriteEnable();
    FW_FLASH_SPI_HoldTillWriteEnd();
    
    SPI_NSS_LOW();
    
	SPI1_SendByte(FL_SectorErase);
    SPI1_SendByte((saddr & 0xFF0000) >> 16);
    SPI1_SendByte((saddr & 0xFF00) >> 8);
    SPI1_SendByte(saddr & 0xFF);
    
	SPI_NSS_HIGH();
	
    FW_FLASH_SPI_HoldTillWriteEnd();
}

void FW_FLASH_SPI_PageWrite(u8* buf, u32 paddr, u16 len)
{
    FW_FLASH_SPI_WriteEnable();

    SPI_NSS_LOW();
	
    SPI1_SendByte(FL_PageProgram);
    SPI1_SendByte((paddr & 0xFF0000) >> 16);
    SPI1_SendByte((paddr & 0xFF00) >> 8);
    SPI1_SendByte(paddr & 0xFF);

    if (len > FL_PageSize)
        len = FL_PageSize;

    while (len--) {
        SPI1_SendByte(*buf);
        buf++;
    }
	
    SPI_NSS_HIGH();

    FW_FLASH_SPI_HoldTillWriteEnd();
}

void FW_FLASH_SPI_BufferWrite(u8* buf, u32 addr, u16 len)
{
	u8 addr_lc;
	u8 cnt;
	u8 psize;
	u8 pres;
	u8 tmp;

    addr_lc = addr % FL_PageSize;
    cnt = FL_PageSize - addr_lc;
    psize =  len / FL_PageSize;
    pres = len % FL_PageSize;

    if (addr_lc == 0) {
        if (psize == 0) {
            FW_FLASH_SPI_PageWrite(buf, addr, len);
        }
        else {
            while (psize--) {
                FW_FLASH_SPI_PageWrite(buf, addr, FL_PageSize);
                addr += FL_PageSize;
                buf += FL_PageSize;
            }

            FW_FLASH_SPI_PageWrite(buf, addr, pres);
        }
    }
    else {
        if (psize == 0) {
            if (pres > cnt) {
                tmp = pres - cnt;

                FW_FLASH_SPI_PageWrite(buf, addr, cnt);
                addr += cnt;
                buf += cnt;

                FW_FLASH_SPI_PageWrite(buf, addr, tmp);
            }
            else {
                FW_FLASH_SPI_PageWrite(buf, addr, len);
            }
        }
        else  {
            len -= cnt;
            psize =  len / FL_PageSize;
            pres = len % FL_PageSize;

            FW_FLASH_SPI_PageWrite(buf, addr, cnt);
            addr += cnt;
            buf += cnt;

            while (psize--) {
                FW_FLASH_SPI_PageWrite(buf, addr, FL_PageSize);
                addr += FL_PageSize;
                buf += FL_PageSize;
            }

            if (pres != 0) {
                FW_FLASH_SPI_PageWrite(buf, addr, pres);
            }
        }
    }
}

void FW_FLASH_SPI_BufferRead(u8* buf, u32 addr, u16 len)
{
    SPI_NSS_LOW();
    
	SPI1_SendByte(FL_ReadData);
    SPI1_SendByte((addr & 0xFF0000) >> 16);
    SPI1_SendByte((addr& 0xFF00) >> 8);
    SPI1_SendByte(addr & 0xFF);

    while (len--) {
        *buf = SPI1_SendByte(DummyByte);
        buf++;
    }
	
    SPI_NSS_HIGH();
}

u8 SPI1_SendByte(u8 byte)
{
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1, byte);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(SPI1);
}
