#ifndef __FW_DELAY_H
#define __FW_DELAY_H

#include "stm32f10x.h"

void StartIdle(u32 delay);
void StartIdle_us(u32 delay);
u8 CheckIdle(void);
void ClearIdle(void);

void DelayByNOP(__IO u32 delay);

#endif

