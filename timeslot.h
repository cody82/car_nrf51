#ifndef __TIMESLOT_H
#define __TIMESLOT_H

#include <stdint.h>

uint32_t timeslot_init();
uint32_t timeslot_start();
uint32_t timeslot_stop();
void timeslot_on_sys_evt(uint32_t sys_evt);



#endif
