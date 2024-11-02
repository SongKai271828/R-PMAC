#ifndef __BSP_H
#define __BSP_H

#include "stm32f10x.h"

// define all fireware here for app_main to refer
#include "app_config.h"
#include "fw_delay.h"
#include "fw_afe_gpio.h"
#include "fw_plc_gpio.h"
#include "fw_plc_spi.h"
#include "fw_uart485.h"
#include "fw_led_gpio.h"
#include "fw_flash_spi.h"
#include "fw_rtc_iic.h"
#include "fw_timer.h"

void bsp_config(void);

#endif
