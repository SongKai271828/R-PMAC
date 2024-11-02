#include "thread.h"
#include <stdlib.h>
#include "drv_uart485.h"
#include "fw_led_gpio.h"
#include "fw_delay.h"


extern u16 sec;
extern u32 usec;

extern u8 uart485_buf[UARTBufferLength];
extern u8 node_state[VIRTUAL_NODE_NUM];
extern u8 route_list[MAX_NODE_NUM][PHYMAXDepth];
extern u8 nconfig_node_cnt;
extern devcfg dvcf;
extern u8 short_id[VIRTUAL_NODE_NUM];
extern u8 virtual_depth[VIRTUAL_NODE_NUM];
extern u8 current_depth;
extern u8 virtual_node_num;
extern u32 csma_max_node;
extern u8 max_node_num;
extern u8 response_p;
extern u8 mcu_mac[VIRTUAL_NODE_NUM][6];
extern u8 origin_id;
extern u8 mac_list[CSMA_MAX_NODE][7];
extern u8 first_num;
extern u8 max_depth;
extern u8 simu_on;

extern u32 random_backoff_time[VIRTUAL_NODE_NUM];
extern u8 response[VIRTUAL_NODE_NUM];
extern u8 pte_id[CSMA_MAX_NODE];
extern u8 solid[VIRTUAL_NODE_NUM];

void random_response_generate(u8* response)
{
    u8 i;
    u8 online_node_num;
    u8 flag = 0;

    if (virtual_node_num > 2) {
        if (simu_on == 0) {
            online_node_num = 0;
            for (i=0; i<virtual_node_num; i++) {
                if (node_state[i] == ONLINE) online_node_num++;
            }
            if (online_node_num < virtual_node_num) flag = 1;
            // if (virtual_node_num - online_node_num < 5)
            if (virtual_node_num - online_node_num <= ((virtual_node_num/5 > 6)?6:(virtual_node_num/5)))
            {
                for (i=0; i<virtual_node_num; i++) response[i] = 1;
            }
            else {
                for (i=0; i<virtual_node_num; i++){
                    // if (node_state[i] != COLLISION && node_state[i] != ONLINE) {
                    if (node_state[i] == OFFLINE) {
                        if (flag) {
                            response[i] = 1;
                            flag = 0;
                        }
                        else {
                            response[i] = rand()%100;
                            if (response[i] < response_p) response[i] = 1;
                            else response[i] = 0;
                        }
                    }
                    else if (node_state[i] != ONLINE) {
                        response[i] = 1;
                    }
                    else if (node_state[i] == ONLINE) {
                        // response[i] = rand()%15;
                        // if (response[i] < 2) response[i] = 1;
                        // else response[i] = 0;
                        response[i] = 0;
                    }
                }
            }
        }
        else {
            for (i=0; i<virtual_node_num; i++) {
                if (current_depth == virtual_depth[i]) {
                    if (current_depth == 2) {
                        response[i] = 1;
                    }
                    else {
                        if (node_state[i] == OFFLINE) {
                            if (flag < 2) {
                                response[i] = 1;
                                flag++;
                            }
                            else {
                                response[i] = rand()%100;
                                if (response[i] < response_p) response[i] = 1;
                                else response[i] = 0;
                            }
                        }
                        else if (node_state[i] != ONLINE) {
                            response[i] = 1;
                        }
                        else {
                            response[i] = 0;
                        }
                    }
                }
                else {
                    if (current_depth > virtual_depth[i] && node_state[i] != ONLINE) {
                        virtual_depth[i] = current_depth;
                        response[i] = rand()%100;
                        if (response[i] < response_p) response[i] = 1;
                        else response[i] = 0;
                    }
                    else {
                        response[i] = 0;
                    }
                }
            }
        }
    }
    else if (virtual_node_num == 2) {
        if (response[0] == 0 && response[1] == 0) {
            response[0] = 1;
            response[1] = 0;
        }
        else if (response[0] == 1 && response[1] == 0) {
            response[1] = 1;
        }
        else if (response[1] == 0 && response[1] == 1) {
            response[1] = 1;
        }
    }
    else {
        response[0] = 1;
    }
}

u8 some_in_state(u8* state_list, u8 state)
{
    u8 i;
    for (i=0; i<virtual_node_num; i++){
        if (state_list[i] == state) return 1;
    }
    return 0;
}

// input shortid get virtual_idx
u8 get_shortid_idx(u8* short_id_list, u8 shortid)
{
    u8 i;
    for (i=0; i<virtual_node_num; i++){
        if (short_id_list[i] == shortid && shortid != 0 && node_state[i] == ONLINE) break;
    }
    return i;
}

// void random_back_off_generate(u32 *random_time_list)
// {
// 	u8 i, j, tmp;
// 	for (i=0; i<virtual_node_num; i++) {
// 		random_time_list[i] = (rand()%(csma_max_node-4)) + 3;
// 		// random_time_list[i] = 1;
// 	}
// 	for (i=0; i<virtual_node_num-1; i++) {
// 		for (j=i+1; j<virtual_node_num; j++) {
// 			if (random_time_list[i] > random_time_list[j]) {
// 				tmp = random_time_list[i];
// 				random_time_list[i] = random_time_list[j];
// 				random_time_list[j] = tmp;
// 			}
// 		}
// 	}
// }

// generate wait time randomly
void random_back_off_generate(u32 *random_time_list)
{
	u8 i, j, tmp;
    u8 pos[VIRTUAL_NODE_NUM];
    u8 pos_sum = 0;
	for (i=0; i<virtual_node_num; i++) {
        if (response[i]) {
		    random_time_list[i] = (rand()%(csma_max_node-1)) + 1;
            pos[pos_sum] = i;
            pos_sum++;
        }
	}
	for (i=0; i<pos_sum-1; i++) {
		for (j=i+1; j<pos_sum; j++) {
			if (random_time_list[pos[i]] > random_time_list[pos[j]]) {
				tmp = random_time_list[pos[i]];
				random_time_list[pos[i]] = random_time_list[pos[j]];
				random_time_list[pos[j]] = tmp;
			}
		}
	}
}

void solid_update(u8 *solid_list, u8 depth)
{
    u8 i;
    for (i=0; i<virtual_node_num; i++) {
        if (virtual_depth[i] == depth && node_state[i] == ONLINE) {
            solid_list[i] = 1;
        }
    }
}

void tic(u16* simu_sec_tic, u32* simu_usec_tic)
{
    *simu_usec_tic = usec;
    *simu_sec_tic = sec;
}

void toc(u16* simu_sec_tic, u32* simu_usec_tic)
{
    u16 tmp_sec;
    u32 tmp_usec;

    tmp_sec = sec - *simu_sec_tic;
    if (usec < *simu_usec_tic) {
        tmp_usec = usec + 1000000 - *simu_usec_tic;
        tmp_sec--;
    }
    else {
        tmp_usec = usec - *simu_usec_tic;
    }
    *simu_sec_tic = tmp_sec;
    *simu_usec_tic = tmp_usec;
}

// connect with all the nodes
void start_simulation(plc_psdu *packet)
{
    u8  no_node;
    u8 i, j;
    u32 tx_pwr;
    u8 buf_crc;
    u16 simu_sec;
    u32 simu_usec;
    u8 current_max_sid;

    current_max_sid = 0;
    PMAC_init();
    tx_pwr = (packet->payload[0] << 24) | 
			(packet->payload[1] << 16) | 
			(packet->payload[2] << 8) | 
			(packet->payload[3]);
    virtual_node_num = packet->payload[4];
    csma_max_node = packet->payload[5];
    max_node_num = packet->payload[6];
    response_p = packet->payload[7];
    plc_send(packet, dvcf.totjp);
    StartIdle(2000);
    while (CheckIdle()) {}
    ClearIdle();

    packet->type = PMAC_TYPE;
    
    tic(&simu_sec, &simu_usec);
    host_PMAC(packet, 1);
    host_PMAC(packet, 2);
    host_PMAC(packet, 3);
    host_PMAC(packet, 4);
    host_PMAC(packet, 5);
    PMAC_feedback(3);
    // PMAC_reset();
    for (i=0; i<nconfig_node_cnt; i++) {
        if (mac_list[i][6] > current_max_sid) current_max_sid = mac_list[i][6];
    }
    
    if (nconfig_node_cnt == 0) return;
    if (current_max_sid >= max_node_num) goto simulation_end;

    no_node = 0;
    for (current_depth=2; current_depth<PHYMAXDepth; current_depth++)
    {
        no_node = 1;
        PMAC_reset();
        // nconfig_node_cnt = current_depth;
        // PMAC_feedback(1);
        for (j=1; j<=max_node_num; j++) {
            // tmp = current_depth;
            packet->depth = current_depth;
            packet->direction = 0;
            packet->type = PMAC_TYPE;
            if (get_route_depth(j) == current_depth) {
                PMAC_reset();
                // PMAC_feedback(1);
                packet->route[0] = dvcf.shortid;
	            for (i = 2; i <= current_depth; i++) {packet->route[i-1] = get_route_list_unit(j, i);}
                packet->target = packet->route[1];

                packet->payload[0] = (u8)((tx_pwr&0xFF000000)>>24);
                packet->payload[1] = (u8)((tx_pwr&0x00FF0000)>>16);
                packet->payload[2] = (u8)((tx_pwr&0x0000FF00)>>8);
                packet->payload[3] = (u8)(tx_pwr&0x000000FF);
                // StartIdle(5);
                // while (CheckIdle()) {}
                // ClearIdle();
                buf_crc = start_PMAC(packet);
                PMAC_feedback(3);
                if (nconfig_node_cnt > 0) {
                    no_node = 0;
                }
                // packet->route[0] = dvcf.shortid;
                // packet->type = TIME_FEEDBACK_TYPE;
                // packet->target = packet->route[1];
                // packet->direction = 0;
                // start_time_feedback(packet);
                for (i=0; i<nconfig_node_cnt; i++) {
                    if (mac_list[i][6] > current_max_sid) current_max_sid = mac_list[i][6];
                    // packet->depth = current_depth + 1;
                    // packet->route[packet->depth-1] = mac_list[i][6];
                    // packet->route[0] = dvcf.shortid;
                    // packet->type = TIME_FEEDBACK_TYPE;
                    // packet->target = packet->route[1];
                    // packet->direction = 0;
                    // start_time_feedback(packet);
                }
                if (current_max_sid >= max_node_num) break;
            }
        }
        if (current_max_sid >= max_node_num) break;
        if (no_node) {
            break;
        }
    }

    simulation_end:

    toc(&simu_sec, &simu_usec);

    current_depth = 0;
    for (i=1; i<=max_node_num; i++) {
        if (get_route_depth(i) > current_depth) current_depth = get_route_depth(i);
    }
    
    StartIdle(200);
    while (CheckIdle()) {}
    ClearIdle();
    for (i=2; i<=current_depth; i++)
    {
        packet->payload[0] = i;
        route_feedback(packet);
        StartIdle(200);
        while (CheckIdle()) {}
        ClearIdle();
    }

	uart485_buf[0] = 0x68;
    uart485_buf[1] = 9 + 1;
	uart485_buf[2] = SIMU_TYPE;
    uart485_buf[3] = current_depth;
    uart485_buf[4] = no_node;

    uart485_buf[5] = (u8)((simu_sec>>8)&0x00FF);
	uart485_buf[6] = (u8)(simu_sec&0x00FF);
	uart485_buf[7] = (u8)((simu_usec>>24)&0x000000FF);
	uart485_buf[8] = (u8)((simu_usec>>16)&0x000000FF);
	uart485_buf[9] = (u8)((simu_usec>>8)&0x000000FF);
	uart485_buf[10] = (u8)(simu_usec&0x000000FF);

    buf_crc = 0;
	for (i = 0; i < uart485_buf[1]; i++)
		buf_crc += uart485_buf[1 + i];
	uart485_buf[uart485_buf[1] + 1] = buf_crc;
	uart485_send(uart485_buf, uart485_buf[1]+2);
}

// nodes update parameters of simulation
void process_simu(plc_psdu *packet)
{
    u8 i, depth, num_per_depth;
    u32 record, tmp;

    FW_LED_GPIO_Green(1);
    PMAC_init();
    virtual_node_num = packet->payload[4];
    csma_max_node = packet->payload[5];
    max_node_num = packet->payload[6];
    response_p = packet->payload[7];
    max_depth = packet->payload[8];
    first_num = packet->payload[9];
    current_depth = 2;
    depth = current_depth;
    for (i=0; i<first_num; i++) {
        virtual_depth[i] = depth;
    }
    num_per_depth = (virtual_node_num - first_num)/(max_depth - 2);
    for (i=first_num; i<virtual_node_num; i++) {
        if ((i-first_num)%num_per_depth == 0 && depth < max_depth) depth++;
        virtual_depth[i] = depth;
    }
    simu_on = 1;
    srand(usec *  mcu_mac[0][0] * mcu_mac[0][1] + sec + usec);

	random_response_generate(response);
	random_back_off_generate(random_backoff_time);
	record = 0;
	for (i=0; i<virtual_node_num; i++) {
		if (response[i] && !solid[i]) {
			tmp = record;
			record = random_backoff_time[i];
			random_backoff_time[i] -= tmp;
		}
		else {
			random_backoff_time[i] = 0;
			response[i] = 0;
		}
	}
	for (i=0; i<csma_max_node; i++) {
		pte_id[i] = 0xFF;
	}
    StartIdle(1000);
    while (CheckIdle()) {}
    ClearIdle();
    FW_LED_GPIO_Green(0);
}
