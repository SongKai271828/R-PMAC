#include "thread.h"
#include "fw_delay.h"
#include "fw_led_gpio.h"
#include "fw_plc_spi.h"
#include <math.h>

extern u8 red_state;
extern u8 mac_list[CSMA_MAX_NODE][7];
extern u8 pte_id[CSMA_MAX_NODE];
extern u8 tquery_node_cnt;
extern u8 csma_node_cnt;
extern u32 delta_usec[CSMA_MAX_NODE];
extern u32 delta_usec_node[VIRTUAL_NODE_NUM];
extern u8 response[VIRTUAL_NODE_NUM];
extern u8 short_id[VIRTUAL_NODE_NUM];
extern u8 mcu_mac[VIRTUAL_NODE_NUM][6];
extern u8 plc_recvd;
extern devcfg dvcf;
extern u8 host_state[VIRTUAL_NODE_NUM];
extern u8 node_state[VIRTUAL_NODE_NUM];

extern u8 virtual_idx;
extern u8 virtual_node_num;
extern u32 csma_max_node;

extern u32 counters[VIRTUAL_NODE_NUM];

extern u8 plc_update;
extern u32 get_intmap_SPI(void);
extern void plc_rx_update(u32 intn_map);

u32 get_mcu_id(u8* pMcuID)
{
	u32 CpuID[3] = {0};
		
	CpuID[0] =*(vu32*)(0x1ffff7e8); //按全字（32位）读取
	CpuID[1] =*(vu32*)(0x1ffff7ec);
	CpuID[2] =*(vu32*)(0x1ffff7f0);
	
	// read in byte
	pMcuID[0] = (u8)(CpuID[0] & 0x000000FF);
	pMcuID[1] = (u8)((CpuID[0] & 0xFF00) >>8);
	pMcuID[2] = (u8)((CpuID[0] & 0xFF0000) >>16);
	pMcuID[3] = (u8)((CpuID[0] & 0xFF000000) >>24);
	
	pMcuID[4] = (u8)(CpuID[1] & 0xFF);
	pMcuID[5] = (u8)((CpuID[1] & 0xFF00) >>8);
	pMcuID[6] = (u8)((CpuID[1] & 0xFF0000) >>16);
	pMcuID[7] = (u8)((CpuID[1] & 0xFF000000) >>24);
	
	pMcuID[8] = (u8)(CpuID[2] & 0xFF);
	pMcuID[9] = (u8)((CpuID[2] & 0xFF00) >>8);
	pMcuID[10] = (u8)((CpuID[2] & 0xFF0000) >>16);
	pMcuID[11] = (u8)((CpuID[2] & 0xFF000000) >>24);

	return (CpuID[0]>>1)+(CpuID[1]>>2)+(CpuID[2]>>3);
}

void get_mcu_mac(u8* pMacBuf)
{
	u32 uiMcuId = 0;
	u8 McuID[15] = {0};
	u8 i = 0;

	uiMcuId = get_mcu_id(McuID);

	for(i=0; i<12; i++) //获取McuID[12]
	{
		McuID[12] += McuID[i];	
	}
	for(i=0; i<12; i++)	//获取McuID[13]
	{
		McuID[13] ^= McuID[i];	
	}

	pMacBuf[0] = (u8)(uiMcuId & 0xF0);
	pMacBuf[1] = (u8)((uiMcuId & 0xFF00) >>8);
	pMacBuf[2] = (u8)((uiMcuId & 0xFF0000) >>16);
	pMacBuf[3] = (u8)((uiMcuId & 0xFF000000) >>24);
	pMacBuf[4] = McuID[12];
	pMacBuf[5] = McuID[13];
}

void plc_packet_tquery(plc_psdu *packet, u8 delta_time_idx)
{
    u8 i, stop;
    packet->type = TQUERY_TYPE;
    packet->depth = 0;
    packet->direction = 0;

    // packet->payload[0] = delta_time_idx + 1;
    // packet->payload[1] = (csma_node_cnt/FRAME_LOAD_TQUERY + ((csma_node_cnt%FRAME_LOAD_TQUERY == 0)?0:1));

    if (csma_node_cnt -  delta_time_idx * FRAME_LOAD_TQUERY < FRAME_LOAD_TQUERY) 
        stop = csma_node_cnt -  delta_time_idx * FRAME_LOAD_TQUERY;
    else 
        stop = FRAME_LOAD_TQUERY;
    packet->payload[0] = stop;
    for (i=0; i<stop; i++) {
        // packet->payload[1+i] = (u8)round(((double)delta_usec[delta_time_idx*FRAME_LOAD_TQUERY+i]+0.001)/PREAMBLE_US);
        packet->payload[1+i] = (u8)((delta_usec[delta_time_idx*FRAME_LOAD_TQUERY+i]+PREAMBLE_US/2)/PREAMBLE_US);
    }
    // packet->payload[0] = (u8)((delta_sec[delta_time_idx]>>8)&0x00FF);
	// packet->payload[1] = (u8)(delta_sec[delta_time_idx]&0x00FF);
    // packet->payload[0] = 0;
	// packet->payload[1] = 0;
	// packet->payload[2] = (u8)((delta_usec[delta_time_idx]>>24)&0x000000FF);
	// packet->payload[3] = (u8)((delta_usec[delta_time_idx]>>16)&0x000000FF);
	// packet->payload[4] = (u8)((delta_usec[delta_time_idx]>>8)&0x000000FF);
	// packet->payload[5] = (u8)(delta_usec[delta_time_idx]&0x000000FF);
}

u8 host_tquery(plc_psdu *plc_packet, u8 delta_time_idx)
{
    u8 i;
    u8 err;
    u8 node_one_time;
    u8 stop;

    plc_packet_tquery(plc_packet, delta_time_idx);
    stop = plc_packet->payload[0];

    err = plc_send(plc_packet, dvcf.totjp);
	if (err) return 1; // error
    StartIdle(13);
	while (CheckIdle()) {}
    ClearIdle();

    // if (stop > 1) {
    // err = plc_send(plc_packet, dvcf.totjp);
    // if (err) return 1; // error
    // StartIdle(13);
    // while (CheckIdle()) {}
    // ClearIdle();

    plc_packet->payload[0] = 0;
    err = plc_send(plc_packet, dvcf.totjp);
    // }

    node_one_time = 0;
    StartIdle(FRAME_SLOT_MS * (stop + 2));
	while (CheckIdle()) {
        if (plc_update) {
            plc_update = 0;
            plc_rx_update(get_intmap_SPI());
        }
		if (plc_recvd == 1) {
            plc_recv(plc_packet);
            if ((plc_packet->type & 0x7F) == TQUERY_TYPE && plc_packet->direction == 1 && plc_packet->depth == 0) {
                for (i=0; i<7; i++) mac_list[tquery_node_cnt][i] = plc_packet->payload[i];
                host_state[virtual_idx] = IN_NCONFIG;
                tquery_node_cnt++;
                node_one_time++;
			}
            plc_recvd = 0;
        }
    }
	ClearIdle();

    return node_one_time;
}

u8 node_tquery(plc_psdu *plc_packet)
{
    u8 err, done;
    u8 i, j;
    u8 tmp_id;
    u32 wait[VIRTUAL_NODE_NUM+1];
    u8 pos[VIRTUAL_NODE_NUM];
    u32 max_wait;
    u8 sum;
    
    sum = 0;

    for (i=0; i<plc_packet->payload[0]; i++) {
        if (plc_packet->payload[1+i] < csma_max_node) {
            tmp_id = pte_id[plc_packet->payload[1+i]];
            if (tmp_id < virtual_node_num) {
                if (response[tmp_id] && node_state[tmp_id] == IN_PTE) {
                    node_state[tmp_id] = IN_TQUERY;
                    wait[sum] = i;
                    pos[sum] = tmp_id;
                    tquery_node_cnt++;
                    sum++;
                }
            }
        }
    }
    if (!sum) return 1;
    // wait[sum] = plc_packet->payload[0]-1;
    // sum++;

    for (j=0; j<virtual_node_num; j++) {
        if (node_state[j] == IN_PTE) node_state[j] = COLLISION;
    }

    max_wait = (u32)(plc_packet->payload[0]) - 1 - wait[sum-1];
    // if (plc_packet->payload[0] > 1) {
    for (j=sum-1; j>0; j--) wait[j] = wait[j] - wait[j-1];
    done = 0;
    StartIdle(100);
	while (CheckIdle() && done == 0) {
        if (plc_update) {
            plc_update = 0;
            plc_rx_update(get_intmap_SPI());
        }
		if (plc_recvd == 1) {
            plc_recv(plc_packet);
            if ((plc_packet->type & 0x7F) == TQUERY_TYPE && plc_packet->direction == 0 && plc_packet->depth == 0 && plc_packet->payload[0] == 0) {
                done = 1;
			}
            plc_recvd = 0;
        }
    }
	ClearIdle();
    plc_packet->direction = 1;
    plc_packet->type = TQUERY_TYPE;
    // }

    set_counters(wait, sum);
    for (j=0; j<sum; j++) {
        for (i=0; i<6; i++) plc_packet->payload[i] = mcu_mac[pos[j]][i];
        // plc_packet->payload[6] = short_id[pos[j]];
        plc_packet->payload[6] = 0;
        plc_packet->depth = 0;
        while(counters[j] > 0) {}
        err = plc_send(plc_packet, dvcf.totjp);
        if (err) {
            node_state[pos[j]] = COLLISION;
            response[pos[j]] = 0;
        }
    }
    if (max_wait) {
        StartIdle(max_wait * FRAME_SLOT_MS);
        while (CheckIdle()) {
            if (plc_update) {
                plc_update = 0;
                plc_rx_update(get_intmap_SPI());
            }
            if (plc_recvd == 1) {
                plc_recv(plc_packet);
                plc_recvd = 0;
            }
        }
        ClearIdle();
    }
    // else {
        plc_rx_update(get_intmap_SPI());
        plc_recv(plc_packet);
        plc_recvd = 0;
        plc_update = 0;
    // }
    // plc_update = 0;
    // FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000002);
    return 0;
}
