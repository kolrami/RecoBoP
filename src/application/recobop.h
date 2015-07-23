#ifndef RECOBOP_H
#define RECOBOP_H

#include <stdint.h>

struct recobop_info {
	uint32_t saw_vin;
	uint32_t saw_vsense;
	uint32_t saw_power;
	uint32_t saw_pos;

	uint32_t ctrl_touch_wait;
};

#endif /* RECOBOP_H */