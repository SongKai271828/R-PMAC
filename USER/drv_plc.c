#include "drv_plc.h"

#include "fw_delay.h"
#include "fw_plc_spi.h"
#include "fw_led_gpio.h"

extern u8 plc_sent;
extern u8 pfc_sent;
extern u8 plc_update;
extern void plc_prm_tx_update(u32 intn_map);
extern u32 get_intmap_SPI(void);
extern void plc_tx_update(u32 intn_map);

void plc_static_config(void)
{
	// load preamble data
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xCEF7FB00);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC53E1765);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x3584DCE6);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x3277FA07);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xD44A143C);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x047CCD94);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x2DA0DD31);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xD1AC1816);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xCECD10ED);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x15C130C9);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF1DC34B8);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xD779DD4E);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x17B5CC58);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x38FAF960);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x373200EA);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x340FF0A0);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x0602C82F);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xDF1AD8C3);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xEB7B352F);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xE3FE35D0);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC98EF604);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xD335E75A);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xD60AE1BF);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC96E09CE);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xE5A22908);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xFC0ECE01);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF369C590);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x11223964);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x137D31EA);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC56B0551);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC9F1106E);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x215AD1EC);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x1231D06C);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xEC7D3207);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x2ACD1B7A);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x3628F7B1);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xDD0B28AC);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC1BBFD72);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x03F5C8AC);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x2E8E0A04);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x0B153872);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF1A13749);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x24CC2C33);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x32D6F4B5);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF438CD0A);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xEA39CB36);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF258CCF2);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC4DFFA16);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xDED925FD);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x34861713);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x327C1849);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x09183481);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x00D636A9);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF96E3162);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF6F134FE);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF31536A6);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xF3C73238);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x14CE3213);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x0ADF32EE);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xCAF60E9D);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0xC875FE8C);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x00B033B6);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x2D161F97);
	FW_PLC_SPI_Write32Bits(CMD_PreambleOp, 0x225BD35B);
	
	// load tx fir coefficients
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0xFF90FFDC);
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0x01AC00F9);
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0xFC0FFD0A);
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0x07F10755);
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0xF069ED66);
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0x27737057);
	
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0xF926F791);
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0x24CA6A0A);
	
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0xFB3FF7DE);
	FW_PLC_SPI_Write32Bits(CMD_TxFilterOp, 0x218C6B4C);
	
	// load rx fir coefficients
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0xFF90FFDC);
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0x01AC00F9);
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0xFC0FFD0A);
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0x07F10755);
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0xF069ED66);
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0x27737057);
	
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0xF926F791);
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0x24CA6A0A);

	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0xFB3FF7DE);
	FW_PLC_SPI_Write32Bits(CMD_RxFilterOp, 0x218C6B4C);
	
	// set scaling
	// txc24cl, txc8scl, txcsscl, txsscl, txoscl
	// rxc24cl, rxcsscl, rxs0scl, rxs1scl, rxoscl
	FW_PLC_SPI_Write32Bits(CMD_SysParamOp, 0x026A02AA);
	
	// set rx optimz
	// 0xF20F0870, 0xF sync base lv, 0x2 sync rltv lv, 0xF cp ofs, 0x870 as 720 crs x 3 sftbt
	FW_PLC_SPI_Write32Bits(CMD_RxOptimzOp, 0xF20F0870);
	
	// set intn mask for rx_err(6),rx_recvd(5),rx_recv(4),tx_done(1) at upper 16bits
	//FW_PLC_SPI_Write32Bits(CMD_InterruptOp, 0x00620000);
	// with txp_done(10), rxp_done(11)
	FW_PLC_SPI_Write32Bits(CMD_InterruptOp, 0x0C620000);
}

void plc_dynamic_config(u32 prmfreq, u8 bandwd, u8 phymd, u32 trxgain, u16 totwdt)
{
	u32 bwpm;
	u32 txp;
	u32 rxp;
	u32 wdt;
	
	// set central frequency
	// 0x16666666, 3.5MHz
	//FW_PLC_SPI_Write32Bits(CMD_FrequencyOp, 0x16666666);
	FW_PLC_SPI_Write32Bits(CMD_FrequencyOp, prmfreq);
	
	// set bandwidth and phymode
	// 0x3, 1.25MHz
	// 0x3, data mode
	bwpm = (((bandwd & 0x0000000F) << 4) | (phymd & 0x0000000F)) << 8;
	FW_PLC_SPI_Write32Bits(CMD_SysModeOp, bwpm);
	
	// set tx params
	// 0x400007FF, 0x4 auto pa ctrl, 0x17FF as max
	txp = ((trxgain & 0x0000FFFF) | 0x40000000);
	FW_PLC_SPI_Write32Bits(CMD_TxParamOp, txp);
	
	// set rx params
	// 0x40000001, 0x4 auto vga ctrl, 0x2 as max vga lv if on
	rxp = (((trxgain & 0xFFFF0000) >> 16) | 0x40000000);
	FW_PLC_SPI_Write32Bits(CMD_RxParamOp, rxp);
	
	// load configuration
	FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000010);
	
	// set watchdog
	// 0x803C0000, 0x8 enable watchdog, 0x003C for 60 secs
	wdt = (totwdt & 0x0000FFFF) << 16;
	FW_PLC_SPI_Write32Bits(CMD_WatchDog, wdt);
}

void plc_wdt_reset(void)
{
	// set watchdog
	// 0x80010000, 0x8 enable watchdog, 0x0001 for 1 sec
	FW_PLC_SPI_Write32Bits(CMD_WatchDog, 0x80010000);
}

int plc_intftest(void)
{
	u32 byte_t;
	
	FW_PLC_SPI_Write32Bits(CMD_SystemOp, 0x55AA55AA);
	byte_t = FW_PLC_SPI_Read32Bits(CMD_SystemOp);
	
	if (byte_t == 0x55AA55AA)
		return 0;
	else
		return 1;
}

u8 plc_send_pfc(plc_psdu *packet, u32 tot)
{
	u8 phymd;
	u32 bwpm;
	
	phymd = packet->target;
	
	if (phymd == 3)
		return 1; // error
	
	bwpm = FW_PLC_SPI_Read32Bits(CMD_SysModeOp);
	bwpm = (bwpm & 0x0000F000) | ((phymd & 0x0000000F) << 8);
	FW_PLC_SPI_Write32Bits(CMD_SysModeOp, bwpm);
	
	// send packet
	FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000008);
	
	StartIdle(tot); // 1 ms
	while (CheckIdle() && pfc_sent == 0) {}
    ClearIdle();
	
	// restore to data mode
	bwpm = (bwpm & 0x0000F000) | 0x00000300;
	FW_PLC_SPI_Write32Bits(CMD_SysModeOp, bwpm);
	
	if (pfc_sent == 0)
		return 1; // error
	
	pfc_sent = 0;
	
	return 0; // success
}

u8 plc_send_PFC(u8 phymd, u32 tot_us)
{
	u32 bwpm;
	
	if (phymd == 3)
		return 1; // error
	
	bwpm = FW_PLC_SPI_Read32Bits(CMD_SysModeOp);
	bwpm = (bwpm & 0x0000F000) | ((phymd & 0x0000000F) << 8);
	FW_PLC_SPI_Write32Bits(CMD_SysModeOp, bwpm);
	
	// send packet
	FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000008);
	
	StartIdle_us(tot_us); // 1 ms
	while (CheckIdle() && pfc_sent == 0) {
		if (plc_update) {
			plc_update = 0;
			plc_prm_tx_update(get_intmap_SPI());
		}
	}
    ClearIdle();
	
	// restore to data mode
	bwpm = (bwpm & 0x0000F000) | 0x00000300;
	FW_PLC_SPI_Write32Bits(CMD_SysModeOp, bwpm);
	
	if (pfc_sent == 0)
		return 1; // error
	
	pfc_sent = 0;
	
	return 0; // success
}

u8 plc_send(plc_psdu *packet, u32 tot)
{
	u8 i;
	u8 buf[UARTBufferLength];
	u8 buf_pls;
	
	u32 byte_0;
	u32 byte_1;
	u32 byte_2;
	u32 byte_3;
	u32 byte_cb;
	u32 bufStatus;
	
	bufStatus = FW_PLC_SPI_Read32Bits(CMD_BufferStatus);
	if ((bufStatus & 0x80000000) >> 31 != 0x1) {
		// clear txbuffer
		FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000004);
		//return 1; // error
	}
	
	buf[0] = packet->type;
	if (packet->direction == 0)
		buf[0] = buf[0] & 0x7F;
	else
		buf[0] = buf[0] | 0x80;
	buf[1] = packet->depth;
	buf[2] = packet->target;
	for (i = 0; i < packet->depth; i++)
		buf[3+i] = packet->route[i];
	
	buf_pls = 3 + packet->depth;
    for (i = 0; i < PHYPayloadLength; i++)
        buf[buf_pls+i] = packet->payload[i];
	
	// load packet
	for (i = 0; i < 25; i++) {
		byte_3 = buf[4*i];
		byte_2 = buf[4*i+1];
		byte_1 = buf[4*i+2];
		byte_0 = buf[4*i+3];
		
		byte_cb = (byte_3 << 24) | (byte_2 << 16) | (byte_1 << 8) | byte_0;
		
		FW_PLC_SPI_Write32Bits(CMD_TxBuffer, byte_cb);
	}
	
	// send packet
	FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000008);
	
	StartIdle(tot); // 20 ms
	while (CheckIdle() && plc_sent == 0) {
		if (plc_update) {
			plc_update = 0;
			plc_tx_update(get_intmap_SPI());
		}
	}
    ClearIdle();
	
	// clear txbuffer
	FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000004);
	
	if (plc_sent == 0) {
		return 1; // error
	}
	
	plc_sent = 0;
	
	return 0; // success
}

u8 plc_recv(plc_psdu *packet)
{
	u8 i;
	u8 buf[UARTBufferLength];
	u8 buf_pls;
	
	u32 byte_cb;
	u32 bufStatus;
	
	// clear rx buffer if load packet failed
	bufStatus = FW_PLC_SPI_Read32Bits(CMD_BufferStatus);
	if ((bufStatus & 0x0000003F) != 0x1B) { // number of bytes reaches 25 + 1 + 1
		// clear rxbuffer
		FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000002);
		return 1; // error
    }
	
	// FW_LED_GPIO_Green(1);
	
	for (i = 0; i < 26; i++) {
		byte_cb = FW_PLC_SPI_Read32Bits(CMD_RxBuffer);
		
		buf[4*i] = (byte_cb & 0xFF000000) >> 24;
		buf[4*i+1] = (byte_cb & 0x00FF0000) >> 16;
		buf[4*i+2] = (byte_cb & 0x0000FF00) >> 8;
		buf[4*i+3] = byte_cb & 0xFF;
    }
	
	packet->type = buf[0];
	packet->direction = (buf[0] & 0x80) >> 7;
	packet->depth = buf[1];
	packet->target = buf[2];
	for (i = 0; i < packet->depth; i++)
		packet->route[i] = buf[3+i];
	
	buf_pls = 3 + packet->depth;
    for (i = 0; i < PHYPayloadLength; i++)
        packet->payload[i] = buf[buf_pls+i];
	
	byte_cb = FW_PLC_SPI_Read32Bits(CMD_RxBuffer);
	packet->errsyms = (byte_cb & 0x00FF0000) >> 16;
	packet->errbits = byte_cb & 0x0000FFFF;
	
	// clear rxbuffer
	FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000002);
	
	// FW_LED_GPIO_Green(0);
	
	return 0; // success
}
