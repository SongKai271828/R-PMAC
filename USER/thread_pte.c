#include "thread.h"
#include "fw_plc_spi.h"
#include "drv_uart485.h"
#include "fw_delay.h"
#include "bsp.h"
#include <stdlib.h>
#include <math.h>

// uart related
extern u8 uart485_buf[UARTBufferLength];

// PTE related
extern u8 hour, day, red_state;
extern u16 sec;
extern u32 usec;
extern u32 start_time[2];
extern u32 end_time[2];
extern u32 delta_usec[CSMA_MAX_NODE];

extern u32 delta_usec_node[VIRTUAL_NODE_NUM];
extern u32 random_backoff_time[VIRTUAL_NODE_NUM];
extern u8 response[VIRTUAL_NODE_NUM];
extern u8 short_id[VIRTUAL_NODE_NUM];
extern u8 host_state[VIRTUAL_NODE_NUM];
extern u8 node_state[VIRTUAL_NODE_NUM];

// csma related
extern u8 csma_node_cnt;

// plc related
extern devcfg dvcf;
extern u8 plc_recvd;
extern u8 pfc_recvd;
extern u8 plc_update;

extern devcfg dvcf;

extern u8 mac_list[CSMA_MAX_NODE][7];

extern id_list_unit id_list_head;
extern id_list_unit* id_list_tail;
extern u8 route_list[MAX_NODE_NUM][PHYMAXDepth];
extern u8 ber_list[MAX_NODE_NUM];


extern u8 tquery_node_cnt;
extern u8 nconfig_node_cnt;

extern u8 virtual_idx;
extern u8 solid[VIRTUAL_NODE_NUM];
extern u8 virtual_node_num;
extern u32 csma_max_node;
extern u8 mcu_mac[VIRTUAL_NODE_NUM][6];
extern u8 ber_wait_sum;
extern u8 pte_id[CSMA_MAX_NODE];
extern u8 flag;

extern void plc_prm_rx_update(u32 intn_map);
extern void plc_rx_update(u32 intn_map);
extern u32 get_intmap_SPI(void);

u8 start_pte(plc_psdu *packet)
{
	u8 err;
	
	u32 tx_pwr;
	
	tx_pwr = (packet->payload[0] << 24) | 
			(packet->payload[1] << 16) | 
			(packet->payload[2] << 8) | 
			(packet->payload[3]);
	
	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);
	
	err = plc_send_pfc(packet, 1);
	if (err)
		return 1; // error
	
	return 0; // success
}

u8 host_PTE(u32 csma_tot_us, u32 tx_pwr)
{
	u8 err;
	
	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);

	
	err = plc_send_PFC(PFC_NET, PFC_TOT_US);
	if (err != 0) return 1;

	get_time_record();

	// StartIdle(csma_tot_ms);
	StartIdle_us(csma_tot_us + 10 * PREAMBLE_US);
	while (CheckIdle()) {
		if (plc_update) {
			plc_update = 0;
			plc_prm_rx_update(get_intmap_SPI());
		} 
		if (pfc_recvd == PFC_REQ+1) {
			get_delta_time(delta_usec, csma_node_cnt);
			delta_usec[csma_node_cnt] += DT_COMPENSATE;
			csma_node_cnt++;
			host_state[virtual_idx] = IN_TQUERY;
			
			pfc_recvd = 0;
		}
	}
	ClearIdle();

	return 0;
}

u8 get_time_record(void)
{
	end_time[0] = usec;
	end_time[1] = sec;
	start_time[0] = end_time[0];
	start_time[1] = end_time[1];
	return 0;
}

void get_delta_time(u32 *delta_us, u8 csma_node_idx)
{
	end_time[0] = usec;
	end_time[1] = sec;

	delta_us[csma_node_idx] = end_time[1]*1000000+end_time[0]-(start_time[1]*1000000+start_time[0]);
	// delta_s[csma_node_idx] = delta_usec[csma_node_idx]/1000000;
}

u8 node_PTE(void)
{
	u8 err, i;
	u32 record, tmp;

	get_time_record();
    // srand(usec *  mcu_mac[0][0] * mcu_mac[0][1] + sec);
	// random_response_generate(response);
	// random_back_off_generate(random_backoff_time);
	// record = 0;
	// for (i=0; i<virtual_node_num; i++) {
	// 	if (response[i] && !solid[i]) {
	// 		tmp = record;
	// 		record = random_backoff_time[i];
	// 		random_backoff_time[i] -= tmp;
	// 	}
	// 	else {
	// 		random_backoff_time[i] = 0;
	// 		response[i] = 0;
	// 	}
	// }
	// for (i=0; i<csma_max_node; i++) {
	// 	pte_id[i] = 0xFF;
	// }
	// FW_LED_GPIO_Green(1);
	csma_node_cnt = 0;
	tquery_node_cnt = 0;
	nconfig_node_cnt = 0;
	ber_wait_sum = 0;
	for (i=0; i<virtual_node_num; i++) {
		if (random_backoff_time[i] > 1 && response[i] == 1) {
			err = csma_cd(random_backoff_time[i] - 1);
			get_delta_time(delta_usec_node, i);
			csma_node_cnt++;
			node_state[i] = IN_PTE;
			tmp = (delta_usec_node[i]+PREAMBLE_US/2)/PREAMBLE_US;
			if (tmp < csma_max_node) {
				pte_id[(u8)(tmp)] = i;
			}
			else {
				response[i] = 0;
				delta_usec_node[i] = 0;
				node_state[i] = COLLISION;
			}
		}
		else {
			if (response[i] == 1 && random_backoff_time[i] <= 1 && !solid[i]) node_state[i] = COLLISION;
			response[i] = 0;
			delta_usec_node[i] = 0;
		}
	}
	get_delta_time(&record, 0);
	while (record < csma_max_node * PREAMBLE_US) {
		if (plc_update == 1)
		{
			plc_update = 0;
			plc_prm_rx_update(get_intmap_SPI());
		}
		if (pfc_recvd > 0){
			pfc_recvd = 0;
		}
		get_delta_time(&record, 0);
	}
	flag = 0;
	// plc_prm_rx_update(get_intmap_SPI());
	// pfc_recvd = 0;
	// plc_update = 0;
	// FW_LED_GPIO_Green(0);
	return err;
}

u8 process_host_PTE(plc_psdu *packet)
{
	u8 err;
	u8 i, j;
	plc_psdu plc_packet;

	if (packet->direction == 0 && (packet->type & 0x7F) == PMAC_TYPE) {
		// if (host_state[virtual_idx] == OFFLINE) {
			FW_LED_GPIO_Green(1);
			PMAC_reset();
			for (i=0; i<4; i++) plc_packet.payload[i] = packet->payload[i];
			host_PMAC(&plc_packet, 1);
			if (csma_node_cnt > 0) host_state[virtual_idx] = IN_TQUERY;
		// }
		packet->payload[0] = host_state[virtual_idx];
		packet->payload[1] = csma_node_cnt;
		packet->type = PMAC_TYPE;
		set_destination_reply(packet);
		err = plc_send(packet, dvcf.totjp);
		
		FW_LED_GPIO_Green(0);
	}
	else if (packet->direction == 0 && (packet->type & 0x7F) == TQUERY_TYPE) {
		if (host_state[virtual_idx] == IN_TQUERY) {
			host_PMAC(&plc_packet, 2);
			if (tquery_node_cnt > 0) host_state[virtual_idx] = IN_NCONFIG;
			else host_state[virtual_idx] = OFFLINE;
			nconfig_node_cnt = 0;
		}
		else if (host_state[virtual_idx] == IN_NCONFIG) {
			if (nconfig_node_cnt == packet->payload[0]) {
				for (i=0; i<packet->payload[1]; i++) {
					mac_list[nconfig_node_cnt-packet->payload[1]+i][6] = packet->payload[2+i];
				}
				if (nconfig_node_cnt == tquery_node_cnt) goto start_nconfig;
			}
			else if (nconfig_node_cnt > packet->payload[0])
			{
				nconfig_node_cnt = packet->payload[0];
			}
		}

		packet->type = TQUERY_TYPE;
		packet->payload[1] = 0;
		packet->payload[0] = 0;
		set_destination_reply(packet);
		for (i=0; i<(PHYPayloadLength-2)/7; i++) {
			if (nconfig_node_cnt < tquery_node_cnt) {
				for (j=0; j<7; j++) packet->payload[2+7*i+j] = mac_list[nconfig_node_cnt][j];
				nconfig_node_cnt++;
				packet->payload[0] += 1;
			}
			if (nconfig_node_cnt == tquery_node_cnt) {
				packet->payload[1] = 1;
				// FW_LED_GPIO_Green(0);
				break;
			}
		}
		err = plc_send(packet, dvcf.totjp);
	} 
	else if (packet->direction == 0 && (packet->type & 0x7F) == NCONFIG_TYPE) {
		start_nconfig:
		if (host_state[virtual_idx] == IN_NCONFIG) {
			host_PMAC(&plc_packet, 4);
			if (nconfig_node_cnt > 0) host_state[virtual_idx] = ONLINE;
			// if (nconfig_node_cnt != tquery_node_cnt) FW_LED_GPIO_Green(1);
			tquery_node_cnt = nconfig_node_cnt;
			nconfig_node_cnt = 0;
		}
		else {
			nconfig_node_cnt = packet->payload[0];
		}
		packet->type = NCONFIG_TYPE;
		packet->payload[1] = 0;
		packet->payload[0] = 0;
		set_destination_reply(packet);
		for (i=0; i<(PHYPayloadLength-2)/2; i++) {
			if (nconfig_node_cnt < tquery_node_cnt) {
				packet->payload[2+i*2+0] = mac_list[nconfig_node_cnt][6];
				packet->payload[2+i*2+1] = ber_list[mac_list[nconfig_node_cnt][6]];
				packet->payload[0] += 1;
				nconfig_node_cnt++;
			}
			if (nconfig_node_cnt == tquery_node_cnt) {
				packet->payload[1] = 1;
				// FW_LED_GPIO_Green(0);
				break;
			}
		}
		err = plc_send(packet, dvcf.totjp);
	}
	return err;
}

u8 redirect_host_PTE(plc_psdu *packet)
{
	u8 jp;
	u8 err;
	
	// u32 tot;

	// timeuot on recving
	// tot = 2 * (packet->depth - 1 - jp) * dvcf.totjp;
	// tot += 2000;
	
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

	return err;
}

u8 csma_cd(u32 wait)
{
	u8 err;
	u32 tx_pwr;
	
#if COMPENSATE_TEST==0
	if (wait) {
		StartIdle_us(PREAMBLE_US*wait);
		while (CheckIdle()) {}
		ClearIdle();
	}
#endif	
	tx_pwr = PFC_POWER;
	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);
	err = plc_send_PFC(PFC_REQ, PFC_TOT_US);

	return err;
}
