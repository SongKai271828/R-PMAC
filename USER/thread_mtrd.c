#include "thread.h"
#include "fw_delay.h"
#include "fw_plc_spi.h"
#include "drv_uart485.h"

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

u32 timeout_by_mttyp(u8 mttyp);

u8 start_meter_reading(plc_psdu *packet, u8 len)
{
	u8 i;
	u8 mttyp;
	u8 chntyp;
	u8 buf_len;
	u8 err;
	u8 done;
	
	u32 tot;
	
	plc_psdu packet_r;
	
	packet->payload[0] = uart485_buf[4 + packet->depth]; // set meter command type
	packet->payload[1] = len - packet->depth - 4; // set meter command length
	for (i = 0; i < (len - packet->depth - 4); i++) // copy usart data
		packet->payload[2 + i] = uart485_buf[5 + packet->depth + i];
	
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	mttyp = (packet->payload[0] & 0x7F);
	chntyp = (packet->payload[0] & 0x80) >> 7;
	
	// timeout on recving
	tot = 2 * (packet->depth - 1) * dvcf.totjp;
	tot += timeout_by_mttyp(mttyp);
	if (chntyp == 1)
		tot += dvcf.totchn;
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout + meter reading delay + channel delay
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				packet_r.route[0] == dvcf.shortid && // this is source
				(packet_r.type & 0x7F) == 0x08) // this is meter reading
			{
				buf_len = packet_r.payload[1];
				
				// copy usart data
				for (i = 0; i < buf_len; i++)
					uart485_buf[i] = packet_r.payload[2 + i];
				
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

u8 redirect_meter_reading(plc_psdu *packet)
{
	u8 err;
	u8 jp;
	u8 mttyp;
	u8 chntyp;
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
	
	mttyp = (packet->payload[0] & 0x7F);
	chntyp = (packet->payload[0] & 0x80) >> 7;
	
	// timeuot on recving
	tot = 2 * (packet->depth - 1) * dvcf.totjp;
	tot += timeout_by_mttyp(mttyp);
	if (chntyp == 1)
		tot += dvcf.totchn;
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout + meter reading delay + channel delay
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				//packet_r.route[0] == ShortID && // this is source
				(packet_r.type & 0x7F) == 0x08) // this is meter reading
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

u8 process_meter_reading(plc_psdu *packet)
{
	u8 i;
	u8 mttyp;
	u8 chntyp;
	u8 buf_len;
	u8 pl_len;
	u8 err;
	
	u32 tot;
	
	mttyp = (packet->payload[0] & 0x7F);
	chntyp = (packet->payload[0] & 0x80) >> 7;
	
	for (i = 0; i < 8; i++) {
		if (mttyp == dvcf.prttyp[i].type && dvcf.prttyp[i].inuse == 1) {
			uart485_enFE = dvcf.prttyp[i].enFE;
			uart485_nohdr = dvcf.prttyp[i].nohdr;
			uart485_lenpos = dvcf.prttyp[i].lenpos;
			uart485_lenmult = dvcf.prttyp[i].lenmult;
			uart485_lenadj = dvcf.prttyp[i].lenadj;
			
			break;
		}
	}
	
	buf_len = packet->payload[1];
	for (i = 0; i < buf_len; i++)
		uart485_buf[i] = packet->payload[2 + i];
	
	tot = timeout_by_mttyp(mttyp);
	if (chntyp == 1)
		tot += dvcf.totchn;
	
	restart_uart485();
	
	uart485_send(uart485_buf, buf_len);
	pl_len = start_uart485(tot);
	if (pl_len == 0)
		return 1; // error
	
	restart_uart485();
	
	set_destination_reply(packet);
	packet->payload[1] = pl_len;
	for (i = 0; i < pl_len; i++)
		packet->payload[2 + i] = uart485_buf[i];
	
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	return 0; // success
}

u32 timeout_by_mttyp(u8 typ)
{
	u8 i;
	u32 tot;
	
	tot = 400; // 400 ms by default
	
	for (i = 0; i < 8; i++) {
		if (typ == dvcf.prttyp[i].type && dvcf.prttyp[i].inuse == 0x01) {
			tot = dvcf.prttyp[i].tot;
			break;
		}
	}
	
	return tot;
}
