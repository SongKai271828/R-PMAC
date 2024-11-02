#ifndef __DRV_RTC_H
#define __DRV_RTC_H

#include "stm32f10x.h"

void rtc_setclock(u8* datetime);
void rtc_current(u8* datetime);

#endif
