#ifndef __DRV_FLASH_H
#define __DRV_FLASH_H

#include "stm32f10x.h"

void flash_read_buffer(u8* buf, u16 ind);
void flash_write_buffer(u8* buf, u16 ind);

#endif
