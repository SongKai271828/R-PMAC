#ifndef __FW_LED_GPIO_H
#define __FW_LED_GPIO_H

#include "stm32f10x.h"

void FW_LED_GPIO_Config(void);
void FW_LED_GPIO_Red(u8 ison);
void FW_LED_GPIO_Green(u8 ison);

#endif
