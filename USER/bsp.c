#include "bsp.h"

void bsp_config(void)
{
	// flash access for initial values
	FW_FLASH_SPI_Config();
	
	// stable fpga related interface asap
	FW_PLC_GPIO_Config();
	FW_PLC_SPI_Config();
	FW_AFE_GPIO_Config();
	
	FW_LED_GPIO_Config();
	FW_RTC_IIC_Config();
	FW_Timer_Config();
	PTE_Timer_Config();
	
	// wait for fpga power-up
	StartIdle(500); // 500 ms timeout
	while (CheckIdle());
	ClearIdle();
	
	FW_PLC_GPIO_Reset(); // reset phy after fpga power-up
}
