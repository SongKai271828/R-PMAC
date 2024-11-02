#include "fw_delay.h"

u32 delay_rpt;
u32 delay_mod;

void StartIdle(u32 delay)
{
	delay_rpt = delay / 1800;
	delay_mod = delay % 1800;
	
	//SysTick->LOAD = 9000 * delay; // delay ms, maximum 2^24/9000 > 1.8s
	if (delay_rpt == 0)
		SysTick->LOAD = 9000 * delay;
	else
		SysTick->LOAD = 9000 * 1800;
	SysTick->VAL = 0x00;
	SysTick->CTRL = 0x01;
}

void StartIdle_us(u32 delay)
{
	delay_rpt = delay / 1800000;
	delay_mod = delay % 1800000;
	
	//SysTick->LOAD = 9000 * delay /1000; // delay us, maximum 2^24/9000 > 1.8s
	if (delay_rpt == 0)
		SysTick->LOAD = 9000 * delay / 1000;
	else
		SysTick->LOAD = 9000 * 1800 / 1000;
	SysTick->VAL = 0x00;
	SysTick->CTRL = 0x01;
}

u8 CheckIdle(void)
{
	u32 sys_tic;
	
	sys_tic = SysTick->CTRL;
	if ((sys_tic & 0x01) && (!(sys_tic & (1 << 16))))
		return 1;
	else if (delay_rpt > 1) {
		SysTick->CTRL = 0x00;
		SysTick->VAL = 0x00;
		
		SysTick->LOAD = 9000 * 1800;
		SysTick->VAL = 0x00;
		SysTick->CTRL = 0x01;
		
		delay_rpt--;
		return 1;
	}
	else if (delay_rpt == 1) {
		SysTick->CTRL = 0x00;
		SysTick->VAL = 0x00;
		
		SysTick->LOAD = 9000 * delay_mod;
		SysTick->VAL = 0x00;
		SysTick->CTRL = 0x01;
		
		delay_rpt--;
		return 1;
	}
	else
		return 0;
}

void ClearIdle(void)
{
    SysTick->CTRL = 0x00;
    SysTick->VAL = 0x00;
}

void DelayByNOP(__IO u32 delay)
{
	for(; delay != 0; delay--);
}
