#ifndef RECOBOP_H
#define RECOBOP_H

#include <math.h>
#include <stdint.h>

struct recobop_info {
	uint32_t saw_vin;
	uint32_t saw_vsense;
	uint32_t saw_power;
	uint32_t saw_pos;

	uint32_t ctrl_touch_wait;

	uint32_t perf_touch;
	uint32_t perf_control;
	uint32_t perf_inverse;
	uint32_t perf_all;
};

static inline uint32_t fltofi(float f, int n, int dn) {
	int i;
	int wn = n - dn;

	int d;
	float w;

	uint32_t df, wf;
	uint32_t m = 0xffffffff;

	if (f > 0) {
		d = floor(f);
		w = f - d;
	} else {
		d = floor(f);
		w = f - d;
	}

	df = 0 | (d << wn);

	wf = 0;
	for (i = -1; i >= -wn; i--) {
		if (w >= pow(2, i)) {
			wf |= 1 << (wn + i);
			w -= pow(2, i);
		}
	}

	return (df | wf) & (m >> (32 - n));
}

static inline float fitofl(uint32_t f, int n, int dn) {
	int i;
	int wn = n - dn;

	int d;
	float w;

	uint32_t m = 0xffffffff;

	if (((f >> (n - 1)) & 0x1) == 1) {
		d = (m << dn) | (f >> wn);
	} else {
		d = 0 | (f >> wn);
	}

	w = 0;
	for (i = -1; i >= -wn; i--) {
		if (((f >> (wn + i)) & 0x1) == 1) {
			w += pow(2, i);
		}
	}

	return d + w;
}

#endif /* RECOBOP_H */