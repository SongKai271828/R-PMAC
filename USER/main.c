#include "stm32f10x.h"

extern void app_main(void);
extern void usart485_isr(u8 data);
extern void plc_isr(void);
extern void tim_isr(void);
extern void time_update_isr(void);

int main(void)
{
    app_main();
	
	while (1);
}

void USART1_IRQHandler(void)
{
    u8 data;
	
    if (USART_GetITStatus(USART1, USART_IT_RXNE)) {
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		
		data = USART_ReceiveData(USART1);
        usart485_isr(data);
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line8) != RESET) {
		EXTI_ClearITPendingBit(EXTI_Line8);
        
        plc_isr();
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_FLAG_Update);
        
        tim_isr();
  }
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        time_update_isr();
        TIM_ClearITPendingBit(TIM4, TIM_FLAG_Update);
    }
}
