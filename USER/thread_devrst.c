#include "thread.h"
#include "fw_delay.h"
#include "fw_plc_spi.h"
#include "drv_uart485.h"

// config related
extern devcfg dvcf;
// plc related
extern u8 plc_recvd;
// uart related
extern u8 uart485_buf[UARTBufferLength];

u8 start_device_reset(plc_psdu *packet)
{
	u8 i;
	u8 buf_len;
	u8 buf_crc;
	u8 err;
	u8 done;
	
	u32 tot;
	
	plc_psdu packet_r;
	
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	// timeuot on recving
	tot = 2 * (packet->depth - 1) * dvcf.totjp;
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				packet_r.route[0] == dvcf.shortid && // this is source
				(packet_r.type & 0x7F) == 0x09) // this is device reset
			{
				buf_len = 4 + packet_r.depth + 1;
				
				// set usart data
                uart485_buf[0] = 0x68;
                uart485_buf[1] = buf_len - 2;
				uart485_buf[2] = packet_r.type;
				uart485_buf[3] = packet_r.depth;
				for (i = 0; i < packet_r.depth; i++)
					uart485_buf[4 + i] = packet_r.route[i];
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

u8 redirect_device_reset(plc_psdu *packet)
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
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout
	while (CheckIdle() && done == 0) {
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				//packet_r.route[0] == ShortID && // this is source
				(packet_r.type & 0x7F) == 0x09) // this is device reset
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

u8 process_device_reset(plc_psdu *packet)
{
	u8 err;
	
	// set destination relpy
    set_destination_reply(packet);
    
    err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	return 0; // success
}
