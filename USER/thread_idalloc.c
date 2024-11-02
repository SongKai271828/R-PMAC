#include "thread.h"
# include <stdlib.h>

void id_list_init(id_list_unit *head)
{
    id_list_unit *tmp;
    
	tmp = (id_list_unit*)malloc(sizeof(id_list_unit));
    tmp->next = head->next;
    tmp->start = MIN_ID;
    tmp->end = MAX_ID;

    head->next = tmp;
}

u8 get_idle_id(id_list_unit *unit, u8 idx)
{
    id_list_unit *tmp;
    tmp = unit->next;
    if (tmp == NULL) return 0;
    while (idx+1 > tmp->end-tmp->start+1) {
        idx -= (tmp->end-tmp->start+1);
        tmp = tmp->next;
        if (tmp == NULL) return 0;
    }
    return tmp->start + idx;
}

u8 update_id_list(id_list_unit *unit, u8 used_id)
{
    id_list_unit *tmp;
    id_list_unit *tmp2;
    
    tmp = unit->next;
    while (tmp != NULL) {
        if (tmp->start < tmp->end) {
            if (tmp->start == used_id) {
                tmp->start = used_id+1;
                return 0;
            } else if (tmp->end == used_id) {
                tmp->end = used_id-1;
                return 0;
            } else if (tmp->start< used_id && tmp->end > used_id) {
                tmp2 = (id_list_unit*)malloc(sizeof(id_list_unit));
                tmp2->next = tmp->next;
                tmp2->end = tmp->end;
                tmp2->start = used_id+1;
                tmp->next = tmp2;
                tmp->end = used_id-1;
                return 0;
            } else {
                tmp = tmp->next;
            }
        } 
        else {
            if (tmp->start == used_id) {
                if (tmp->next == NULL) {
                    tmp2 = unit;
                    while (tmp2->next != tmp) tmp2 = tmp2->next;
                    free(tmp);
                    tmp2->next = NULL;
                }
                else {
                    tmp2 = tmp->next;
                    tmp->start = tmp->next->start;
                    tmp->end = tmp->next->end;
                    tmp->next = tmp->next->next;
                    free(tmp2);
                }
                return 0;
            }
            else {
                tmp = tmp->next;
            }
        }
    }
    return 1;
}

void add_id(id_list_unit *unit, u8 new_id)
{
    id_list_unit *tmp;
    id_list_unit *tmp2;

    tmp = unit->next;
    if (tmp == NULL) {
        tmp2 = (id_list_unit*)malloc(sizeof(id_list_unit));
        tmp2->start = new_id;
        tmp2->end = new_id;
        tmp2->next = tmp;
        unit->next = tmp2;
        return;
    }
    while (tmp != NULL) {
        if (tmp->start == new_id+1) {
            tmp->start = new_id;
        } else if (tmp->end == new_id-1) {
            tmp->end = new_id;
        } else if (tmp->start > new_id+1) {
            tmp2 = (id_list_unit*)malloc(sizeof(id_list_unit));
            tmp2->start = tmp->start;
            tmp2->end = tmp->end;
            tmp2->next = tmp->next;
            tmp->start = new_id;
            tmp->end = new_id;
            tmp->next = tmp2;
        } else {
            tmp = tmp->next;
        }
    }
}

id_list_unit * id_list_to_plc(id_list_unit *unit, plc_psdu *packet)
{
    id_list_unit *tmp;
    u8 i;
    u8 cnt;

    tmp = unit;
    cnt = 0;
    for (i=0; i<(PHYPayloadLength-2)/2; i++)
    {
        if (tmp != NULL) {
            packet->payload[2+2*i+0] = tmp->start;
            packet->payload[2+2*i+1] = tmp->end;
            tmp = tmp->next;
            cnt++;
        }
        else {
            break;
        }
    }
    packet->payload[0] = (tmp == NULL)?1:0;
    packet->payload[1] = cnt;
    return tmp;
}

id_list_unit* id_list_reconstrction(id_list_unit *unit, plc_psdu *packet)
{
    u8 i;
    id_list_unit *tmp;

    tmp = unit;
    for (i=0; i<packet->payload[1]; i++) {
        tmp->next = (id_list_unit*)malloc(sizeof(id_list_unit));
        tmp->next->start = packet->payload[2+i*2+0];
        tmp->next->end = packet->payload[2+i*2+1];
        tmp = tmp->next;
    }
    tmp->next = NULL;
    return tmp;
}

void id_list_free(id_list_unit *unit)
{
    id_list_unit *tmp;
    id_list_unit *tmp2;

    tmp = unit;
    while (tmp->next != NULL)
    {
        tmp2 = tmp->next->next;
        free(tmp->next);
        tmp->next = tmp2;
    }
}
