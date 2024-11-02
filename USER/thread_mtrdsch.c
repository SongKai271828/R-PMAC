#include "thread.h"
#include "fw_delay.h"
#include "fw_plc_spi.h"
#include "drv_uart485.h"
#include "drv_flash.h"
#include "drv_rtc.h"

#define SchPayloadLength 55
#define MeterReadingTimeout 400
#define MAXFlashIndex 32767 // 2 Mbytes = 32768 x 64 bytes

u8 tmpl[20] = { 0xFE, 0xFE, 0xFE, 0xFE, 0x68, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x68, 0x11, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x16 };
u8 cmds[18][8] = { // 4 bytes commands, 1 byte position, 1 byte length
	{ 0x33, 0x33, 0x34, 0x33, 0x0E, 0x04, 0x0E, 0x04 }, // composed forward power, 4 bytes
	{ 0x33, 0x33, 0x35, 0x33, 0x0E, 0x04, 0x0E, 0x04 }, // composed backward power, 4 bytes
	{ 0x33, 0x33, 0x36, 0x33, 0x0E, 0x04, 0x0E, 0x04 }, // composed forward reactive power, 4 bytes
	{ 0x33, 0x33, 0x37, 0x33, 0x0E, 0x04, 0x0E, 0x04 }, // composed backward reactive power, 4 bytes
	
	{ 0x33, 0x34, 0x35, 0x35, 0x0E, 0x04, 0x0E, 0x03 }, // phase-A current, 3 bytes
	{ 0x33, 0x35, 0x35, 0x35, 0x0E, 0x04, 0x0E, 0x03 }, // phase-B current, 3 bytes
	{ 0x33, 0x36, 0x35, 0x35, 0x0E, 0x04, 0x0E, 0x03 }, // phase-C current, 3 bytes
	{ 0x34, 0x33, 0xB3, 0x35, 0x0E, 0x04, 0x0E, 0x03 }, // N current, 3 bytes
	
	{ 0x33, 0x33, 0x39, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // total power factor, 2 bytes
	{ 0x33, 0x34, 0x39, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // phase-A power factor, 2 bytes
	{ 0x33, 0x35, 0x39, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // phase-B power factor, 2 bytes
	{ 0x33, 0x36, 0x39, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // phase-C power factor, 2 bytes
	
	{ 0x33, 0x34, 0x3B, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // ua distortion factor, 2 bytes
	{ 0x33, 0x35, 0x3B, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // ub distortion factor, 2 bytes
	{ 0x33, 0x36, 0x3B, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // uc distortion factor, 2 bytes
	{ 0x33, 0x34, 0x3C, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // ia distortion factor, 2 bytes
	{ 0x33, 0x34, 0x3C, 0x35, 0x0E, 0x04, 0x0E, 0x02 }, // ib distortion factor, 2 bytes
	{ 0x33, 0x34, 0x3C, 0x35, 0x0E, 0x04, 0x0E, 0x02 } // ic distortion factor, 2 bytes
};

// config related
extern devcfg dvcf;
// plc related
extern u8 plc_recvd;
// uart related
extern u8 uart485_enFE;
extern u8 uart485_nohdr;
extern u8 uart485_lenpos;
extern u8 uart485_lenmult;
extern u8 uart485_lenadj;
extern u8 uart485_buf[UARTBufferLength];
// schedule metering related
extern u8 scproc_en;
extern u16 sc_cntdwn;
extern u16 sc_cntini;

void prst_uart485_send(u8 itm);

void meter_reading_sch_init(void)
{
	u8 intl;
	u8 sc_s;
	u8 scen;
	u8 dttm[7];
	u8 flbuf[64];
	
	u32 ts_s;
	u32 ts_i;
	u32 ts_c;
	
	flash_read_buffer(flbuf, 0);
	rtc_current(dttm);
	
	sc_s = (flbuf[60] & 0x80) >> 7; // read interval operation enable
	if (sc_s == 1) {
		scen = (flbuf[60] & 0x40) >> 6; // read interval set enable
		if (scen == 1) {
			// set interval in seconds
			intl = (flbuf[60] & 0x3F); // read interval value
			switch (intl) {
				case 0: ts_i = 60; break;
				case 1: ts_i = 300; break;
				case 2: ts_i = 600; break;
				case 3: ts_i = 900; break;
				case 4: ts_i = 1800; break;
				case 5: ts_i = 3600; break;
				default: ts_i = 300; break;
			}
			
			// calc total seconds in current time
			ts_s = ((dttm[5] & 0x000000F0) >> 4) * 600 + (dttm[5] & 0x0000000F) * 60 + 
				((dttm[6] & 0x000000F0) >> 4) * 10 + (dttm[6] & 0x0000000F);
			ts_c = ts_i - (ts_s % ts_i); // calc seconds left to reach interval
			
			sc_cntdwn = ts_c;
			sc_cntini = ts_i;
			scproc_en = 1;
		}
		else
			scproc_en = 0;
	}
}

u8 start_meter_reading_sch(plc_psdu *packet)
{
	u8 i;
	u8 buf_len;
	u8 buf_crc;
	u8 err;
	u8 done;
	
	u32 tot;
	
	plc_psdu packet_r;
	
	for (i = 0; i < 10; i++)
		packet->payload[i] = uart485_buf[4 + packet->depth + i];
	
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	// timeout on recving
	tot = 2 * (packet->depth - 1) * dvcf.totjp;
	tot += FlashAccessTimeout; // flash access delay
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout + flash access delay
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				packet_r.route[0] == dvcf.shortid && // this is source
				(packet_r.type & 0x7F) == 0x0A) // this is scheduled meter reading
			{
				buf_len = 2 + 2 + SchPayloadLength + 1; // fixed value
				
				// copy usart data
				uart485_buf[0] = 0x68;
                uart485_buf[1] = buf_len - 2;
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

u8 redirect_meter_reading_sch(plc_psdu *packet)
{
	u8 jp;
	u8 err;
	u8 done;
	
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
	tot += FlashAccessTimeout; // flash access delay
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout + meter reading delay
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				//packet_r.route[0] == ShortID && // this is source
				(packet_r.type & 0x7F) == 0x0A) // this is scheduled meter reading
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

u8 process_meter_reading_sch(plc_psdu *packet)
{
	u8 i;
	u8 intl;
	u8 sc_s;
	u8 scen;
	u8 err;
	u8 dttm[7];
	u8 flbuf[64];
	
	u16 rq_ind;
	u16 fl_ind;
	
	u32 ts_s;
	u32 ts_i;
	u32 ts_c;
	
	rq_ind = (packet->payload[0] & 0x00FF);
	rq_ind = (rq_ind << 8) | packet->payload[1];
	
	// get record id from flash
    flash_read_buffer(flbuf, 0);
	fl_ind = ((flbuf[58] & 0x00FF) << 8) | (flbuf[59] & 0x00FF); // read record index
	
	sc_s = (packet->payload[2] & 0x80) >> 7;
	if (sc_s == 1) { // only if time adj required
		scen = (packet->payload[2] & 0x40) >> 6;
		if (scen == 1) {
			// set datetime
			for (i = 0; i < 7; i++)
				dttm[i] = packet->payload[3+i];
			rtc_setclock(dttm);
			
			// set interval in seconds
			intl = (packet->payload[2] & 0x3F);
			switch (intl) {
				case 0: ts_i = 60; break;
				case 1: ts_i = 300; break;
				case 2: ts_i = 600; break;
				case 3: ts_i = 900; break;
				case 4: ts_i = 1800; break;
				case 5: ts_i = 3600; break;
				default: ts_i = 300; break;
			}
			
			// calc total seconds in current time
			ts_s = ((packet->payload[8] & 0x000000F0) >> 4) * 600 + (packet->payload[8] & 0x0000000F) * 60 + 
				((packet->payload[9] & 0x000000F0) >> 4) * 10 + (packet->payload[9] & 0x0000000F);
			ts_c = ts_i - (ts_s % ts_i); // calc seconds left to reach interval
			
			sc_cntdwn = ts_c;
			sc_cntini = ts_i;
			scproc_en = 1;
		}
		else
			scproc_en = 0;
			
		flbuf[60] = packet->payload[2]; // save interval settings
		flash_write_buffer(flbuf, 0);
	}
	
	// load sch mtr rd record id
	set_destination_reply(packet);
	// read record index
	packet->payload[0] = flbuf[58];
	packet->payload[1] = flbuf[59];
	
	// fetch results from Flash
	if (rq_ind == 0)
		rq_ind = fl_ind;
	
	if (rq_ind > 0) {
		flash_read_buffer(flbuf, rq_ind);
		for (i = 0; i < SchPayloadLength; i++)
			packet->payload[2 + i] = flbuf[i];
	}
	
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	return 0; // success
}

void save_meter_reading_sch(void)
{
	u8 i;
	u8 j;
	u8 ind;
	u8 pl_len;
	u8 rt_pos;
	u8 err;
	u8 dttm[7];
	u8 flbuf[64];
	
	u16 fl_ind;
	
	u32 tot;
	
	// set protocol parameters
	uart485_enFE = 1;
	uart485_nohdr = 0;
	uart485_lenpos = 9;
	uart485_lenmult = 1;
	uart485_lenadj = 12;
	
	ind = 0;
	
	tot = MeterReadingTimeout;
	
	// get record id from flash
    flash_read_buffer(flbuf, 0);
	fl_ind = ((flbuf[58] & 0x00FF) << 8) | (flbuf[59] & 0x00FF); // read record index
	
	// set datetime
	rtc_current(dttm);
	for (i = 0; i < 7; i++)
		flbuf[i] = dttm[i];
	
	ind = 7;
    
	// 18 scheduled meter reading commands
    for (i = 0; i < 18; i++) {
		restart_uart485();
		
		err = 1;
		prst_uart485_send(i);
		pl_len = start_uart485(tot);
		if (pl_len > 8) {
			if (uart485_buf[8] == 0x91) {
				for (j = 0; j < cmds[i][7]; j++) {
					rt_pos = cmds[i][6];
					flbuf[ind + j] = uart485_buf[rt_pos + j];
				}
				ind += cmds[i][7]; //pl_len - 16;
				err = 0;
			}
		}
		
		if (err == 1) {
			for (j = 0; j < cmds[i][7]; j++)
				flbuf[ind + j] = 0xFF;
            ind += cmds[i][7];
		}
		
		restart_uart485();
	}
	
	// save results to Flash
	fl_ind++;
	if (fl_ind > MAXFlashIndex)
		fl_ind = 1;
	flash_write_buffer(flbuf, fl_ind);
	
	// update the record id
    flash_read_buffer(flbuf, 0);
	// save current record index
	flbuf[58] = (fl_ind & 0xFF00) >> 8;
	flbuf[59] = (fl_ind & 0xFF);
	flash_write_buffer(flbuf, 0);
}

void prst_uart485_send(u8 itm)
{
    u8 i;
	u8 pos;
    u8 buf_crc;
	
	pos = cmds[itm][4];
	for (i = 0; i < cmds[itm][5]; i++) {
		tmpl[pos + i] = cmds[itm][i];
	}
    
    buf_crc = 0;
    for (i = 4; i < 18; i++)
        buf_crc += tmpl[i];
    tmpl[18] = buf_crc;
	
	uart485_send(tmpl, 20);
}
