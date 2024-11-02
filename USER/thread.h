#ifndef __THREAD_H
#define __THREAD_H

#include "drv_plc.h"

#define COMPENSATE_TEST 0	// compensate delay mode
#define ROUTE_UPDATE_ENABLE 1

#define VIRTUAL_NODE_NUM 80	// number of nodes per real nodes
#define CSMA_MAX_NODE 254	// max slot num of PTE
#define MAX_NODE_NUM 254	// max node num in the network
#define MIN_ID 1			// min SID of node
#define MAX_ID 253			// max SID of node

#define TQUERY_TOT_MS 50	
#define NCONFIG_TOT_MS 50
#define PREAMBLE_US 600
#define FRAME_SLOT_MS 17

#define TIME_FEEDBACK_TYPE 0x0C
#define PMAC_TYPE 0x0B
#define TQUERY_TYPE 0x0D
#define NCONFIG_TYPE 0x0E
#define SIMU_TYPE 0X0F
#define ROUTE_TYPE 0X10

#define DT_COMPENSATE 125
#define TIME_ACCURACY (50*2)

#define PFC_NET 0
#define PFC_REQ 1
#define PFC_ACK 2
#define PFC_DAT 3
#define PFC_POWER 0x40001EFF
#define PFC_TOT_US 1000

#define OFFLINE 0
#define IN_PTE 1
#define IN_TQUERY 2
#define IN_NCONFIG 3
#define ONLINE 4
#define COLLISION 5

#define FRAME_LOAD_TQUERY (PHYPayloadLength-1)
#define FRAME_LOAD_NCONFIG ((PHYPayloadLength-3) / 7)


typedef struct {
	u8 inuse; // 1 bit, can be inferred
	// 2 bytes
	u8 type; // 4 bits
	u16 tot; // 12 bits
	// 1 byte
	u8 lenpos; // 4 bits
	u8 lenmult; // 4 bits
	// 1 byte
	u8 enFE; // 1 bit
	u8 nohdr; // 1 bit
	u8 lenadj; // 6 bits
} protcl; // 4 bytes

typedef struct {
    u8 baudrate; // 2 bits
	u8 is8b;
	u8 parity; // 2 bits
} uartcfg;

typedef struct {
	u16 dbgmap; // 16 bits, for debug parameters, to be inferred
	u16 savmap; // 16 bits, for saved parameters
	u8 nodetype;
	u8 shortid;
	u32 prmfreq;
	u32 swtfreq;
	uartcfg uartcb; // 8 bits
	u8 bandwd;
	u8 phymd;
	u32 trxgain;
	u16 totchn; // 12 bits
	u16 totjp; // 12 bits
	u16 totwdt; // 16 bits
	protcl prttyp[8];
} devcfg;


u8 plc_compose_header(plc_psdu *packet);
u8 get_current_jump(plc_psdu *packet);
void set_target_ber(plc_psdu *packet, u8 jp);
void set_next_target(plc_psdu *packet, u8 jp);
void set_destination_reply(plc_psdu *packet);

void device_config_load(void);
void start_this_config(void);
u8 start_device_config(plc_psdu *packet);
u8 redirect_device_config(plc_psdu *packet);
u8 process_device_config(plc_psdu *packet);

u8 start_channel_check(plc_psdu *packet);
u8 redirect_channel_check(plc_psdu *packet);
u8 process_channel_check(plc_psdu *packet);

u8 start_meter_reading(plc_psdu *packet, u8 len);
u8 redirect_meter_reading(plc_psdu *packet);
u8 process_meter_reading(plc_psdu *packet);

u8 start_device_reset(plc_psdu *packet);
u8 redirect_device_reset(plc_psdu *packet);
u8 process_device_reset(plc_psdu *packet);

void meter_reading_sch_init(void);
u8 start_meter_reading_sch(plc_psdu *packet);
u8 redirect_meter_reading_sch(plc_psdu *packet);
u8 process_meter_reading_sch(plc_psdu *packet);
void save_meter_reading_sch(void);


// for PMAC
u8 host_PMAC(plc_psdu *packet, u8 type);
u8 start_PMAC(plc_psdu *packet);
u8 start_time_feedback(plc_psdu *packet);
void PMAC_init(void);
u8 PMAC_feedback(u8 out_type);
void PMAC_reset(void);
u8 process_time_feedback(plc_psdu *packet);
u8 redirect_time_feedback(plc_psdu *packet);
u8 get_time_record(void);
void get_delta_time(u32 *delta_usec, u8 csma_node_idx);


// for PTE
u8 start_pte(plc_psdu *packet);
u8 host_PTE(u32 csma_tot_us, u32 tx_pwr);
u8 node_PTE(void);
u8 redirect_host_PTE(plc_psdu *packet);
u8 process_host_PTE(plc_psdu *packet);

// for csma/cd
u8 csma_cd(u32 wait);

// for TQuery
u32 get_mcu_id(u8* pMcuID);
void get_mcu_mac(u8* pMacBuf);
void plc_packet_tquery(plc_psdu *plc_packet, u8 delta_time_idx);
u8 host_tquery(plc_psdu *plc_packet, u8 delta_time_idx);
u8 node_tquery(plc_psdu *packet);

// for NConfig
u8 host_nconfig(plc_psdu *packet, u8 node_num_idx, u8 is_final);
u8 node_nconfig(plc_psdu *packet);

// for ID_allocation
typedef struct id_list
{
	struct id_list* next;
	u8 start;
	u8 end;
} id_list_unit;
u8 get_idle_id(id_list_unit *unit, u8 idx);
void id_list_init(id_list_unit *unit);
u8 update_id_list(id_list_unit *unit, u8 used_id);
void add_id(id_list_unit *unit, u8 new_id);
id_list_unit* id_list_reconstrction(id_list_unit *unit, plc_psdu *packet);
void id_list_free(id_list_unit *unit);
id_list_unit * id_list_to_plc(id_list_unit *unit, plc_psdu *packet);

// for route
void add_route_list_unit(u8 idx, u8 next_jump);
u8 get_route_depth(u8 destination);
u8 get_route_list_unit(u8 destination, u8 depth);
void free_route_list(u8 head);

//for ber
u8 get_ber_param(plc_psdu *plc_packet);
u8 host_ber_update(plc_psdu *packet, u8 idx);
u8 node_ber_update(plc_psdu *packet);

// for simulation
u8 some_in_state(u8* state_list, u8 state);
void random_response_generate(u8* response);
u8 get_shortid_idx(u8* short_id_list, u8 shortid);
void random_back_off_generate(u32 *random_time_list);
void start_simulation(plc_psdu *packet);
u8 route_feedback(plc_psdu *packet);
void solid_update(u8 *solid_list, u8 depth);
void process_simu(plc_psdu *packet);

//for epmac
void set_counters(u32* wait, u8 sum);
#endif
