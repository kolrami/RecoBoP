#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>

#define KP -0.09
//#define KI -0.00004
#define KI 0
#define KD -28

#define MC 16
#define MCL 4

static float average(float *data, int mc) {
	int i;
	float sum = 0;

	for (i = 0; i < mc; i++) {
		sum += data[i];
	}

	return sum / mc;
}

THREAD_ENTRY() {
	struct recobop_info *rb_info;
	int i;

	THREAD_INIT();
	rb_info = (struct recobop_info *)GET_INIT_DATA();

	float error_x = 0, error_x_diff = 0, error_x_sum = 0;
	float error_y = 0, error_y_diff = 0, error_y_sum = 0;

	int p_p_b_x_last = 0, p_p_b_y_last = 0;

	float error_x_diff_m[MC], error_y_diff_m[MC];

	float dd = 0;
	int target_x, target_y;

	while (1) {
		uint32_t pos = MBOX_GET(touch_pos);
		uint32_t wait = MBOX_GET(touch_pos);
		MBOX_PUT(performance_perf, 0x10000000);

		int p_p_b_x = (pos >> 12) & 0xfff;
		if (((p_p_b_x >> 11) & 0x1) == 1) {
			p_p_b_x |= 0xfffff000;
		}
		int p_p_b_y = (pos >> 0) & 0xfff;
		if (((p_p_b_y >> 11) & 0x1) == 1) {
			p_p_b_y |= 0xfffff000;
		}

		float delta = wait / 100000.0;
		int mc = -(int)delta / 6 + 17;
		mc = 4;

		debug("position of ball on plate %d %d", p_p_b_x, p_p_b_y);
		//printf("position of ball on plate %d %d (%f)\n", p_p_b_x, p_p_b_y, delta);
		printf("%d %d %f %f\n", p_p_b_x, p_p_b_y, (16*sin(dd) * sin(dd) * sin(dd)) * 38, (13*cos(dd) - 5*cos(2*dd) - 2*cos(3*dd) - cos(4*dd)) * 38);

#if 1
		dd += 0.01;
		target_x = (16*sin(dd) * sin(dd) * sin(dd)) * 38;
		target_y = (13*cos(dd) - 5*cos(2*dd) - 2*cos(3*dd) - cos(4*dd)) * 38;
#endif

#if 0
		target_x = 600 * cos(dd);
		target_y = 600 * sin(dd);
#endif

#if 0
		target_x = 0;
		target_y = 0;
#endif

		// calculate errors
		error_x = p_p_b_x - target_x;
		error_y = p_p_b_y - target_y;


		// implement PID controller for x
		error_x_diff = (p_p_b_x - p_p_b_x_last) / delta;
		error_x_sum += error_x * delta;

		for (i = MC - 1; i > 0; i--) {
			error_x_diff_m[i] = error_x_diff_m[i - 1];
		}
		error_x_diff_m[0] = error_x_diff;
		error_x_diff = average(error_x_diff_m, mc);

		float ctrl_x_p = KP * error_x;
		float ctrl_x_i = KI * error_x_sum;
		float ctrl_x_d = KD * error_x_diff;
		float ctrl_x = ctrl_x_p + ctrl_x_i + ctrl_x_d;


		// implement PID controller for y
		error_y_diff = (p_p_b_y - p_p_b_y_last) / delta;
		error_y_sum += error_y * delta;

		for (i = MC - 1; i > 0; i--) {
			error_y_diff_m[i] = error_y_diff_m[i - 1];
		}
		error_y_diff_m[0] = error_y_diff;
		error_y_diff = average(error_y_diff_m, mc);

		float ctrl_y_p = KP * error_y;
		float ctrl_y_i = KI* error_y_sum;
		float ctrl_y_d = KD * error_y_diff;
		float ctrl_y = ctrl_y_p + ctrl_y_i + ctrl_y_d;


		// store last position
		p_p_b_x_last = p_p_b_x;
		p_p_b_y_last = p_p_b_y;

		
		// calculate plate rotation
		float len = sqrt(ctrl_x * ctrl_x + ctrl_y * ctrl_y);

		float t_p2b_ra_x = -ctrl_y / len;
		float t_p2b_ra_y = ctrl_x / len;

		if (len > 100) {
			len = 100;
		}

		// write command to inverse threads
		uint32_t cmd_x = fltofi(t_p2b_ra_x, 10, 2);
		uint32_t cmd_y = fltofi(t_p2b_ra_y, 10, 2);
		uint32_t cmd_a = len;

		//printf("angle %f\n", len);

#if 1
		MBOX_PUT(performance_perf, 0x11000000);
		for (i = 0; i < 6; i++) {
			MBOX_PUT(inverse_cmd, ((cmd_x << 22) | (cmd_y << 12) | (cmd_a << 3) | (i << 0)));
		}
#endif
		
		// saw
		rb_info->ctrl_angle = len;

	}
}
