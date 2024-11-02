#include "thread.h"
#include "bsp.h"
//#include "drv_flash.h"
#include "drv_uart485.h"
//#include "drv_rtc.h"
# include <stdlib.h>

// plc related
u8 plc_update;
u8 plc_sent;
u8 plc_recvd;
u8 pfc_sent;
u8 pfc_recvd;

// sch mtr rd related
u8 scproc_en;
u16 sc_cntdwn;
u16 sc_cntini;
u8 red_state;

// PTE related
u8 day, hour;	// global time, day and hour
u16 sec;		// global time, second and usecond
u32 usec;
u32 start_time[2];	//start of delta time
u32 end_time[2];	//end of delta time
u32 delta_usec[CSMA_MAX_NODE];	// delta time in us of host
u32 delta_usec_node[VIRTUAL_NODE_NUM];	// delta time in us of node

// csma related
u8 csma_node_cnt;	// node counter of one pte
u32 random_backoff_time[VIRTUAL_NODE_NUM];	// slot number of random backoff
u8 response[VIRTUAL_NODE_NUM];	// flag that the node will send network request
u8 pte_id[CSMA_MAX_NODE];

// TQUery related
u8 mcu_mac[VIRTUAL_NODE_NUM][6];	// MAC of virual nodes
u8 mac_list[CSMA_MAX_NODE][7];		// MAC record in one networking process
u8 tquery_node_cnt;					// node counter of one tquery
u8 nconfig_node_cnt;				// node counter of one nconfig

u8 short_id[VIRTUAL_NODE_NUM];		// SID of virtual nodes
u8 host_state[VIRTUAL_NODE_NUM];	// state of host or proxy
u8 node_state[VIRTUAL_NODE_NUM];	// state of nodes

// route related
id_list_unit id_list_head;			// head of idle SID link list
id_list_unit* id_list_tail;			// tail of idle SID link list
u8 route_list[MAX_NODE_NUM][PHYMAXDepth];	// route list
u8 solid[VIRTUAL_NODE_NUM];			// flag that the node will not send network request any more

// ber related
u8 ber_list[MAX_NODE_NUM]; 			// BER metric of nodes
u32 ber_wait[VIRTUAL_NODE_NUM];		// wait time for node sending BER
u8 ber_wait_pos[VIRTUAL_NODE_NUM];
u8 ber_wait_sum;

// simulation related
u8 virtual_idx;
u8 virtual_depth[VIRTUAL_NODE_NUM];
u8 origin_id;
u8 virtual_node_num;
u32 csma_max_node;
u8 max_node_num;
u8 response_p;
u8 simu_on;
u8 flag = 0;
u8 first_num;
u8 max_depth;
u8 current_depth;

u32 counters[VIRTUAL_NODE_NUM];		// timer used in sending
u8 counter_idx;
u8 counter_cnt;


u8 tot_type;


extern devcfg dvcf;


// 485 recvd started
extern u8 uart485_start;

// gpio related
extern u8 rstr_en;

void host_process(void); // base process
void node_process(void); // node process
//void software_reset(void);
void plc_prm_rx_update(u32 intn_map);
void plc_rx_update(u32 intn_map);
u32 get_intmap_SPI(void);

void app_main(void)
{
	u8 err;
	u32 flid;
	
	
	bsp_config();
	
	device_config_load();
	origin_id = dvcf.shortid;
	plc_update = 0;
	
	max_node_num = MAX_NODE_NUM;
	csma_max_node = CSMA_MAX_NODE;
	response_p = 20;
	// virtual_node_num = VIRTUAL_NODE_NUM;
	virtual_node_num = 40;
	first_num = 8;
	max_depth = 3;
	simu_on = 0;
	usec = 0;
	sec = 0;
	hour = 0;
	day = 0;
	red_state = 1;
	counter_cnt = 0;

	id_list_head.next = NULL;
	PMAC_init();
	if (rstr_en == 1)
		return;
	
	FW_UART485_Config(dvcf.uartcb.baudrate, dvcf.uartcb.is8b, dvcf.uartcb.parity);
	
	err = plc_intftest();
	flid = FW_FLASH_SPI_ID();
	
	if (err == 0 && flid == 0x00C84016) {
		FW_UART485_SendByte(0xFE);
		FW_LED_GPIO_Red(1);
	}
		
	// init plc config
	plc_sent = 0;
	plc_recvd = 0;
	pfc_sent = 0;
	pfc_recvd = 0;
	
	// init uart
	restart_uart485();
	
	// init sch config
	scproc_en = 0;
	sc_cntdwn = 0;
	sc_cntini = 0;
	
	// plc settings
	plc_static_config();
	plc_dynamic_config(dvcf.prmfreq, dvcf.bandwd, dvcf.phymd, dvcf.trxgain, dvcf.totwdt);
	
	meter_reading_sch_init(); // init scheduled meter reading
	
	if (dvcf.nodetype == 0){
		host_process();
	}
	else {
		node_process();
	}
	
	//while (1);
}

void host_process(void)
{
	u8 err;
    u8 pl_len;
	plc_psdu plc_packet;
	
	while (1) {
		if (uart485_start == 1) {
			pl_len = start_uart485(UARTTimeout);
			if (pl_len == 0)
				continue;
			
			err = plc_compose_header(&plc_packet);
			if (err != 0) {
				restart_uart485();
				continue;
			}
			
			if (plc_packet.type == 0x06) {
				if (plc_packet.depth == 1 && plc_packet.route[0] == dvcf.shortid) {
					start_this_config();
					plc_dynamic_config(dvcf.prmfreq, dvcf.bandwd, dvcf.phymd, dvcf.trxgain, dvcf.totwdt);
					continue;
				}
				
                err = start_device_config(&plc_packet);
				if (err != 0) {
					restart_uart485();
					continue;
				}
            }
			
			if (plc_packet.type == 0x07) {
                err = start_channel_check(&plc_packet);
				if (err != 0) {
					restart_uart485();
					continue;
				}
            }
			
			if (plc_packet.type == 0x08) {
				err = start_meter_reading(&plc_packet, pl_len);
				if (err != 0) {
					restart_uart485();
					continue;
				}
			}
			
			if (plc_packet.type == 0x09) {
				if (plc_packet.depth == 1 && plc_packet.route[0] == dvcf.shortid) {
					//software_reset();
					plc_wdt_reset();
					continue;
				}
				
				err = start_device_reset(&plc_packet);
				if (err != 0) {
					restart_uart485();
					continue;
				}
			}
			
			if (plc_packet.type == 0x0A) {
				err = start_meter_reading_sch(&plc_packet);
				if (err != 0) {
					restart_uart485();
					continue;
				}
			}
			
			// if (plc_packet.type == 0x0B) {
			// 	if (plc_packet.depth == 1 && plc_packet.route[0] == dvcf.shortid) {
			// 		err = start_pte(&plc_packet);
			// 		if (err != 0) {
			// 			restart_uart485();
			// 			continue;
			// 		}
			// 	}
			// }

			// instruction of network constuction startup
			if (plc_packet.type == PMAC_TYPE) {
				if (plc_packet.depth == 1 && plc_packet.route[0] == dvcf.shortid) {
					err = host_PMAC(&plc_packet, 1);
					err = host_PMAC(&plc_packet, 2);
					err = host_PMAC(&plc_packet, 3);
					err = host_PMAC(&plc_packet, 4);
					err = host_PMAC(&plc_packet, 5);

					PMAC_feedback(3);
					if (err != 0) {
						restart_uart485();
						continue;
					}
				}
				else if (plc_packet.depth > 1 && plc_packet.route[0] == dvcf.shortid) {
					start_PMAC(&plc_packet);
					PMAC_feedback(3);
				}
				PMAC_reset();
			}
			
			//instruction of time feedback
			if (plc_packet.type == TIME_FEEDBACK_TYPE) {
				err = start_time_feedback(&plc_packet);
			}

			// instruction of simulation
			if (plc_packet.type == SIMU_TYPE) {
				start_simulation(&plc_packet);
			}

			// get SIDs of nodes at a depth
			if (plc_packet.type == ROUTE_TYPE) {
				route_feedback(&plc_packet);
			}
			
			restart_uart485();
		}
		
		if (plc_recvd == 1) {
			// error - clear rxbuffer
			FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000002);
			
			plc_recvd = 0;
			continue;
		}
	}
}

void node_process(void)
{
	u8 err;
	
	plc_psdu plc_packet;
	u32 intn_map;
	u8 idx;
	u8 i;
	u32 record, tmp;
	
	while (1) {
		if (plc_update) {
			plc_update = 0;
			intn_map = get_intmap_SPI();
			plc_rx_update(intn_map);
			plc_prm_rx_update(intn_map);
		}
		else {
			continue;
		}
		if (plc_recvd == 1) {

#if COMPENSATE_TEST
			get_time_record();
#endif

			err = plc_recv(&plc_packet);
			if (err != 0) {
				plc_recvd = 0;
				continue;
			}
			
			if ((plc_packet.type & 0x7F) == SIMU_TYPE) {
				// FW_LED_GPIO_Green(1);
				plc_recvd = 0;
				process_simu(&plc_packet);
				// FW_LED_GPIO_Green(0);
				continue;
			}
			if (plc_packet.depth == 0 && (plc_packet.type & 0x7F) == TQUERY_TYPE && plc_packet.direction == 0 && plc_packet.payload[0] > 0) {
				// FW_LED_GPIO_Green(1);
				plc_recvd = 0;
				err = node_tquery(&plc_packet);
				// FW_LED_GPIO_Green(0);
				continue;
			}
			if (plc_packet.depth == 0 && (plc_packet.type & 0x7F) == NCONFIG_TYPE && plc_packet.direction == 0) {
				// FW_LED_GPIO_Green(1);
				plc_recvd = 0;
				err = node_nconfig(&plc_packet);
				// FW_LED_GPIO_Green(0);
				continue;
			}
			if (plc_packet.depth == 0 && (plc_packet.type & 0x7F) == 0x07 && plc_packet.direction == 0) {
				// FW_LED_GPIO_Green(1);
				plc_recvd = 0;
				err = node_ber_update(&plc_packet);
				// FW_LED_GPIO_Green(0);
				continue;
			}
			if (plc_packet.depth == 0) {
				plc_recvd = 0;
				continue;
			}
			if ((plc_packet.type & 0x7F) == PMAC_TYPE && plc_packet.depth > 0 && plc_packet.direction == 0) {
				if (current_depth < plc_packet.depth + 1) {
					current_depth = plc_packet.depth + 1;
					flag = 0;
				}
				if (flag == 0) {
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
					flag = 1;
				}
			}

			plc_recvd = 0;

			idx = get_shortid_idx(short_id, plc_packet.target);
			if (idx < virtual_node_num) {
				virtual_idx = idx;
				dvcf.shortid = short_id[virtual_idx];
			}
			else {
				continue;
			}
			
			// downlink processed here, uplink processed in each thread
			if (plc_packet.route[plc_packet.depth - 1] != dvcf.shortid) // this is not destination
			{
				
				if ((plc_packet.type & 0x7F) == 0x06)
					redirect_device_config(&plc_packet);
				
				if ((plc_packet.type & 0x7F) == 0x07) {
					redirect_channel_check(&plc_packet);
					if (plc_packet.direction) {
						FW_PLC_GPIO_WDTFeed();
					}
				}
				
				if ((plc_packet.type & 0x7F) == 0x08)
					redirect_meter_reading(&plc_packet);
				
				if ((plc_packet.type & 0x7F) == 0x09)
					redirect_device_reset(&plc_packet);
				
				if ((plc_packet.type & 0x7F) == 0x0A)
					redirect_meter_reading_sch(&plc_packet);
				
				// node PTE startup
				if ((plc_packet.type & 0x7F) == PMAC_TYPE || (plc_packet.type & 0x7F) == TQUERY_TYPE || (plc_packet.type & 0x7F) == NCONFIG_TYPE) {
					redirect_host_PTE(&plc_packet);
				}
				// node time feedback redirect
				if ((plc_packet.type & 0x7F) == TIME_FEEDBACK_TYPE) {
					redirect_time_feedback(&plc_packet);
				}
			}
			
			if (plc_packet.route[plc_packet.depth - 1] == dvcf.shortid) // this is destination
			{
				
				if ((plc_packet.type & 0x7F) == 0x06) {
					process_device_config(&plc_packet);
					plc_dynamic_config(dvcf.prmfreq, dvcf.bandwd, dvcf.phymd, dvcf.trxgain, dvcf.totwdt);
				}
				
				if ((plc_packet.type & 0x7F) == 0x07) {
					process_channel_check(&plc_packet);
					FW_PLC_GPIO_WDTFeed();
				}
				
				if ((plc_packet.type & 0x7F) == 0x08)
					process_meter_reading(&plc_packet);
				
				if ((plc_packet.type & 0x7F) == 0x09) {
					process_device_reset(&plc_packet);
					//software_reset();
					plc_wdt_reset();
				}
				
				if ((plc_packet.type & 0x7F) == 0x0A)
					process_meter_reading_sch(&plc_packet);

				// node time feedback process
				if ((plc_packet.type & 0x7F) == TIME_FEEDBACK_TYPE) {
					process_time_feedback(&plc_packet);
				}

				if ((plc_packet.type & 0x7F) == PMAC_TYPE || (plc_packet.type & 0x7F) == TQUERY_TYPE || (plc_packet.type & 0x7F) == NCONFIG_TYPE) {
					process_host_PTE(&plc_packet);
				}
			}

			
		}

		if (pfc_recvd) {
			if (pfc_recvd == PFC_NET+1) {
				pfc_recvd = 0;
				node_PTE();
			}
		}
		// if (pfc_recvd) {
		// 	for (i = 0; i < pfc_recvd; i++) {
		// 		FW_LED_GPIO_Green(1);
		// 		StartIdle(500);
		// 		while (CheckIdle()) {}
		// 		ClearIdle();
		// 		FW_LED_GPIO_Green(0);
		// 		StartIdle(500);
		// 		while (CheckIdle()) {}
		// 		ClearIdle();
		// 	}
			
		// 	pfc_recvd = 0;
		// }
		
		if (scproc_en == 1 && sc_cntdwn <= 0) {
			FW_LED_GPIO_Green(1);
            save_meter_reading_sch();
			FW_LED_GPIO_Green(0);
        }
	}
}

//void software_reset(void)
//{
//    NVIC_SETFAULTMASK();
//    NVIC_GenerateSystemReset();
//}

void usart485_isr(u8 data)
{
	// use global to capture all bytes
	if (dvcf.nodetype == 0)
		uart485_host_recvbyte(data);
	else
		uart485_node_recvbyte(data);
}

void plc_isr(void)
{
	if (!plc_update) plc_update = 1;
}

void plc_rx_update(u32 intn_map)
{
	if ((intn_map & 0x00000020) >> 5 == 0x1) {
		if ((intn_map & 0x00000040) >> 6 == 0x1) {
			// error - clear rxbuffer
			FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000002);
			plc_recvd = 0;
		}
		else
			plc_recvd = 1;
    }
}

void plc_tx_update(u32 intn_map)
{
	if ((intn_map & 0x00000002) >> 1 == 0x1)
		plc_sent = 1;
}

void plc_prm_tx_update(u32 intn_map)
{
	if ((intn_map & 0x00000400) >> 10 == 0x1)
		pfc_sent = 1;
}

void plc_prm_rx_update(u32 intn_map)
{
	if ((intn_map & 0x00000800) >> 11 == 0x1) {
		if ((intn_map & 0x00003000) >> 12 == 0x3) {
			// error - data mode
			pfc_recvd = 0;
		}
		else {
			pfc_recvd = (intn_map & 0x00003000) >> 12;
			pfc_recvd++;
		}
	}
}

void plc_isr_open(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
	EXTI_InitStructure.EXTI_Line = EXTI_Line8;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}
void plc_isr_close(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
	EXTI_InitStructure.EXTI_Line = EXTI_Line8;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;
	EXTI_Init(&EXTI_InitStructure);
}
u32 get_intmap_SPI(void)
{
	u32 intn_map;
	// plc_isr_close();
	intn_map = FW_PLC_SPI_Read32Bits(CMD_InterruptOp);
	FW_PLC_SPI_Write32Bits(CMD_ExecutionOp, 0x00000001);
	// plc_isr_open();
	return intn_map;
}

void tim_isr(void)
{
    if (scproc_en == 1) {
        if (sc_cntdwn <= 0)
            sc_cntdwn = sc_cntini;
        
        sc_cntdwn--;
    }
}

void set_counters(u32* wait, u8 sum)
{
	u8 i;
	counter_idx = 0;
	for (i=0; i<sum; i++) counters[i] = wait[i] * FRAME_SLOT_MS * 1000;
	counter_cnt = sum;
}

// void time_update_isr(void)
// {
// 	usec += 25;
// 	if (counter_cnt) {
// 		if (counters[counter_idx] <= 25) {
// 			counters[counter_idx] = 0;
// 			if (counter_idx == counter_cnt-1) counter_cnt = 0;
// 			else counter_idx++;
// 		}
// 		else if (counters[counter_idx] > 25) {
// 			counters[counter_idx] -= 25;
// 		}
// 	}
// 	if (usec==1000000) {
// 		sec += 1;
// 		usec = 0;
// 		if (sec == 3600)
// 		{
// 			hour += sec/3600;
// 			sec %= 3600;
// 			day += hour/24;
// 			hour %= 24;
// 		}
// 		FW_LED_GPIO_Red(red_state);
// 	 	red_state = (red_state+1)%2;
// 	}
// }

void time_update_isr(void)
{
	usec += 50;
	if (counter_cnt) {
		if (counters[counter_idx] <= 50) {
			counters[counter_idx] = 0;
			if (counter_idx == counter_cnt-1) counter_cnt = 0;
			else counter_idx++;
		}
		else if (counters[counter_idx] > 50) {
			counters[counter_idx] -= 50;
		}
	}
	if (usec==1000000) {
		sec += 1;
		usec = 0;
		if (sec == 3600)
		{
			hour += sec/3600;
			sec %= 3600;
			day += hour/24;
			hour %= 24;
		}
		FW_LED_GPIO_Red(red_state);
	 	red_state = (red_state+1)%2;
	}
}
