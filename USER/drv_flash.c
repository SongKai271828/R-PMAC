#include "drv_flash.h"
#include "fw_flash_spi.h"

u8 fl_buf[4096];

void flash_write_buffer(u8* buf, u16 ind)
{
    int i;
    
	u32 addr;
    u32 sc_addr;
    
    addr = ind;
    addr = (addr << 6) & 0xFFFFF000;
    for (i = 0; i < 16; i++) {
        sc_addr = addr + i * 256;
        FW_FLASH_SPI_BufferRead(&fl_buf[i*256], sc_addr, 256);
    }
    FW_FLASH_SPI_SectorErase(addr);
    
    addr = ind;
    addr = (addr << 6) & 0x00000FFF;
    for (i = 0; i < 64; i++) {
        fl_buf[addr + i] = buf[i];
    }
    
    addr = ind;
    addr = (addr << 6) & 0xFFFFF000;
    for (i = 0; i < 16; i++) {
        sc_addr = addr + i * 256;
        FW_FLASH_SPI_BufferWrite(&fl_buf[i*256], sc_addr, 256);
    }
}

void flash_read_buffer(u8* buf, u16 ind)
{
    u32 addr;
    
    addr = ind;
    addr = (addr << 6) & 0xFFFFFFC0;
    FW_FLASH_SPI_BufferRead(buf, addr, 64);
}
