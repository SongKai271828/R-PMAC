#ifndef __FW_PLC_SPI_H
#define __FW_PLC_SPI_H

#include "stm32f10x.h"

void FW_PLC_SPI_Config(void);
u32 FW_PLC_SPI_Read32Bits(u8 cmd);
void FW_PLC_SPI_Write32Bits(u8 cmd, u32 data);

#endif
