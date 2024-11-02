#include "thread.h"
#include "fw_delay.h"
#include "fw_plc_spi.h"
#include "drv_flash.h"
#include "drv_uart485.h"

#define SettingsWRFlashSpace 58 // 58 bytes in total
#define SettingsRDFlashSpace 63 // 63 bytes in total

// config related
devcfg dvcf;
// plc related
extern u8 plc_recvd;
// uart related
extern u8 uart485_buf[UARTBufferLength];
// gpio related
extern u8 rstr_en;

void device_config_load(void)
{
	u8 i;
	u8 cfg_no;
	u8 flbuf[64];
	
	// load settings from flash
	flash_read_buffer(flbuf, 0);
	
	if (rstr_en == 1) {
		for (i = 0; i < SettingsRDFlashSpace; i++)
			flbuf[i] = 0x00;
		
		flash_write_buffer(flbuf, 0);
	}
	
	// initial settings
	dvcf.dbgmap = 0x0000;
	dvcf.savmap = 0x0000;
	dvcf.nodetype = 0x00;
	dvcf.shortid = 0x00;
	dvcf.prmfreq = 0x16666666;
	dvcf.swtfreq = 0x16666666;
	dvcf.uartcb.baudrate = 0x02;
	dvcf.uartcb.is8b = 0x01;
	dvcf.uartcb.parity = 0x00;
	dvcf.bandwd = 0x03;
	dvcf.phymd = 0x03;
	dvcf.trxgain = 0x000117FF;
	dvcf.totchn = 0x01F4;
	// dvcf.totjp = 0x0016;
	dvcf.totjp = 0x0014;
	dvcf.totwdt = 0x0000;
	dvcf.prttyp[0].inuse = 0x01;
	dvcf.prttyp[0].type = 0x00;
	dvcf.prttyp[0].tot = 0x0190;
	dvcf.prttyp[0].lenpos = 0x09;
	dvcf.prttyp[0].lenmult = 0x01;
	dvcf.prttyp[0].enFE = 0x01;
	dvcf.prttyp[0].nohdr = 0x00;
	dvcf.prttyp[0].lenadj = 0x0C;
	for (i = 1; i < 8; i++) {
		dvcf.prttyp[i].inuse = 0x00;
		dvcf.prttyp[i].type = 0x00;
		dvcf.prttyp[i].tot = 0x0000;
		dvcf.prttyp[i].lenpos = 0x00;
		dvcf.prttyp[i].lenmult = 0x00;
		dvcf.prttyp[i].enFE = 0x00;
		dvcf.prttyp[i].nohdr = 0x00;
		dvcf.prttyp[i].lenadj = 0x00;
	}
	
	dvcf.savmap = ((flbuf[2] & 0xFFFF) << 8) | (flbuf[3] & 0xFFFF);
	if (((dvcf.savmap & 0x4000) >> 14) == 0x01)
		dvcf.nodetype = flbuf[4];
	if (((dvcf.savmap & 0x2000) >> 13) == 0x01)
		dvcf.shortid = flbuf[5];
	if (((dvcf.savmap & 0x1000) >> 12) == 0x01)
		dvcf.prmfreq = ((flbuf[6] & 0x000000FF) << 24) | ((flbuf[7] & 0x000000FF) << 16) | 
			((flbuf[8] & 0x000000FF) << 8) | (flbuf[9] & 0x000000FF);
	if (((dvcf.savmap & 0x0800) >> 11) == 0x01)
		dvcf.swtfreq = ((flbuf[10] & 0x000000FF) << 24) | ((flbuf[11] & 0x000000FF) << 16) | 
			((flbuf[12] & 0x000000FF) << 8) | (flbuf[13] & 0x000000FF);
	if (((dvcf.savmap & 0x0400) >> 10) == 0x01) {
		dvcf.uartcb.baudrate = (flbuf[14] & 0xF0) >> 4;
		dvcf.uartcb.is8b = (flbuf[14] & 0x08) >> 3;
		dvcf.uartcb.parity = (flbuf[14] & 0x03);
	}
	if (((dvcf.savmap & 0x0200) >> 9) == 0x01) {
		dvcf.bandwd = (flbuf[15] & 0xF0) >> 4;
		dvcf.phymd = (flbuf[15] & 0x0F);
	}
	if (((dvcf.savmap & 0x0100) >> 8) == 0x01)
		dvcf.trxgain = ((flbuf[16] & 0x000000FF) << 24) | ((flbuf[17] & 0x000000FF) << 16) | 
			((flbuf[18] & 0x000000FF) << 8) | (flbuf[19] & 0x000000FF);
	if (((dvcf.savmap & 0x0080) >> 7) == 0x01)
		dvcf.totchn = ((flbuf[20] & 0x000F) << 8) | (flbuf[21] & 0x00FF);
	if (((dvcf.savmap & 0x0040) >> 6) == 0x01)
		dvcf.totjp = ((flbuf[22] & 0x000F) << 8) | (flbuf[23] & 0x00FF);
	if (((dvcf.savmap & 0x0020) >> 5) == 0x01)
		dvcf.totwdt = ((flbuf[24] & 0x00FF) << 8) | (flbuf[25] & 0x00FF);
	if (((dvcf.savmap & 0x0008) >> 3) == 0x01) {
		// get number of configurations
		cfg_no = (dvcf.savmap & 0x0007); // 0..7
		// make sure the number is in range
		for (i = 0; i <= cfg_no; i++) {
			dvcf.prttyp[i].inuse = 0x01;
			dvcf.prttyp[i].type = (flbuf[i*4+26] & 0xF0) >> 4;
			dvcf.prttyp[i].tot = ((flbuf[i*4+26] & 0x000F) << 8) | (flbuf[i*4+27] & 0x00FF);
			dvcf.prttyp[i].lenpos = (flbuf[i*4+28] & 0xF0) >> 4;
			dvcf.prttyp[i].lenmult = (flbuf[i*4+28] & 0x0F);
			dvcf.prttyp[i].enFE = (flbuf[i*4+29] & 0x80) >> 7;
			dvcf.prttyp[i].nohdr = (flbuf[i*4+29] & 0x40) >> 6;
			dvcf.prttyp[i].lenadj = (flbuf[i*4+29] & 0x3F);
		}
	}
}

void start_this_config(void)
{
	u8 i;
	u8 ofs;
	u8 cfg_no;
	u8 buf_crc;
	u8 flbuf[64];
	
	u16 dbgmap;
	u16 savmap;
	u16 savmap_o;
	
	// load settings from flash
	flash_read_buffer(flbuf, 0);
	
	ofs = 4 + uart485_buf[3];
	
	dbgmap = ((uart485_buf[ofs] & 0xFFFF) << 8) | (uart485_buf[ofs+1] & 0xFFFF);
	savmap = ((uart485_buf[ofs+2] & 0xFFFF) << 8) | (uart485_buf[ofs+3] & 0xFFFF);
	savmap_o = ((flbuf[2] & 0xFFFF) << 8) | (flbuf[3] & 0xFFFF);
	
	if (dbgmap != 0x0000 && savmap == 0x0000) {
		if (((dbgmap & 0x4000) >> 14) == 0x01)
			dvcf.nodetype = uart485_buf[ofs+4];
		if (((dbgmap & 0x2000) >> 13) == 0x01)
			dvcf.shortid = uart485_buf[ofs+5];
		if (((dbgmap & 0x1000) >> 12) == 0x01)
			dvcf.prmfreq = ((uart485_buf[ofs+6] & 0x000000FF) << 24) | ((uart485_buf[ofs+7] & 0x000000FF) << 16) | 
				((uart485_buf[ofs+8] & 0x000000FF) << 8) | (uart485_buf[ofs+9] & 0x000000FF);
		if (((dbgmap & 0x0800) >> 11) == 0x01)
			dvcf.swtfreq = ((uart485_buf[ofs+10] & 0x000000FF) << 24) | ((uart485_buf[ofs+11] & 0x000000FF) << 16) | 
				((uart485_buf[ofs+12] & 0x000000FF) << 8) | (uart485_buf[ofs+13] & 0x000000FF);
		if (((dbgmap & 0x0400) >> 10) == 0x01) {
			dvcf.uartcb.baudrate = (uart485_buf[ofs+14] & 0xF0) >> 4;
			dvcf.uartcb.is8b = (uart485_buf[ofs+14] & 0x08) >> 3;
			dvcf.uartcb.parity = (uart485_buf[ofs+14] & 0x03);
		}
		if (((dbgmap & 0x0200) >> 9) == 0x01) {
			dvcf.bandwd = (uart485_buf[ofs+15] & 0xF0) >> 4;
			dvcf.phymd = (uart485_buf[ofs+15] & 0x0F);
		}
		if (((dbgmap & 0x0100) >> 8) == 0x01)
			dvcf.trxgain = ((uart485_buf[ofs+16] & 0x000000FF) << 24) | ((uart485_buf[ofs+17] & 0x000000FF) << 16) | 
				((uart485_buf[ofs+18] & 0x000000FF) << 8) | (uart485_buf[ofs+19] & 0x000000FF);
		if (((dbgmap & 0x0080) >> 7) == 0x01)
			dvcf.totchn = ((uart485_buf[ofs+20] & 0x000F) << 8) | (uart485_buf[ofs+21] & 0x00FF);
		if (((dbgmap & 0x0040) >> 6) == 0x01)
			dvcf.totjp = ((uart485_buf[ofs+22] & 0x000F) << 8) | (uart485_buf[ofs+23] & 0x00FF);
		if (((dbgmap & 0x0020) >> 5) == 0x01)
			dvcf.totwdt = ((uart485_buf[ofs+24] & 0x00FF) << 8) | (uart485_buf[ofs+25] & 0x00FF);
		if (((dbgmap & 0x0008) >> 3) == 0x01) {
			// init firstly
			for (i = 0; i < 8; i++) {
				dvcf.prttyp[i].inuse = 0x00;
				dvcf.prttyp[i].type = 0x00;
				dvcf.prttyp[i].tot = 0x0000;
				dvcf.prttyp[i].lenpos = 0x00;
				dvcf.prttyp[i].lenmult = 0x00;
				dvcf.prttyp[i].enFE = 0x00;
				dvcf.prttyp[i].nohdr = 0x00;
				dvcf.prttyp[i].lenadj = 0x00;
			}
			// get number of configurations
			cfg_no = (dbgmap & 0x0007); // 0..7
			// make sure the number is in range
			for (i = 0; i <= cfg_no; i++) {
				dvcf.prttyp[i].inuse = 0x01;
				dvcf.prttyp[i].type = (uart485_buf[ofs+i*4+26] & 0xF0) >> 4;
				dvcf.prttyp[i].tot = ((uart485_buf[ofs+i*4+26] & 0x000F) << 8) | (uart485_buf[ofs+i*4+27] & 0x00FF);
				dvcf.prttyp[i].lenpos = (uart485_buf[ofs+i*4+28] & 0xF0) >> 4;
				dvcf.prttyp[i].lenmult = (uart485_buf[ofs+i*4+28] & 0x0F);
				dvcf.prttyp[i].enFE = (uart485_buf[ofs+i*4+29] & 0x80) >> 7;
				dvcf.prttyp[i].nohdr = (uart485_buf[ofs+i*4+29] & 0x40) >> 6;
				dvcf.prttyp[i].lenadj = (uart485_buf[ofs+i*4+29] & 0x3F);
			}
		}
	}
	
	if (dbgmap == 0x0000 && savmap != 0x0000) {
		if (((savmap & 0x0008) >> 3) == 0x01) {
			savmap_o = savmap_o & 0x7FF0;
			savmap_o = savmap_o | savmap;

			// init firstly
			for (i = 0; i < 8; i++) {
				flbuf[i*4+26] = 0x00;
				flbuf[i*4+27] = 0x00;
				flbuf[i*4+28] = 0x00;
				flbuf[i*4+29] = 0x00;
			}
			// get number of configurations
			cfg_no = (savmap & 0x0007); // 0..7
			// make sure the number is in range
			for (i = 0; i <= cfg_no; i++) {
				flbuf[i*4+26] = uart485_buf[ofs+i*4+26];
				flbuf[i*4+27] = uart485_buf[ofs+i*4+27];
				flbuf[i*4+28] = uart485_buf[ofs+i*4+28];
				flbuf[i*4+29] = uart485_buf[ofs+i*4+29];
			}
		}
		else {
			savmap = savmap & 0x7FF0;
			savmap_o = savmap_o | savmap;
		}
		
		flbuf[0] = 0x00;
		flbuf[1] = 0x00;
		flbuf[2] = (savmap_o & 0xFF00) >> 8;
		flbuf[3] = (savmap_o & 0x00FF);
//		for (i = 4; i < SettingsWRFlashSpace; i++)
//			flbuf[i] = uart485_buf[ofs+i];
		if (((savmap & 0x4000) >> 14) == 0x01)
			flbuf[4] = uart485_buf[ofs+4];
		if (((savmap & 0x2000) >> 13) == 0x01)
			flbuf[5] = uart485_buf[ofs+5];
		if (((savmap & 0x1000) >> 12) == 0x01)
			for (i = 0; i < 4; i++)
				flbuf[6+i] = uart485_buf[ofs+6+i];
		if (((savmap & 0x0800) >> 11) == 0x01)
			for (i = 0; i < 4; i++)
				flbuf[10+i] = uart485_buf[ofs+10+i];
		if (((savmap & 0x0400) >> 10) == 0x01)
			flbuf[14] = uart485_buf[ofs+14];
		if (((savmap & 0x0200) >> 9) == 0x01)
			flbuf[15] = uart485_buf[ofs+15];
		if (((savmap & 0x0100) >> 8) == 0x01)
			for (i = 0; i < 4; i++)
				flbuf[16+i] = uart485_buf[ofs+16+i];
		if (((savmap & 0x0080) >> 7) == 0x01) {
			flbuf[20] = uart485_buf[ofs+20];
			flbuf[21] = uart485_buf[ofs+21];
		}
		if (((savmap & 0x0040) >> 6) == 0x01) {
			flbuf[22] = uart485_buf[ofs+22];
			flbuf[23] = uart485_buf[ofs+23];
		}
		if (((savmap & 0x0020) >> 5) == 0x01) {
			flbuf[24] = uart485_buf[ofs+24];
			flbuf[25] = uart485_buf[ofs+25];
		}
		
		flash_write_buffer(flbuf, 0);
	}
	
	uart485_buf[0] = 0x68;
	uart485_buf[1] = SettingsRDFlashSpace + 1;
	uart485_buf[2] = 0x00;
	uart485_buf[3] = 0x00;
	for (i = 2; i < SettingsRDFlashSpace; i++)
		uart485_buf[i + 2] = flbuf[i];
	
	buf_crc = 0;
	for (i = 0; i < (SettingsRDFlashSpace + 1); i++)
		buf_crc += uart485_buf[1 + i];
	uart485_buf[SettingsRDFlashSpace + 2] = buf_crc;
	
	uart485_send(uart485_buf, SettingsRDFlashSpace + 3);
}

u8 start_device_config(plc_psdu *packet) // packet incl header already
{
	u8 i;
	u8 buf_crc;
	u8 buf_len;
	u8 done;
	u8 err;
	
	u32 tot;
	
	plc_psdu packet_r;

	// uart configurations
	// meter reading configuration
	for (i = 0; i < SettingsWRFlashSpace; i++) // copy usart data
		packet->payload[i] = uart485_buf[4 + packet->depth + i];
	
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	// timeout on recving
	tot = 2 * (packet->depth - 1) * dvcf.totjp;
	tot += FlashAccessTimeout;
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout + meter reading delay
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				packet_r.route[0] == dvcf.shortid && // this is source
				(packet_r.type & 0x7F) == 0x06) // this is device config
			{
				buf_len = SettingsRDFlashSpace + 3;
				
				// set usart data
                uart485_buf[0] = 0x68;
                uart485_buf[1] = buf_len - 2;
				// copy usart data
				for (i = 0; i < (buf_len - 3); i++)
					uart485_buf[2 + i] = packet_r.payload[i];
                // generate crc
				buf_crc = 0;
                for (i = 0; i < (buf_len - 2); i++)
					buf_crc += uart485_buf[1 + i];
                uart485_buf[buf_len - 1] = buf_crc;
				
				done = 1;
			}
			
			plc_recvd = 0;
		}
	}
	ClearIdle();
	
	if (done == 0)
		return 1; // error
	
	uart485_send(uart485_buf, buf_len);
	
	return 0; // success
}

u8 redirect_device_config(plc_psdu *packet)
{
	u8 jp;
	u8 done;
	u8 err;
	
	u32 tot;
	
	plc_psdu packet_r;
	
	jp = get_current_jump(packet);
	set_next_target(packet, jp);
	
	// switch to temporary frequency
	FW_PLC_SPI_Write32Bits(CMD_FrequencyOp, dvcf.swtfreq);
	
	err = plc_send(packet, dvcf.totjp);
	if (err) {
		// switch to primary frequency
		FW_PLC_SPI_Write32Bits(CMD_FrequencyOp, dvcf.prmfreq);
		
		return 1; // error
	}
	
	// timeuot on recving
	tot = 2 * (packet->depth - 1 - jp) * dvcf.totjp;
	tot += FlashAccessTimeout;
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				//packet_r.route[0] == ShortID && // this is source
				(packet_r.type & 0x7F) == 0x06) // this is device config
			{
				done = 1;
			}
			
			plc_recvd = 0;
		}
	}
	ClearIdle();
	
	// switch to primary frequency
	FW_PLC_SPI_Write32Bits(CMD_FrequencyOp, dvcf.prmfreq);
	
	if (done == 0)
		return 1; // error
	
	jp = get_current_jump(&packet_r);
	set_next_target(&packet_r, jp);
	
	err = plc_send(&packet_r, dvcf.totjp);
	if (err)
		return 1; // error
	
	return 0; // success
}

u8 process_device_config(plc_psdu *packet)
{
	u8 i;
	u8 cfg_no;
	u8 flbuf[64];
	u8 err;
	
	u16 dbgmap;
	u16 savmap;
	u16 savmap_o;
	
	dbgmap = ((packet->payload[0] & 0xFFFF) << 8) | (packet->payload[1] & 0xFFFF);
	savmap = ((packet->payload[2] & 0xFFFF) << 8) | (packet->payload[3] & 0xFFFF);
	
	if (dbgmap != 0x0000 && savmap == 0x0000) {
		if (((dbgmap & 0x4000) >> 14) == 0x01)
			dvcf.nodetype = packet->payload[4];
		if (((dbgmap & 0x2000) >> 13) == 0x01)
			dvcf.shortid = packet->payload[5];
		if (((dbgmap & 0x1000) >> 12) == 0x01)
			dvcf.prmfreq = ((packet->payload[6] & 0x000000FF) << 24) | ((packet->payload[7] & 0x000000FF) << 16) | 
				((packet->payload[8] & 0x000000FF) << 8) | (packet->payload[9] & 0x000000FF);
		if (((dbgmap & 0x0800) >> 11) == 0x01)
			dvcf.swtfreq = ((packet->payload[10] & 0x000000FF) << 24) | ((packet->payload[11] & 0x000000FF) << 16) | 
				((packet->payload[12] & 0x000000FF) << 8) | (packet->payload[13] & 0x000000FF);
		if (((dbgmap & 0x0400) >> 10) == 0x01) {
			dvcf.uartcb.baudrate = (packet->payload[14] & 0xF0) >> 4;
			dvcf.uartcb.is8b = (packet->payload[14] & 0x08) >> 3;
			dvcf.uartcb.parity = (packet->payload[14] & 0x03);
		}
		if (((dbgmap & 0x0200) >> 9) == 0x01) {
			dvcf.bandwd = (packet->payload[15] & 0xF0) >> 4;
			dvcf.phymd = (packet->payload[15] & 0x0F);
		}
		if (((dbgmap & 0x0100) >> 8) == 0x01)
			dvcf.trxgain = ((packet->payload[16] & 0x000000FF) << 24) | ((packet->payload[17] & 0x000000FF) << 16) | 
				((packet->payload[18] & 0x000000FF) << 8) | (packet->payload[19] & 0x000000FF);
		if (((dbgmap & 0x0080) >> 7) == 0x01)
			dvcf.totchn = ((packet->payload[20] & 0x000F) << 8) | (packet->payload[21] & 0x00FF);
		if (((dbgmap & 0x0040) >> 6) == 0x01)
			dvcf.totjp = ((packet->payload[22] & 0x000F) << 8) | (packet->payload[23] & 0x00FF);
		if (((dbgmap & 0x0020) >> 5) == 0x01)
			dvcf.totwdt = ((packet->payload[24] & 0x00FF) << 8) | (packet->payload[25] & 0x00FF);
		if (((dbgmap & 0x0008) >> 3) == 0x01) {
			// get number of configurations
			cfg_no = (dbgmap & 0x0007);
			// make sure the number is in range
			for (i = 0; i < 8; i++) {
				if (i <= cfg_no) {
					dvcf.prttyp[i].inuse = 0x01;
					dvcf.prttyp[i].type = (packet->payload[i*4+26] & 0xF0) >> 4;
					dvcf.prttyp[i].tot = ((packet->payload[i*4+26] & 0x000F) << 8) | (packet->payload[i*4+27] & 0x00FF);
					dvcf.prttyp[i].lenpos = (packet->payload[i*4+28] & 0xF0) >> 4;
					dvcf.prttyp[i].lenmult = (packet->payload[i*4+28] & 0x0F);
					dvcf.prttyp[i].enFE = (packet->payload[i*4+29] & 0x80) >> 7;
					dvcf.prttyp[i].nohdr = (packet->payload[i*4+29] & 0x40) >> 6;
					dvcf.prttyp[i].lenadj = (packet->payload[i*4+29] & 0x3F);
				}
				else {
					dvcf.prttyp[i].inuse = 0x00;
					dvcf.prttyp[i].type = 0x00;
					dvcf.prttyp[i].tot = 0x0000;
					dvcf.prttyp[i].lenpos = 0x00;
					dvcf.prttyp[i].lenmult = 0x00;
					dvcf.prttyp[i].enFE = 0x00;
					dvcf.prttyp[i].nohdr = 0x00;
					dvcf.prttyp[i].lenadj = 0x00;
				}
			}
		}
	}
	
	// load settings from flash
	flash_read_buffer(flbuf, 0);
	
	if (dbgmap == 0x0000 && savmap != 0x0000) {
		savmap_o = ((flbuf[2] & 0xFFFF) << 8) | (flbuf[3] & 0xFFFF);
		if (((savmap & 0x0008) >> 3) == 0x01) {
			savmap_o = savmap_o & 0x7FF0;
			savmap_o = savmap_o | savmap;
			
			// init firstly
			for (i = 0; i < 8; i++) {
				flbuf[i*4+26] = 0x00;
				flbuf[i*4+27] = 0x00;
				flbuf[i*4+28] = 0x00;
				flbuf[i*4+29] = 0x00;
			}
			// get number of configurations
			cfg_no = (savmap & 0x0007); // 0..7
			// make sure the number is in range
			for (i = 0; i <= cfg_no; i++) {
				flbuf[i*4+26] = packet->payload[i*4+26];
				flbuf[i*4+27] = packet->payload[i*4+27];
				flbuf[i*4+28] = packet->payload[i*4+28];
				flbuf[i*4+29] = packet->payload[i*4+29];
			}
		}
		else {
			savmap = savmap & 0x7FF0;
			savmap_o = savmap_o | savmap;
		}
		
		flbuf[0] = 0x00;
		flbuf[1] = 0x00;
		flbuf[2] = (savmap_o & 0xFF00) >> 8;
		flbuf[3] = (savmap_o & 0x00FF);
//		for (i = 2; i < SettingsWRFlashSpace; i++)
//			flbuf[i] = packet->payload[i];
		if (((savmap & 0x4000) >> 14) == 0x01)
			flbuf[4] = packet->payload[4];
		if (((savmap & 0x2000) >> 13) == 0x01)
			flbuf[5] = packet->payload[5];
		if (((savmap & 0x1000) >> 12) == 0x01)
			for (i = 0; i < 4; i++)
				flbuf[6+i] = packet->payload[6+i];
		if (((savmap & 0x0800) >> 11) == 0x01)
			for (i = 0; i < 4; i++)
				flbuf[10+i] = packet->payload[10+i];
		if (((savmap & 0x0400) >> 10) == 0x01)
			flbuf[14] = packet->payload[14];
		if (((savmap & 0x0200) >> 9) == 0x01)
			flbuf[15] = packet->payload[15];
		if (((savmap & 0x0100) >> 8) == 0x01)
			for (i = 0; i < 4; i++)
				flbuf[16+i] = packet->payload[16+i];
		if (((savmap & 0x0080) >> 7) == 0x01) {
			flbuf[20] = packet->payload[20];
			flbuf[21] = packet->payload[21];
		}
		if (((savmap & 0x0040) >> 6) == 0x01) {
			flbuf[22] = packet->payload[22];
			flbuf[23] = packet->payload[23];
		}
		if (((savmap & 0x0020) >> 5) == 0x01) {
			flbuf[24] = packet->payload[24];
			flbuf[25] = packet->payload[25];
		}
		
		flash_write_buffer(flbuf, 0);
	}
	
	set_destination_reply(packet);
	packet->payload[0] = 0x00;
	packet->payload[1] = 0x00;
	for (i = 2; i < SettingsRDFlashSpace; i++)
		packet->payload[i] = flbuf[i];
	
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	return 0; // success
}
