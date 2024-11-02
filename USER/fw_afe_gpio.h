#ifndef __FW_AFE_GPIO_H
#define __FW_AFE_GPIO_H

#include "stm32f10x.h"

void FW_AFE_GPIO_Config(void);
u8 FW_AFE_GPIO_Read(u8 addr);
void FW_AFE_GPIO_Write(u8 addr, u8 data);

#endif
