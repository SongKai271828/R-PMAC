#ifndef __FW_RTC_IIC_H
#define __FW_RTC_IIC_H

#include "stm32f10x.h"

void FW_RTC_IIC_Config(void);
u8 FW_RTC_IIC_WriteByte(u8 addr, u8 data);
u8 FW_RTC_IIC_ReadNBytes(u8 addr, u8* data, u8 len);

#endif
