#ifndef __FW_FLASH_SPI_H
#define __FW_FLASH_SPI_H

#include "stm32f10x.h"

void FW_FLASH_SPI_Config(void);
u32 FW_FLASH_SPI_ID(void);
void FW_FLASH_SPI_SectorErase(u32 saddr);
void FW_FLASH_SPI_BufferWrite(u8* buf, u32 saddr, u16 len);
void FW_FLASH_SPI_BufferRead(u8* buf, u32 saddr, u16 len);

#endif
