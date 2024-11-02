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
extern u32 start_time[4];
extern u32 end_time[4];
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
extern u8 plc_busy;
extern u8 mcu_mac[VIRTUAL_NODE_NUM][6];

extern devcfg dvcf;

extern u8 tquery_node_cnt;
extern u8 nconfig_node_cnt;
extern u8 mac_list[CSMA_MAX_NODE][7];

extern id_list_unit id_list_head;
extern id_list_unit* id_list_tail;
extern u8 route_list[MAX_NODE_NUM][PHYMAXDepth];

extern u8 virtual_idx;

extern u8 ber_list[MAX_NODE_NUM];
extern u8 solid[VIRTUAL_NODE_NUM];
extern u8 virtual_depth[VIRTUAL_NODE_NUM];
extern u8 current_depth;
extern u8 origin_id;
extern u8 virtual_node_num;
extern u32 csma_max_node;
extern u8 max_node_num;
extern u8 response_p;

extern u8 plc_update;
extern u32 get_intmap_SPI(void);
extern void plc_rx_update(u32 intn_map);
extern u32 ber_wait[VIRTUAL_NODE_NUM];
extern u8 ber_wait_sum;
extern u8 pte_id[CSMA_MAX_NODE];

extern u8 tot_type;

u8 start_PMAC(plc_psdu *packet)
{
    u8 err;
    u32 tot, tmp;
    u8 done;
    plc_psdu packet_r;
    u8 i, j;
	u8 tmp_ber;
	// u8 lost_cnt = 0;
	u8 tot_cnt = 0;
	
	packet->type = PMAC_TYPE;
	packet->direction = 0;
	packet->target = packet->route[1];
	err = 1;
	while (err) {err = plc_send(packet, dvcf.totjp);}

    done = 0;
	tot = 2 * (packet->depth - 1) * dvcf.totjp * 1000 + (csma_max_node + 10) * PREAMBLE_US;
	StartIdle_us(tot);
	while (CheckIdle() && done == 0) 
	{
		if (plc_update) {
    		plc_update = 0;
            plc_rx_update(get_intmap_SPI());
	    }
        if (plc_recvd == 1) {
			plc_recv(&packet_r);
			FW_LED_GPIO_Green(1);
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && packet_r.direction == 1 && 
				packet_r.depth == packet->depth && packet_r.route[0] == dvcf.shortid && (packet_r.type & 0x7F) == PMAC_TYPE)
            {
				
                if (packet_r.payload[0] == IN_TQUERY) {
					csma_node_cnt = packet_r.payload[1];
				}
				done = 1;
            }
			plc_recvd = 0;
		}
	}
    ClearIdle();
	if (!done || !csma_node_cnt) {
		if (!done) {
			csma_node_cnt = 0xFF;
		}
		FW_LED_GPIO_Green(0);
		return 1;
	}
	// PMAC_feedback(1);
	packet->type = TQUERY_TYPE;
	packet->payload[0] = tquery_node_cnt;
	done = 0;
	tmp = csma_node_cnt;
	tot = 2*(packet->depth-1)*dvcf.totjp + (tmp + 7 + (tmp+FRAME_LOAD_TQUERY-1)/FRAME_LOAD_TQUERY*2*2) * FRAME_SLOT_MS;
	tot_type = 0x00;
	tquery_repeat:
	err = 1;
	while (err) {err = plc_send(packet, dvcf.totjp);}
	StartIdle(tot);
	for (i=0; i<csma_max_node; i++) {mac_list[i][6] = get_idle_id(&id_list_head, i);}
	while (CheckIdle() && done == 0)
    {
		if (plc_update) {
            plc_update = 0;
            plc_rx_update(get_intmap_SPI());
        }
        if (plc_recvd == 1) {
			plc_recv(&packet_r);
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && packet_r.direction == 1 && 
				packet_r.depth == packet->depth && packet_r.route[0] == dvcf.shortid && (packet_r.type & 0x7F) == TQUERY_TYPE)
            {
                if (packet_r.payload[1] == 1 || packet_r.payload[0] == 0) {
					done = 1;
				}
				for (i=0; i<packet_r.payload[0]; i++) {
					for (j=0; j<6; j++) mac_list[tquery_node_cnt][j] = packet_r.payload[2+7*i+j];
					if (packet_r.payload[2+7*i+6] != 0) {
						mac_list[tquery_node_cnt][6] = packet_r.payload[2+7*i+6];
					}
					packet->payload[2+i] = mac_list[tquery_node_cnt][6];
					tquery_node_cnt++;
				}
				packet->payload[0] = tquery_node_cnt;
				packet->payload[1] = packet_r.payload[0];
				ClearIdle();
				tot = 2 * (packet->depth - 1) * dvcf.totjp;
				err = 1;
				while (err) {err = plc_send(packet, dvcf.totjp);}
				StartIdle(tot);
            }
			plc_recvd = 0;
		}
    }
    ClearIdle();
	if (!done) {
		tot_type += 0x10;
		tot_cnt++;
		if (tquery_node_cnt == 0 && tot_cnt > 1) {// tiemout 2 times, return
			return 1;
		}
		if (tot_cnt > 1) { // tiemout 2 times, return
			tquery_node_cnt = 0xFF;
			csma_node_cnt = 0xFF;
			return 1; 
		}
		goto tquery_repeat;
	}
	if (tquery_node_cnt == 0) { // no nodes in TQUERY
		FW_LED_GPIO_Green(0);
		return 1;
	}
	// PMAC_feedback(2);

	done = 0;
	tmp = tquery_node_cnt;
	tot = 2*(packet->depth-1) * dvcf.totjp + (tmp + 7 + (tmp+FRAME_LOAD_NCONFIG-1)/FRAME_LOAD_NCONFIG*2*2) * FRAME_SLOT_MS;
	tot_cnt = 0;
	packet->type = NCONFIG_TYPE;
	packet->payload[0] = nconfig_node_cnt;
	nconfig_repeat:
	StartIdle(tot); // NofJumps * 20 ms timeout
	while (CheckIdle() && done == 0) 
    {
		if (plc_update) {
            plc_update = 0;
            plc_rx_update(get_intmap_SPI());
        }
        if (plc_recvd == 1) {
			plc_recv(&packet_r);
			// only if source, dir, target, and type all right
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && packet_r.depth == packet->depth && packet_r.route[0] == dvcf.shortid) // this is reply 
            {
                if ((packet_r.type & 0x7F) == NCONFIG_TYPE) {
					if (packet_r.payload[1]) {
						done = 1;
					}
					else {
						packet->type = NCONFIG_TYPE;
						packet->direction = 0;
						packet->target = packet->route[1];
						packet->payload[0] = nconfig_node_cnt + packet_r.payload[0];
						ClearIdle();
						tot = 2 * (packet->depth - 1) * dvcf.totjp;
						err = plc_send(packet, dvcf.totjp);
						StartIdle(tot);
					}
					for (i=0; i<packet_r.payload[0]; i++) {
						// while (mac_list[nconfig_node_cnt+lost_cnt][6] != packet_r.payload[2+2*i+0]) {
						// 	mac_list[nconfig_node_cnt+lost_cnt][6] = 0;
						// 	lost_cnt++;
						// }
						mac_list[nconfig_node_cnt][6] = packet_r.payload[2+2*i+0];
						if (get_route_depth(packet_r.payload[2+2*i+0]) > 1) {
							tmp_ber = packet_r.payload[2+2*i+1];
							tmp_ber = (tmp_ber > ber_list[packet_r.route[packet_r.depth-1]])?tmp_ber:ber_list[packet_r.route[packet_r.depth-1]];
							if (ber_list[packet_r.payload[2+2*i+0]] > tmp_ber) {
								free_route_list(packet_r.payload[2+2*i+0]);
								for (j=1; j<packet_r.depth; j++) {
									add_route_list_unit(packet_r.payload[2+2*i+0], packet_r.route[j]);
								}
								add_route_list_unit(packet_r.payload[2+2*i+0], packet_r.payload[2+2*i+0]);
								ber_list[packet_r.payload[2+2*i+0]] = tmp_ber;
							}
						}
						else {
							for (j=1; j<packet_r.depth; j++) {
								add_route_list_unit(packet_r.payload[2+2*i+0], packet_r.route[j]);
							}
							update_id_list(&id_list_head, packet_r.payload[2+2*i+0]);
							add_route_list_unit(packet_r.payload[2+2*i+0], packet_r.payload[2+2*i+0]);
							ber_list[packet_r.payload[2+2*i+0]] = packet_r.payload[2+2*i+1];
						}
						nconfig_node_cnt++;
					}					
				}
            }
			plc_recvd = 0;
		}
    }
    ClearIdle();
	if (!done) {
		tot_type += 0x01;
		tot_cnt++;
		if (tot_cnt > 1){
			csma_node_cnt = 0xFF;
			tquery_node_cnt = 0XFF;
			nconfig_node_cnt = 0XFF;
			return 1;
		}
		err = plc_send(packet, dvcf.totjp);
		goto nconfig_repeat;
	}
	// csma_node_cnt = 0;
	// tquery_node_cnt = 1;
	// PMAC_feedback(3);
	FW_LED_GPIO_Green(0);
    return 0;
}

u8 host_PMAC(plc_psdu *packet, u8 type)
{
	u32 csma_tot_us = csma_max_node*PREAMBLE_US;
	u32 tx_pwr;
	u8 err;
	u8 i, j;
	
	err = 0;
	if (type == 1) {
		tx_pwr = (packet->payload[0] << 24) | 
				(packet->payload[1] << 16) | 
				(packet->payload[2] << 8) | 
				(packet->payload[3]);

		host_PTE(csma_tot_us, tx_pwr);
		// PMAC_feedback(1);
	}
	else if (type == 2) {
		if (csma_node_cnt) {
			for (i=0; i<(csma_node_cnt/FRAME_LOAD_TQUERY + ((csma_node_cnt%FRAME_LOAD_TQUERY == 0)?0:1)); i++) {
				host_tquery(packet, i);
			}
			// PMAC_feedback(2);
		}
	}
	else if (type == 3) {
		for (i=0; i<tquery_node_cnt; i++) {
			if (mac_list[i][6] == 0) {
                mac_list[i][6] = get_idle_id(&id_list_head, i);
            }
		}
	}
	else if (type == 4) {
		if (tquery_node_cnt) {
			for (j=0; j<(tquery_node_cnt/FRAME_LOAD_NCONFIG + ((tquery_node_cnt%FRAME_LOAD_NCONFIG == 0)?0:1)); j++) {
				err = host_nconfig(packet, j, 0);
			}
			nconfig_node_cnt = 0;
			// csma_node_cnt = 0;
			host_ber_update(packet, tquery_node_cnt);
			for (i=0; i<tquery_node_cnt; i++) {
				if (ber_list[mac_list[i][6]] == 0xFF) {
					packet->route[1] = mac_list[i][6];
					if (host_ber_update(packet, 0) == 1) {
						csma_node_cnt++;
						// PMAC_feedback(1);
						if (i != nconfig_node_cnt) {
							for (j=0; j<7; j++) mac_list[nconfig_node_cnt][j] = mac_list[i][j];
							ber_list[mac_list[nconfig_node_cnt][6]] = ber_list[mac_list[i][6]];
							ber_list[mac_list[i][6]] = 0xFF;
						}
						nconfig_node_cnt++;
					}
				}
				else {
					if (i != nconfig_node_cnt) {
						for (j=0; j<7; j++) mac_list[nconfig_node_cnt][j] = mac_list[i][j];
						ber_list[mac_list[nconfig_node_cnt][6]] = ber_list[mac_list[i][6]];
						ber_list[mac_list[i][6]] = 0xFF;
					}
					nconfig_node_cnt++;
				}
			}
		}
	}
	else if (type == 5) {
		for (i=0; i<nconfig_node_cnt; i++) {
			add_route_list_unit(mac_list[i][6], mac_list[i][6]);
			update_id_list(&id_list_head, mac_list[i][6]);
		}
	}
	return err;
}

u8 PMAC_feedback(u8 out_type)
{
	u8 i;
	u8 buf_crc;

	uart485_buf[0] = 0x68;
	uart485_buf[2] = tot_type;
	uart485_buf[3] = csma_node_cnt;
	uart485_buf[4] = tquery_node_cnt;
	uart485_buf[5] = nconfig_node_cnt;

	if (out_type == 1) {
		uart485_buf[1] = 5+3*csma_node_cnt;
		for (i=0; i<csma_node_cnt; i++) {
			if (i*3+2+6 >= UARTBufferLength - 1) {
				uart485_buf[1] = 5+3*i;
				break;
			}
			uart485_buf[i*3+0+6] = (u8)((delta_usec[i]>>16)&0x000000FF);
			uart485_buf[i*3+1+6] = (u8)((delta_usec[i]>>8)&0x000000FF);
			uart485_buf[i*3+2+6] = (u8)(delta_usec[i]&0x000000FF);
		}
		// uart485_buf[1] = 4+6*csma_node_cnt;
		// for (i=0; i<csma_node_cnt; i++) {
		// 	uart485_buf[i*6+0+5] = 0;
		// 	uart485_buf[i*6+1+5] = (u8)((delta_usec[i]+PREAMBLE_US/2)/PREAMBLE_US);
		// 	uart485_buf[i*6+2+5] = (u8)((delta_usec[i]>>24)&0x000000FF);
		// 	uart485_buf[i*6+3+5] = (u8)((delta_usec[i]>>16)&0x000000FF);
		// 	uart485_buf[i*6+4+5] = (u8)((delta_usec[i]>>8)&0x000000FF);
		// 	uart485_buf[i*6+5+5] = (u8)(delta_usec[i]&0x000000FF);
		// }
	}
	if (out_type == 2) {
		uart485_buf[1] = 5+3*tquery_node_cnt;
		for (i=0; i<tquery_node_cnt; i++) {
			if (i*3+2+6 >= UARTBufferLength - 1) {
				uart485_buf[1] = 5+ i * 3; 
				break;
			}
			uart485_buf[i*3+0+6] = (u8)(mac_list[i][6]);
			uart485_buf[i*3+1+6] = (u8)(mac_list[i][0]);
			uart485_buf[i*3+2+6] = (u8)(mac_list[i][5]);
		}
		// uart485_buf[1] = 4+7*tquery_node_cnt;
		// for (i=0; i<tquery_node_cnt; i++) {
			// uart485_buf[i*7+0+5] = mac_list[i][6];
			// uart485_buf[i*7+1+5] = (u8)(mac_list[i][0]);
			// uart485_buf[i*7+2+5] = (u8)(mac_list[i][1]);
			// uart485_buf[i*7+3+5] = (u8)(mac_list[i][2]);
			// uart485_buf[i*7+4+5] = (u8)(mac_list[i][3]);
			// uart485_buf[i*7+5+5] = (u8)(mac_list[i][4]);
			// uart485_buf[i*7+6+5] = (u8)(mac_list[i][5]);
		// }
	}
	if (out_type == 3) {
		uart485_buf[1] = 5+3*nconfig_node_cnt;
		for (i=0; i<nconfig_node_cnt; i++) {
			if (i*3+2+6 >= UARTBufferLength - 1) {
				uart485_buf[1] = 5 + 3 * i; 
				break;
			}
			uart485_buf[i*3+0+6] = mac_list[i][6];
			uart485_buf[i*3+1+6] = (u8)(mac_list[i][0]);
			uart485_buf[i*3+2+6] = (u8)(mac_list[i][5]);
			// uart485_buf[i*3+2+5] = ber_list[mac_list[i][6]];
		}
		// uart485_buf[1] = 4+4*nconfig_node_cnt;
		// for (i=0; i<nconfig_node_cnt; i++) {
		// 	uart485_buf[i*4+0+5] = mac_list[i][6];
		// 	uart485_buf[i*4+1+5] = (u8)(mac_list[i][0]);
		// 	uart485_buf[i*4+2+5] = (u8)(mac_list[i][5]);
		// 	uart485_buf[i*4+3+5] = ber_list[mac_list[i][6]];
		// }
	}
	
	// for (i=0; i<nconfig_node_cnt; i++) {
	// 	uart485_buf[i*2+0+5] = mac_list[i][6];
	// 	uart485_buf[i*2+1+5] = ber_list[mac_list[i][6]];
	// }

	buf_crc = 0;
	for (i = 0; i < uart485_buf[1]; i++)
		buf_crc += uart485_buf[1 + i];
	uart485_buf[uart485_buf[1] + 1] = buf_crc;
	uart485_send(uart485_buf, uart485_buf[1]+2);
	
	return 0;
}

u8 route_feedback(plc_psdu* packet)
{
    u8 cnt;
	u8 buf_crc;
    u16 i;
	u8 depth;

	depth = packet->payload[0];
	uart485_buf[0] = 0x68;
	uart485_buf[2] = ROUTE_TYPE;

	cnt = 0;
	for (i=1; i<=max_node_num; i++) {
		if (get_route_depth(i) == depth) {
			cnt++;
		}
	}

	uart485_buf[1] = 4;
	uart485_buf[3] = depth;
	uart485_buf[4] = cnt;
	buf_crc = 0;
	for (i = 0; i < uart485_buf[1]; i++)
		buf_crc += uart485_buf[1 + i];
	uart485_buf[uart485_buf[1] + 1] = buf_crc;
	uart485_send(uart485_buf, uart485_buf[1]+2);
	
	return 0;
}

void PMAC_init(void)
{
    u16 i, j;
	
	dvcf.shortid = 0;
	get_mcu_mac(mcu_mac[0]);
	for (i=1; i<VIRTUAL_NODE_NUM; i++) {
        for (j=0; j<6; j++) {
			mcu_mac[i][j] = mcu_mac[0][j];
		}
		mcu_mac[i][5] += i;
		ber_wait[i] = 0;
    }
	ber_wait_sum = 0;

    for (i=0; i<MAX_NODE_NUM; i++) {
        for (j=1; j<PHYMAXDepth; j++) route_list[i][j] = 0;
		ber_list[i] = 255;
		route_list[i][0] = 1;
    }
	// usec = 0;
	// sec = 0;
	// hour = 0;
	// day = 0;
	// red_state = 1;
	
	csma_node_cnt = 0;
	start_time[0] = 0;
	start_time[1] = 0;
	end_time[0] = 0;
	end_time[1] = 0;
	tot_type = 0;
	
	delta_usec[csma_node_cnt] = 0;

	tquery_node_cnt = 0;
	nconfig_node_cnt = 0;

	for (i=0; i<VIRTUAL_NODE_NUM; i++) {
		short_id[i] = 0;
		host_state[i] = OFFLINE;
		node_state[i] = OFFLINE;
		delta_usec_node[i] = 0;
		solid[i] = 0;
		virtual_depth[i] = 1;
	}

	for (i=0; i<CSMA_MAX_NODE; i++)
	{
		pte_id[i] = 0xFF;
		for (j=0; j<7; j++) mac_list[i][j] = 0;
	}
	

	id_list_free(&id_list_head);
	id_list_head.next = NULL;
	id_list_tail = &id_list_head;
	if (dvcf.nodetype == 0) {
		id_list_init(&id_list_head);
		id_list_tail = id_list_head.next;
	}

	virtual_idx = 0;
}

u8 start_time_feedback(plc_psdu *packet)
{
	u32 tot;
	u8 err;
	u8 i;
	u8 done, buf_crc;
	plc_psdu packet_r;

	tot = 4 * (packet->depth - 1) * dvcf.totjp;
	err = plc_send(packet, dvcf.totjp);
	if (err)
		return 1; // error

    uart485_buf[0] = 0x68;
    uart485_buf[1] = 0x08+(packet->depth+1)+1+1+1;
    uart485_buf[2] = TIME_FEEDBACK_TYPE;
    for (i=0; i<packet->depth; i++) {
		uart485_buf[13+i] = packet->route[i];
	}

	done = 0;
	StartIdle(tot);
	while (CheckIdle() && done == 0) {
		if (plc_update) {
            plc_update = 0;
            plc_rx_update(get_intmap_SPI());
        }
		if (plc_recvd == 1) {
			plc_recv(&packet_r);
			if (packet_r.target == dvcf.shortid && // this is target
				packet_r.direction == 1 && // this is reply
				packet_r.route[0] == dvcf.shortid && // this is source
				(packet_r.type & 0x7F) == TIME_FEEDBACK_TYPE)
			{
				uart485_buf[0] = 0x68;
				uart485_buf[1] = 0x08+(packet_r.depth+1)+4+packet_r.payload[10];
				uart485_buf[2] = TIME_FEEDBACK_TYPE;
				uart485_buf[3] = packet_r.payload[0];
				uart485_buf[4] = packet_r.payload[1];
				uart485_buf[5] = packet_r.payload[2];
				uart485_buf[6] = packet_r.payload[3];
				uart485_buf[7] = packet_r.payload[4];
				uart485_buf[8] = packet_r.payload[5];
				uart485_buf[9] = packet_r.type;
				uart485_buf[10] = packet_r.payload[6];
				uart485_buf[11]= packet_r.payload[7];
				uart485_buf[12]= packet_r.payload[8];
				uart485_buf[13]= packet_r.payload[9];
				for (i=0; i<packet_r.payload[10]; i++) uart485_buf[14+i] = packet_r.payload[11+i];
				for (i=0; i<packet_r.depth; i++) {
					uart485_buf[uart485_buf[1]+1-packet_r.depth+i] = packet_r.route[i];
				}
				buf_crc = 0;
				for (i = 0; i < uart485_buf[1]; i++)
					buf_crc += uart485_buf[1 + i];
				uart485_buf[uart485_buf[1] + 1] = buf_crc;
			    done = 1;
			}
			plc_recvd = 0;
		}
	}
	ClearIdle();
	
	if (done == 0)
		return 1; // error
	
	uart485_send(uart485_buf, uart485_buf[1]+2);

	return 0;
}

u8 process_time_feedback(plc_psdu *packet)
{
	u8 err;
	u8 idx;
	u8 i, j;
	for (idx=0; idx<virtual_node_num; idx++) {
		if (packet->route[packet->depth-1] == short_id[idx]) break;
	}
	set_destination_reply(packet);
	
	// packet->type = TIME_FEEDBACK_TYPE;
	// packet->payload[0] = (u8)((delta_sec_node[idx]>>8)&0x00FF);
	// packet->payload[1] = (u8)(delta_sec_node[idx]&0x00FF);
	packet->payload[0] = 0;
	packet->payload[1] = 0;
	packet->payload[2] = (u8)((delta_usec_node[idx]>>24)&0x000000FF);
	packet->payload[3] = (u8)((delta_usec_node[idx]>>16)&0x000000FF);
	packet->payload[4] = (u8)((delta_usec_node[idx]>>8)&0x000000FF);
	packet->payload[5] = (u8)(delta_usec_node[idx]&0x000000FF);
	// packet->payload[0] = (u8)(mcu_mac[virtual_idx][0]);
	// packet->payload[1] = (u8)(mcu_mac[virtual_idx][1]);
	// packet->payload[2] = (u8)(mcu_mac[virtual_idx][2]);
	// packet->payload[3] = (u8)(mcu_mac[virtual_idx][3]);
	// packet->payload[4] = (u8)(mcu_mac[virtual_idx][4]);
	// packet->payload[5] = (u8)(mcu_mac[virtual_idx][5]);

	// packet->payload[6] = dvcf.shortid;
	// packet->payload[6] = origin_id;
	// packet->payload[7] = host_state[virtual_idx];
	// packet->payload[8] = node_state[virtual_idx];
	packet->payload[6] = csma_node_cnt;
	packet->payload[7] = tquery_node_cnt;
	packet->payload[8] = nconfig_node_cnt;
	packet->payload[9] = origin_id;
	packet->payload[10] = 3*nconfig_node_cnt;
	for (i=0; i<nconfig_node_cnt; i++) {
		for (j=0; j<virtual_node_num; j++) {
			if (short_id[j] == mac_list[i][6] && node_state[j] == IN_NCONFIG) {
				mac_list[i][6] = 0xDD;
			}
		}
		
	}
	for (i=0; i<nconfig_node_cnt; i++) {
		packet->payload[11+i*3+0] = mac_list[i][6];
		packet->payload[11+i*3+1] = mac_list[i][0];
		packet->payload[11+i*3+2] = mac_list[i][5];
	}
	err = plc_send(packet, dvcf.totjp);
	if (err) return 1; // error
	return 0;
}

u8 redirect_time_feedback(plc_psdu *packet)
{
	u8 jp;
	u8 err;
	
	// u32 tot;
	
	
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
	// 			(packet_r.type & 0x7F) == TIME_FEEDBACK_TYPE) // this is device config
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
	// set_next_target(&packet_r, jp);
	
	// err = plc_send(&packet_r, dvcf.totjp);
    
	return err;
}

void PMAC_reset(void)
{
    csma_node_cnt = 0;
    tquery_node_cnt = 0;
    nconfig_node_cnt = 0;
}
