#include "thread.h"
# include <stdlib.h>
#include "fw_plc_spi.h"
#include "fw_delay.h"
#include "fw_led_gpio.h"

extern u8 red_state;
extern u8 plc_recvd;
extern u8 mac_list[CSMA_MAX_NODE][7];
extern devcfg dvcf;
extern u8 ber_list[MAX_NODE_NUM];
extern u8 route_list[MAX_NODE_NUM][PHYMAXDepth];

extern u8 plc_update;
extern u32 get_intmap_SPI(void);
extern void plc_rx_update(u32 intn_map);

extern u32 ber_wait[VIRTUAL_NODE_NUM];
extern u8 ber_wait_pos[VIRTUAL_NODE_NUM];
extern u8 ber_wait_sum;
extern u32 counters[VIRTUAL_NODE_NUM];
extern u8 short_id[VIRTUAL_NODE_NUM];
extern u8 response[VIRTUAL_NODE_NUM];
extern u8 node_state[VIRTUAL_NODE_NUM];
extern u8 solid[VIRTUAL_NODE_NUM];
extern u8 nconfig_node_cnt;

u8 get_ber_param(plc_psdu *plc_packet)
{
    u8 max_ber;
    u8 i, length;
    length = 2 * (plc_packet->depth - 1) + 3;
    max_ber = 0;
    for (i=0; i<length-4; i++)
    {
        if (max_ber < plc_packet->payload[4+i]) max_ber = plc_packet->payload[4+i];
    }
    if (max_ber < (plc_packet->errbits & 0xFF)) max_ber = (plc_packet->errbits & 0xFF);
    return max_ber;
}

u8 host_ber_update(plc_psdu *packet, u8 ber_cnt_max)
{
    u32 tx_pwr;
    u8 err;
    u32 tot;
    plc_psdu packet_r;
    u8 tmp_ber;
    u8 ber_cnt;

    tx_pwr = PFC_POWER;
    tmp_ber = 0xFF;
    ber_cnt = 0;
    
    if (ber_cnt_max) {
	    tot = (ber_cnt_max + 2) * FRAME_SLOT_MS;
	    packet->depth = 0;
        packet->target = dvcf.shortid;
    }
    else {
	    tot = 2 * dvcf.totjp;
	    packet->depth = 2;
        packet->target = packet->route[1];
    }

	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);
    
	packet->payload[0] = (u8)((tx_pwr&0xFF000000)>>24);
	packet->payload[1] = (u8)((tx_pwr&0x00FF0000)>>16);
	packet->payload[2] = (u8)((tx_pwr&0x0000FF00)>>8);
	packet->payload[3] = (u8)(tx_pwr&0x000000FF);
    packet->payload[5] = ber_cnt_max;
	packet->route[0] = dvcf.shortid;
	packet->type = 0x07;
	packet->direction = 0;

    // FW_LED_GPIO_Green(1);
	err = plc_send(packet, dvcf.totjp);
    if (err) return 1;

	StartIdle(tot);
	while (CheckIdle()) {
        if (plc_update) {
            plc_update = 0;
            plc_rx_update(get_intmap_SPI());
        }
		if (plc_recvd == 1) {
            
			plc_recv(&packet_r);
            // if ((packet_r.type & 0x7F) == 0x07 && packet_r.direction == 1 && packet_r.depth == 2) {
            if ((packet_r.type & 0x7F) == 0x07 && packet_r.direction == 1 && packet_r.depth == 2) {
                // FW_LED_GPIO_Green(1);
                tmp_ber = (packet_r.payload[4]>(packet_r.errbits & 0xFF))?packet_r.payload[4]:(packet_r.errbits & 0xFF);
                if (ber_list[packet_r.route[1]] == 0xFF && ber_cnt < ber_cnt_max) {
                    ber_list[packet_r.route[1]] = tmp_ber;
                    // ber_list[packet_r.route[1]] = packet_r.target;
                    ber_cnt++;
                }
            }
            plc_recvd = 0;
        }
	}
    ClearIdle();
    // FW_LED_GPIO_Green(0);
    return ber_cnt;
}

u8 node_ber_update(plc_psdu *packet)
{
    u8 err;
    u32 tx_pwr;
    u32 max_wait;
    u8 j;
	
    if (ber_wait_sum == 0) return 1;
	// set tx power
	tx_pwr = (packet->payload[0] << 24) | 
		(packet->payload[1] << 16) | 
		(packet->payload[2] << 8) | 
		(packet->payload[3]);
	
	if (tx_pwr != 0x00000000)
		FW_PLC_SPI_Write32Bits(CMD_TxParamOp, tx_pwr);
	
	// set destination relpy
    packet->depth = 2;
    packet->type = 0x07;
    packet->target = packet->route[0];
    packet->direction = 1;
    packet->payload[4 + packet->depth - 2] = (packet->errbits & 0xFF);
    
    max_wait = (u32)packet->payload[5] - 1 - ber_wait[ber_wait_sum-1];
    for (j=ber_wait_sum-1; j>0; j--) {
        ber_wait[j] -= ber_wait[j-1]; 
    }
    set_counters(ber_wait, ber_wait_sum);
    for (j=0; j<ber_wait_sum; j++) {
        packet->route[1] = short_id[ber_wait_pos[j]];
        // response[ber_wait_pos[j]] = 1;   
        while(counters[j] > 0) {}
        err = plc_send(packet, dvcf.totjp);
        if (node_state[ber_wait_pos[j]] == IN_NCONFIG && err == 0) {
            node_state[ber_wait_pos[j]] = ONLINE;
            solid[ber_wait_pos[j]] = 1;
        }
        // FW_LED_GPIO_Green(1);
    }
    if (max_wait) {
        StartIdle(max_wait * FRAME_SLOT_MS);
        while (CheckIdle()) {
            if (plc_update) {
                plc_update = 0;
                plc_rx_update(get_intmap_SPI());
            }
            if (plc_recvd == 1) {
                plc_recv(packet);
                plc_recvd = 0;
            } 
        }
        ClearIdle();
    }
    // else {
        plc_rx_update(get_intmap_SPI());
        plc_recv(packet);
        plc_recvd = 0;
        plc_update = 0;
    // }
    FW_LED_GPIO_Green(0);
    ber_wait_sum = 0;
    return 0;
}

void add_route_list_unit(u8 idx, u8 next_jump)
{
    if (route_list[idx][0] + 1 == PHYMAXDepth) return;
    route_list[idx][route_list[idx][0]] = next_jump;
    route_list[idx][0]++;
}
u8 get_route_depth(u8 destination)
{
    return route_list[destination][0];
}
u8 get_route_list_unit(u8 destination, u8 depth)
{
    return route_list[destination][depth-1];
}
void free_route_list(u8 head)
{
    route_list[head][0] = 1;
}
