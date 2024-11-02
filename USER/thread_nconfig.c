#include "thread.h"
#include "fw_delay.h"
#include "fw_led_gpio.h"

extern u8 red_state;
extern u8 tquery_node_cnt;
extern u8 nconfig_node_cnt;
extern u8 plc_recvd;
extern u8 short_id[VIRTUAL_NODE_NUM];
extern u8 mcu_mac[VIRTUAL_NODE_NUM][6];
extern u8 host_state[VIRTUAL_NODE_NUM];
extern u8 node_state[VIRTUAL_NODE_NUM];
extern u8 response[VIRTUAL_NODE_NUM];
extern devcfg dvcf;
extern u8 mac_list[CSMA_MAX_NODE][7];
extern u8 ber_list[MAX_NODE_NUM];
extern id_list_unit id_list_head;

extern u8 virtual_idx;
extern u8 virtual_depth[VIRTUAL_NODE_NUM];
extern u8 solid[VIRTUAL_NODE_NUM];
extern u8 virtual_node_num;

extern u32 counters[VIRTUAL_NODE_NUM];

extern u8 plc_update;
extern u32 get_intmap_SPI(void);
extern void plc_rx_update(u32 intn_map);
extern u32 ber_wait[VIRTUAL_NODE_NUM];
extern u8 ber_wait_sum;
extern u8 ber_wait_pos[VIRTUAL_NODE_NUM];

u8 host_nconfig(plc_psdu *plc_packet, u8 node_num_idx, u8 is_final)
{
    u8 i, j;
    u8 err;
    u8 idle_id;
    u8 stop;

    if (!is_final) {
        if (node_num_idx == 0) 
            plc_packet->payload[2] = 0;
        else 
            plc_packet->payload[2] = node_num_idx * FRAME_LOAD_NCONFIG;
        
        if (tquery_node_cnt -  node_num_idx * FRAME_LOAD_NCONFIG < FRAME_LOAD_NCONFIG) 
            stop = tquery_node_cnt -  node_num_idx * FRAME_LOAD_NCONFIG;
        else 
            stop = FRAME_LOAD_NCONFIG;
    }
    else {
        if (node_num_idx == 0) 
            plc_packet->payload[2] = 0;
        else 
            plc_packet->payload[2] = node_num_idx * FRAME_LOAD_NCONFIG;

        if (nconfig_node_cnt -  node_num_idx * FRAME_LOAD_NCONFIG < FRAME_LOAD_NCONFIG) 
            stop = nconfig_node_cnt -  node_num_idx * FRAME_LOAD_NCONFIG;
        else 
            stop = FRAME_LOAD_NCONFIG;
    }

    plc_packet->type = NCONFIG_TYPE;
    plc_packet->depth = 0;
    plc_packet->direction = 0;
    plc_packet->payload[0] = stop;

    if (!is_final) {
        // plc_packet->payload[1] = 0;
        plc_packet->payload[1] = virtual_depth[virtual_idx];
        for (j=0; j<stop; j++) {
            idle_id =  mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][6];
            for (i=0; i<6; i++) plc_packet->payload[3+7*j+i] = mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][i];
            plc_packet->payload[3+7*j+6] = idle_id;
        }
    }
    else {
        plc_packet->payload[1] = virtual_depth[virtual_idx];
        for (j=0; j<stop; j++) {
            idle_id = get_idle_id(&id_list_head, 0);
            if (idle_id <= mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][6]) {
                update_id_list(&id_list_head, idle_id);
            }
            else {
                idle_id = mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][6];
            }
            add_route_list_unit(idle_id, idle_id);
            if (idle_id != mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][6]) {
                ber_list[idle_id] = ber_list[mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][6]];
                ber_list[mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][6]] = 0xFF; 
            }
            mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][6] = idle_id;
            for (i=0; i<6; i++) plc_packet->payload[3+7*j+i] = mac_list[node_num_idx*FRAME_LOAD_NCONFIG+j][i];
            plc_packet->payload[3+7*j+6] = idle_id;
        }
    }
    // StartIdle(500*2);
    // while (CheckIdle()) {}
    // ClearIdle();
    // for (i=0; i<((stop<=1)?1:2); i++) {
    for (i=0; i<1; i++) {
        err = plc_send(plc_packet, dvcf.totjp);
        StartIdle(15);
        while (CheckIdle()) {}
        ClearIdle();
    }
    return err;
}

u8 node_nconfig(plc_psdu *plc_packet)
{
    u8 i, j, k;
    u8 match;
    u8 err = 0;
    u8 ber_wait_sum_old = ber_wait_sum;
    u32 tmp;

    // FW_LED_GPIO_Green(1);
    for (j=0; j<virtual_node_num; j++) {
        if (response[j]) {
            // FW_LED_GPIO_Green(1);
        // if (node_state[j] == IN_TQUERY || node_state[j] == IN_NCONFIG) {
            for (k=0; k<plc_packet->payload[0]; k++) {
                match = 1;
                for (i=0; i<6; i++) {
                    if (plc_packet->payload[3+k*7+i] != mcu_mac[j][i]) {
                        match = 0;
                        break;
                    }
                }
                if (match && plc_packet->payload[3+k*7+6] > 0) {
#if ROUTE_UPDATE_ENABLE
                    // node_state[j] = (plc_packet->payload[1] > 0)?(ONLINE):(IN_NCONFIG);
                    node_state[j] = IN_NCONFIG;
#else
                    node_state[j] = ONLINE;
#endif
                    // FW_LED_GPIO_Green(1);
                    short_id[j] = plc_packet->payload[3+k*7+6];
                    // if (plc_packet->payload[1] == 0 && ROUTE_UPDATE_ENABLE) {
                        ber_wait[ber_wait_sum] = k + plc_packet->payload[2];
                        ber_wait_pos[ber_wait_sum] = j;
                        response[j] = 0;
                        ber_wait_sum++;
                    // }
                    // if ((plc_packet->payload[1] >= 1 && ROUTE_UPDATE_ENABLE) || (plc_packet->payload[1] == 0 && !ROUTE_UPDATE_ENABLE)) {
                        for (i=0; i<7; i++) mac_list[nconfig_node_cnt][i] = plc_packet->payload[3+k*7+i];
                        mac_list[nconfig_node_cnt][6] = short_id[j];
                        virtual_depth[j] = plc_packet->payload[1] + 1;
                        nconfig_node_cnt++;
                        response[j] = 0;
                        // FW_LED_GPIO_Green(1);
// #if ROUTE_UPDATE_ENABLE == 0
                        // solid[j] = 1;
// #endif
                    // }
                    for (i=0; i<6; i++) {plc_packet->payload[3+k*7+i] = 0;}
                    break;
				}
                else if (match && plc_packet->payload[3+k*7+6] == 0) { // run out of SID
                    node_state[j] = OFFLINE;
                    short_id[j] = 0;
                }
                // else if (node_state[j] == IN_NCONFIG && plc_packet->payload[1] >= 1) {
                //     short_id[j] = 0;
                //     node_state[j] = COLLISION;
                // }
                else if (node_state[j] == IN_TQUERY) {
                    node_state[j] = COLLISION;
                }
            }
        }
    }
#if ROUTE_UPDATE_ENABLE
    // if (ber_wait_sum > ber_wait_sum_old && plc_packet->payload[1] == 0) {
    if (ber_wait_sum > ber_wait_sum_old) {
        for (i=ber_wait_sum_old; i<ber_wait_sum-1; i++) {
            for (j=i+1; j<ber_wait_sum; j++) {
                if (ber_wait[i] > ber_wait[j]) {
                    tmp = ber_wait[i];
                    ber_wait[i] = ber_wait[j];
                    ber_wait[j] = tmp;
                    k = ber_wait_pos[i];
                    ber_wait_pos[i] = ber_wait_pos[j];
                    ber_wait_pos[j] = k;
                }
            }
        }
    }
#endif
    return err;
}
