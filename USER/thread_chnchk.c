#include "thread.h"
#include "fw_delay.h"
#include "fw_plc_spi.h"
#include "drv_uart485.h"
#include "fw_led_gpio.h"

// config related
extern devcfg dvcf;
// plc related
extern u8 plc_recvd;
// uart related
extern u8 uart485_buf[UARTBufferLength];
extern u8 red_state;

extern u8 plc_update;
extern u32 get_intmap_SPI(void);
extern void plc_rx_update(u32 intn_map);

u8 start_channel_check(plc_psdu *packet)
{
	u8 i;
	u8 err;
	u8 buf_crc;
	u8 buf_len;
	u8 done;
	
	u32 tot;
	u32 tx_pwr;
	
	plc_psdu packet_r;
	

	tx_pwr = (uart485_buf[6] << 24) | 
		(uart485_buf[7] << 16) | 
		(uart485_buf[8] << 8) | 
		(uart485_buf[9]);
	// tx_pwr = (uart485_buf[4 + packet->depth] << 24) | 
	// 	(uart485_buf[5 + packet->depth] << 16) | 
	// 	(uart485_buf[6 + packet->depth] << 8) | 
	// 	(uart485_buf[7 + packet->depth]);
	
	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);
	
	packet->payload[0] = (uart485_buf[6]);
	packet->payload[1] = (uart485_buf[7]);
	packet->payload[2] = (uart485_buf[8]);
	packet->payload[3] = (uart485_buf[9]);
	// packet->payload[0] = uart485_buf[4 + packet->depth];
	// packet->payload[1] = uart485_buf[5 + packet->depth];
	// packet->payload[2] = uart485_buf[6 + packet->depth];
	// packet->payload[3] = uart485_buf[7 + packet->depth];
	
	err = plc_send(packet, dvcf.totjp);
	
	if (err)
		return 1; // error
	
	// timeuot on recving
	tot = 2 * (packet->depth - 1) * dvcf.totjp;
	
	done = 0;
	
	StartIdle(tot); // NofJumps * 20 ms timeout
	while (CheckIdle() && done == 0) {
		if (plc_update) {
            plc_update = 0;
            plc_rx_update(get_intmap_SPI());
        }
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				packet_r.route[0] == dvcf.shortid && // this is source
				(packet_r.type & 0x7F) == 0x07) // this is channel check
			{
				buf_len = 2 * (packet_r.depth - 1) + 3;
				
				// set usart data
                uart485_buf[0] = 0x68;
                uart485_buf[1] = buf_len - 2;
				// set ber results
				for (i = 0; i < (buf_len - 4); i++)
					uart485_buf[2 + i] = packet_r.payload[4 + i];
				uart485_buf[buf_len - 2] = (packet_r.errbits & 0xFF);
                // generate crc
				buf_crc = 0;
                for (i = 0; i < buf_len - 2; i++)
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

u8 redirect_channel_check(plc_psdu *packet)
{
	u8 jp;
	u8 err;
	// u8 done;
	
	// u32 tot;
	u32 tx_pwr;
	
	// plc_psdu packet_r;
	
	// set tx power
	tx_pwr = (packet->payload[0] << 24) | 
		(packet->payload[1] << 16) | 
		(packet->payload[2] << 8) | 
		(packet->payload[3]);
	
	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);
	
	jp = get_current_jump(packet);
	set_target_ber(packet, jp);
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
	// tot = 2 * (packet->depth - 1 - jp) * dvcf.totjp;
	
	// done = 0;
	
	// StartIdle(tot); // NofJumps * 20 ms timeout
	// while (CheckIdle() && done == 0) {
	// 	if (plc_recvd == 1) {
	// 		plc_recv(&packet_r);
			
	// 		// only if source, dir, target, and type all right
	// 		if (packet_r.target == dvcf.shortid && // this is target
	// 			packet_r.direction == 1 && // this is reply
	// 			//packet_r.route[0] == ShortID && // this is source
	// 			(packet_r.type & 0x7F) == 0x07) // this is channel check
	// 		{
	// 			done = 1;
	// 		}
			
	// 		plc_recvd = 0;
	// 	}
	// }
	// ClearIdle();
	
	// // switch to primary frequency
	// FW_PLC_SPI_Write32Bits(CMD_FrequencyOp, dvcf.prmfreq);
	
	// if (done == 0)
	// 	return 1; // error
	
	// jp = get_current_jump(&packet_r);
	// set_target_ber(&packet_r, jp);
	// set_next_target(&packet_r, jp);
	
	// err = plc_send(&packet_r, dvcf.totjp);
	// if (err)
	// 	return 1; // error
	
	return 0; // success
}

u8 process_channel_check(plc_psdu *packet)
{
	u8 err;
	
	u32 tx_pwr;
	
	// set tx power
	tx_pwr = (packet->payload[0] << 24) | 
		(packet->payload[1] << 16) | 
		(packet->payload[2] << 8) | 
		(packet->payload[3]);
	
	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);
	
	// set destination relpy
    set_destination_reply(packet);
    packet->payload[4 + packet->depth - 2] = (packet->errbits & 0xFF);
    
    err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error
	
	return 0; // success
}
