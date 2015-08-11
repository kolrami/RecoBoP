#ifndef RECOBOP_H
#define RECOBOP_H

#include "reconos.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

struct recobop_info {
	float saw_vin;
	float saw_vsense;
	float saw_power;
	uint32_t saw_pos;

	uint32_t ctrl_touch_wait;

	uint32_t perf_touch;
	uint32_t perf_control;
	uint32_t perf_inverse;
	uint32_t perf_all;

	int thread_count;
	struct reconos_thread *thread_p[16];
};

static inline float rbi_saw_power(struct recobop_info *rb_info) {
	return rb_info->saw_power;
}

static inline int rbi_saw_pos_x(struct recobop_info *rb_info) {
	int pos_x = (rb_info->saw_pos >> 12) & 0xfff;
	if (((pos_x >> 11) & 0x1) == 1) {
		pos_x |= 0xfffff000;
	}

	return pos_x;
}

static inline int rbi_saw_pos_y(struct recobop_info *rb_info) {
	int pos_y = (rb_info->saw_pos >> 0) & 0xfff;
	if (((pos_y >> 11) & 0x1) == 1) {
		pos_y |= 0xfffff000;
	}

	return pos_y;
}

static inline float rbi_perf_touch(struct recobop_info *rb_info) {
	//printf("%f\n", rb_info->perf_touch / 100000.0);
	return rb_info->perf_touch / 100000.0;
}

static inline float rbi_perf_control(struct recobop_info *rb_info) {
	//printf("%f\n", rb_info->perf_control / 100000.0);
	return rb_info->perf_control / 100000.0;
}

static inline float rbi_perf_inverse(struct recobop_info *rb_info) {
	//printf("%f\n", rb_info->perf_inverse / 100000.0);
	return rb_info->perf_inverse / 100000.0;
}

static inline float rbi_perf_overhead(struct recobop_info *rb_info) {
	//printf("%f\n", rb_info->perf_all / 100000.0);
	return (rb_info->perf_all - rb_info->perf_touch - rb_info->perf_control - rb_info->perf_inverse) / 100000.0;
}

static inline int rbi_thread_count_m(struct recobop_info *rb_info,
                                     char *thread_name, int thread_mode) {
	int i, count = 0;
	struct reconos_thread *rt;

	for (i = 0; i < 16; i++) {
		rt = rb_info->thread_p[i];
		if (!rt) {
			continue;
		}

		if (strcmp(thread_name, rt->name) == 0)
			if (   (  rt->state == RECONOS_THREAD_STATE_RUNNING_HW
				    && thread_mode == RECONOS_THREAD_MODE_HW)
			    || (  rt->state == RECONOS_THREAD_STATE_RUNNING_SW
			    	&& thread_mode == RECONOS_THREAD_MODE_SW)) {
				count++;
			}
	}

	return count;
}

static inline int rbi_thread_count(struct recobop_info *rb_info,
                                   char *thread_name) {
	return   rbi_thread_count_m(rb_info, thread_name, RECONOS_THREAD_MODE_SW)
	       + rbi_thread_count_m(rb_info, thread_name, RECONOS_THREAD_MODE_HW);
}

static inline int rbi_thread_index(struct recobop_info *rb_info,
                                   char *thread_name, int thread_mode) {
	int i;
	struct reconos_thread *rt;

	for (i = 0; i < 16; i++) {
		rt = rb_info->thread_p[i];
		if (!rt) {
			continue;
		}

		if (   strcmp(thread_name, rt->name) == 0 
			&& rt->state == RECONOS_THREAD_STATE_RUNNING_HW
			&& thread_mode == RECONOS_THREAD_MODE_HW) {
			return i;
		}
	}

	return -1;
}

static inline int rbi_thread_index_free(struct recobop_info *rb_info) {
	int i;

	for (i = 0; i < 16; i++) {
		if (!rb_info->thread_p[i]) {
			return i;
		}
	}

	return -1;
}

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