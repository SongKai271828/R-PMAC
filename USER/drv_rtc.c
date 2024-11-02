#include "drv_rtc.h"

#include "fw_rtc_iic.h"

void rtc_setclock(u8* dttm)
{
	// set year
	FW_RTC_IIC_WriteByte(0x06, *dttm);
	dttm++;
	// set month
	FW_RTC_IIC_WriteByte(0x05, *dttm);
	dttm++;
	// set day
	FW_RTC_IIC_WriteByte(0x04, *dttm);
	dttm++;
	// set weekday
	FW_RTC_IIC_WriteByte(0x03, *dttm);
	dttm++;
	// set hours
	FW_RTC_IIC_WriteByte(0x02, *dttm);
	dttm++;
	// set minutes
	FW_RTC_IIC_WriteByte(0x01, *dttm);
	dttm++;
	// set seconds
	FW_RTC_IIC_WriteByte(0x00, *dttm);
}

void rtc_current(u8* dttm)
{
	u8 i;
	u8 buf[7];
	
	FW_RTC_IIC_ReadNBytes(0x00, buf, 7);
	
	for (i = 0; i < 7; i++) {
		*dttm = buf[6-i];
		dttm++;
	}
}
