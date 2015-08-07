#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>

#define KP -0.08
#define KI -0.00004
#define KD -25

#if 0
#define MC 6

static int median(int *data, int count) {
	int copy[count];
	int i, j, tmp;

	for (i = 0; i < count; i++) {
		copy[i] = data[i];
	}

	for (i = 0; i < count; i++) {
		for (j = 0; j < count - 1; j++) {
			if (copy[j] > copy[j + 1]) {
				tmp = copy[j];
				copy[j] = copy[j + 1];
				copy[j + 1] = tmp;
			}
		}
	}

	return copy[count / 2];
}

static int average(int *data, int count) {
	int i, sum = 0;

	for (i = 0; i < count; i++) {
		sum += data[i];
	}

	return sum / count;
}

static int medianaverage(int *data, int count) {
	int copy[count];
	int i, j, tmp, sum = 0;

	for (i = 0; i < count; i++) {
		copy[i] = data[i];
	}

	for (i = 0; i < count; i++) {
		for (j = 0; j < count - 1; j++) {
			if (copy[j] > copy[j + 1]) {
				tmp = copy[j];
				copy[j] = copy[j + 1];
				copy[j + 1] = tmp;
			}
		}
	}

	for (i = MC / 2 - 2; i < MC / 2 + 2; i++) {
		sum += copy[i];
	}

	return sum / 4;
}
#endif

THREAD_ENTRY() {
	int i;

	THREAD_INIT();

	float error_x = 0, error_x_last = 0, error_x_diff = 0, error_x_sum = 0;
	float error_y = 0, error_y_last = 0, error_y_diff = 0, error_y_sum = 0;

	int p_p_b_x, p_p_b_y;
#if 0
	int p_p_b_x_m[MC], p_p_b_y_m[MC];
#endif

	while (1) {
		uint32_t pos = MBOX_GET(touch_pos);
		uint32_t wait = MBOX_GET(touch_pos);
		MBOX_PUT(performance_perf, 0x10000000);

		int p_p_b_x_n = (pos >> 12) & 0xfff;
		if (((p_p_b_x_n >> 11) & 0x1) == 1) {
			p_p_b_x_n |= 0xfffff000;
		}
		int p_p_b_y_n = (pos >> 0) & 0xfff;
		if (((p_p_b_y_n >> 11) & 0x1) == 1) {
			p_p_b_y_n |= 0xfffff000;
		}

#if 0
		for (i = MC - 1; i > 0; i--) {
			p_p_b_x_m[i] = p_p_b_x_m[i - 1];
			p_p_b_y_m[i] = p_p_b_y_m[i - 1];
		}
		p_p_b_x_m[0] = p_p_b_x_n;
		p_p_b_y_m[0] = p_p_b_y_n;

		p_p_b_x = average(p_p_b_x_m, MC);
		p_p_b_y = average(p_p_b_y_m, MC);
#endif

		p_p_b_x = p_p_b_x_n;
		p_p_b_y = p_p_b_y_n;
		float delta = wait / 100000.0;

		debug("position of ball on plate %d %d", p_p_b_x, p_p_b_y);
		printf("position of ball on plate %d %d vs %d %d\n", p_p_b_x, p_p_b_y, p_p_b_x_n, p_p_b_y_n);


		// implement PID controller for x
		error_x = p_p_b_x;
		error_x_diff = (error_x - error_x_last) / delta;
		error_x_sum += error_x * delta;
		error_x_last = error_x;

		float ctrl_x_p = KP * error_x;
		float ctrl_x_i = KI * error_x_sum;
		float ctrl_x_d = KD * error_x_diff;
		float ctrl_x = ctrl_x_p + ctrl_x_i + ctrl_x_d;


		// implement PID controller for y
		error_y = p_p_b_y;
		error_y_diff = (error_y - error_y_last) / delta;
		error_y_sum += error_y * delta;
		error_y_last = error_y;

		float ctrl_y_p = KP * error_y;
		float ctrl_y_i = KI* error_y_sum;
		float ctrl_y_d = KD * error_y_diff;
		float ctrl_y = ctrl_y_p + ctrl_y_i + ctrl_y_d;

		
		// calculate plate rotation
		float len = sqrt(ctrl_x * ctrl_x + ctrl_y * ctrl_y);

		float t_p2b_ra_x = -ctrl_y / len;
		float t_p2b_ra_y = ctrl_x / len;

		// write command to inverse threads
		uint32_t cmd_x = fltofi(t_p2b_ra_x, 10, 2);
		uint32_t cmd_y = fltofi(t_p2b_ra_y, 10, 2);
		uint32_t cmd_a = len;

		//printf("angle %f\n", len);

#if 0
		for (i = 0; i < 6; i++) {
			MBOX_PUT(inverse_cmd, ((cmd_x << 22) | (cmd_y << 12) | (cmd_a << 3) | (i << 0)));
		}
		MBOX_PUT(performance_perf, 0x11000000);
#endif
	}
}
