#include "fw_rtc_iic.h"

#define I2C_ADDRESS	0x64

#define IIC_SCL_SET() GPIO_SetBits(GPIOB, GPIO_Pin_10)
#define IIC_SCL_RESET() GPIO_ResetBits(GPIOB, GPIO_Pin_10)
#define IIC_SDA_SET() GPIO_SetBits(GPIOB, GPIO_Pin_11)
#define IIC_SDA_RESET() GPIO_ResetBits(GPIOB, GPIO_Pin_11)

#define IIC_SCL_IS_HIGH() (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10) == 1)
#define IIC_SDA_IS_HIGH() (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 1)

void IIC_Start(void);
void IIC_Restart(void);
void IIC_Stop(void);
u8 IIC_RecvACK(void);
void IIC_SendByte(u8 byte);
u8 IIC_RecvByte(void);
void IIC_ACK(void);
void IIC_NACK(void);
void IIC_Delay(void);

void FW_RTC_IIC_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	IIC_SCL_SET();
	IIC_SDA_SET();
}

u8 FW_RTC_IIC_WriteByte(u8 addr, u8 data)
{
	IIC_Start();
	IIC_SendByte((u8)I2C_ADDRESS);
	if (IIC_RecvACK() != 0) {
			IIC_Stop();
			return 1; // error
	}
	IIC_SendByte((addr << 4) & 0xF0);
	if (IIC_RecvACK() != 0) {
			IIC_Stop();
			return 1; // error
	}
	IIC_SendByte(data);
	if (IIC_RecvACK() != 0) {
			IIC_Stop();
			return 1; // error
	}
	IIC_Stop();
	
	return 0; // success
}

u8 FW_RTC_IIC_ReadNBytes(u8 addr, u8* data, u8 len)
{
	u8 i;
	
	IIC_Start();
	IIC_SendByte((u8)I2C_ADDRESS);
	if (IIC_RecvACK() != 0) {
		IIC_Stop();
		return 1; // error
	}
	IIC_SendByte(addr);
	if (IIC_RecvACK() != 0) {
		IIC_Stop();
		return 1; // error
	}

	IIC_Restart();
	IIC_SendByte((u8)(I2C_ADDRESS | 1));
	if (IIC_RecvACK() != 0) {
		IIC_Stop();
		return 1; // error
	}
	for (i = 0; i < (len - 1); i++) {
		*data = IIC_RecvByte();
		data++;
		IIC_ACK();
	}
	*data = IIC_RecvByte();
	IIC_NACK();
	IIC_Stop();
	
	return 0; // success
}

void IIC_Start(void)
{
	IIC_SDA_RESET();
	IIC_Delay();
	IIC_SCL_RESET();
	IIC_Delay();
}

void IIC_Restart(void)
{
	IIC_SDA_SET();
	IIC_Delay();
	IIC_SCL_SET();
	IIC_Delay();
	IIC_SDA_RESET();
	IIC_Delay();
	IIC_SCL_RESET();
	IIC_Delay();
}

void IIC_Stop(void)
{
	IIC_SDA_RESET();
	IIC_Delay();
	IIC_SCL_SET();
	IIC_Delay();
	IIC_SDA_SET();
	IIC_Delay();
}

u8 IIC_RecvACK(void)
{
	u8 recv_ack = 0;
	
	// OUT_OD, set SDA = 1 while reading
	IIC_SDA_SET();
	IIC_Delay();
	
	IIC_SCL_SET();
	IIC_Delay();
	recv_ack = IIC_SDA_IS_HIGH();
	IIC_SCL_RESET();
	IIC_Delay();

	return recv_ack;
}

void IIC_SendByte(u8 byte)
{
	u8 i;

	for (i = 0; i < 8; i++)
	{
		if (byte & 0x80)
			IIC_SDA_SET();
		else
			IIC_SDA_RESET();
		IIC_Delay();
		IIC_SCL_SET();
		IIC_Delay();
		IIC_SCL_RESET();
		IIC_Delay();
		
		byte <<= 1;
	}
}

u8 IIC_RecvByte(void)
{
	u8 i;
	u8 recv_value = 0;
	
	// OUT_OD, set SDA = 1 while reading
	IIC_SDA_SET();
	IIC_Delay();

	for (i = 0; i < 8; i++)
	{
		IIC_SCL_SET();
		IIC_Delay();
		recv_value <<= 1;
		if (IIC_SDA_IS_HIGH())
			recv_value |= 0x01;
		else
			recv_value &= ~0x01;
		IIC_SCL_RESET();
		IIC_Delay();
	}

	return recv_value;
}

void IIC_ACK(void)
{
	IIC_SDA_RESET();
	IIC_Delay();
	IIC_SCL_SET();
	IIC_Delay();
	IIC_SCL_RESET();
	IIC_Delay();
}

void IIC_NACK(void)
{
	IIC_SDA_SET();
	IIC_Delay();
	IIC_SCL_SET();
	IIC_Delay();
	IIC_SCL_RESET();
	IIC_Delay();
}

void IIC_Delay(void)
{
	__NOP();__NOP();__NOP();__NOP();
	__NOP();__NOP();__NOP();__NOP();
	__NOP();__NOP();__NOP();__NOP();
	__NOP();__NOP();__NOP();__NOP();
}
