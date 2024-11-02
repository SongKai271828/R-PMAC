#ifndef __DRV_PLC_H
#define __DRV_PLC_H

#include "stm32f10x.h"
#include "app_config.h"

#define CMD_SystemOp 0x00
#define CMD_PreambleOp 0x01
#define CMD_TxFilterOp 0x03
#define CMD_RxFilterOp 0x05
#define CMD_SysParamOp 0x09
#define CMD_FrequencyOp 0x0A
#define CMD_SysModeOp 0x0B
#define CMD_TxParamOp 0x0C
#define CMD_RxParamOp 0x0D
#define CMD_RxOptimzOp 0x0E
#define CMD_ExecutionOp 0x0F
#define CMD_InterruptOp 0x10
#define CMD_BufferStatus 0x11
#define CMD_TxBuffer 0x12
#define CMD_RxBuffer 0x13
#define CMD_WatchDog 0x16

typedef struct {
    u8 type;
    u8 depth;
    u8 route[PHYMAXDepth]; // max depth, 16
    u8 target; // next jump target, based on route
    u8 direction;
    u8 payload[PHYPayloadLength]; // phy allow 100 bytes
    // received packet information
    u8 errsyms;
	u16 errbits;
} plc_psdu;

void plc_static_config(void);
void plc_dynamic_config(u32 prmfreq, u8 bandwd, u8 phymd, u32 trxgain, u16 totwdt);
void plc_wdt_reset(void);
int plc_intftest(void);

u8 plc_send_PFC(u8 phymd, u32 tot);
u8 plc_send_pfc(plc_psdu *packet, u32 tot);
u8 plc_send(plc_psdu *packet, u32 tot);
u8 plc_recv(plc_psdu *packet);

#endif
