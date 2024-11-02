#include "thread.h"

extern u8 uart485_buf[UARTBufferLength];
extern u8 route_list[MAX_NODE_NUM][PHYMAXDepth];

// u8 plc_compose_header(plc_psdu *packet)
// {
// 	u8 i;
	
// 	// plc packet header
//     packet->type = uart485_buf[2];
//     packet->depth = uart485_buf[3];
// 	if (packet->depth > PHYMAXDepth)
// 		return 1; // error coz depth larger than max routes
	
// 	for (i = 0; i < packet->depth; i++)
// 		packet->route[i] = uart485_buf[4 + i];
//     packet->direction = 0;
//     packet->target = uart485_buf[5];
	
// 	return 0; // success
// }

// generate route according to route_list
u8 plc_compose_header(plc_psdu *packet)
{
	u8 i;
	
	// plc packet header
    packet->type = uart485_buf[2];
    packet->direction = 0;

	if (uart485_buf[3] == 1) {
		packet->depth = 1;
	} else {
		packet->depth = get_route_depth(uart485_buf[5]);
		if (packet->depth == 1) return 1;
	}
	if (packet->depth > PHYMAXDepth)
		return 1; // error coz depth larger than max routes
	
	packet->route[0] = uart485_buf[4];
	for (i = 2; i <= packet->depth; i++) packet->route[i-1] = get_route_list_unit(uart485_buf[5], i);
	
	if (packet->depth > 1) {
		packet->target = packet->route[1];
	} 
	else {
		packet->target = packet->route[0];
	}
	if (packet->type == PMAC_TYPE || packet->type == SIMU_TYPE) {
		packet->payload[0] = (uart485_buf[6]);
		packet->payload[1] = (uart485_buf[7]);
		packet->payload[2] = (uart485_buf[8]);
		packet->payload[3] = (uart485_buf[9]);
	}
	if (packet->type == SIMU_TYPE) {
		packet->payload[4] = (uart485_buf[10]);
		packet->payload[5] = (uart485_buf[11]);
		packet->payload[6] = (uart485_buf[12]);
		packet->payload[7] = (uart485_buf[13]);
		packet->payload[8] = (uart485_buf[14]);
		packet->payload[9] = (uart485_buf[15]);
	}

	return 0; // success
}

// u8 plc_compose_header(plc_psdu *packet)
// {
// 	u8 i;
	
// 	// plc packet header
//     packet->type = uart485_buf[2];
//     packet->direction = 0;

// 	if (uart485_buf[3] == 1) {
// 		packet->depth = 1;
// 		packet->route[0] = uart485_buf[4];
// 	} else {
// 		packet->depth = uart485_buf[3];
// 		for (i = 0; i < packet->depth; i++) packet->route[i] = uart485_buf[4 + i];
// 	}
// 	if (packet->depth > PHYMAXDepth)
// 		return 1; // error coz depth larger than max routes
	
// 	// packet->route[0] = uart485_buf[4];
// 	// for (i = 2; i <= packet->depth; i++) packet->route[i-1] = get_route_list_unit(uart485_buf[5], i);
	
// 	if (packet->depth > 1) {
// 		packet->target = packet->route[1];
// 	} 
// 	else {
// 		packet->target = packet->route[0];
// 	}
// 	if (packet->type == PMAC_TYPE || packet->type == SIMU_TYPE) {
// 		packet->payload[0] = (uart485_buf[4+packet->depth+0]);
// 		packet->payload[1] = (uart485_buf[4+packet->depth+1]);
// 		packet->payload[2] = (uart485_buf[4+packet->depth+2]);
// 		packet->payload[3] = (uart485_buf[4+packet->depth+3]);
// 	}
// 	if (packet->type == SIMU_TYPE) {
// 		packet->payload[4] = (uart485_buf[10]);
// 		packet->payload[5] = (uart485_buf[11]);
// 		packet->payload[6] = (uart485_buf[12]);
// 		packet->payload[7] = (uart485_buf[13]);
// 	}

// 	return 0; // success
// }

u8 get_current_jump(plc_psdu *packet)
{
	u8 jp;
	
	for (jp = 0; jp < packet->depth; jp++)
		if (packet->route[jp] == packet->target)
			break;
	
	return jp;
}

void set_target_ber(plc_psdu *packet, u8 jp)
{
	if (packet->direction == 0)
		packet->payload[3 + jp] = (packet->errbits & 0xFF);
	else if (packet->direction == 1)
		packet->payload[packet->depth + 1 + packet->depth - jp] = (packet->errbits & 0xFF);
}

void set_next_target(plc_psdu *packet, u8 jp)
{
    if (packet->direction == 0)
        packet->target = packet->route[jp + 1];
    else if (packet->direction == 1)
        packet->target = packet->route[jp - 1];
}

void set_destination_reply(plc_psdu *packet)
{
    packet->direction = 1;
    packet->target = packet->route[packet->depth - 2];
}
