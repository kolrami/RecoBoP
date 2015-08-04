#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>

THREAD_ENTRY() {
	int i;

	THREAD_INIT();

	float error, error_last, error_diff, error_sum = 0;

	while (1) {
		uint32_t pos = MBOX_GET(touch_pos);
		uint32_t wait = MBOX_GET(touch_pos);

		int p_p_b_x = (pos >> 12) & 0xfff;
		if (((p_p_b_x >> 11) & 0x1) == 1) {
			p_p_b_x |= 0xfffff000;
		}
		int p_p_b_y = (pos >> 0) & 0xfff;
		if (((p_p_b_y >> 11) & 0x1) == 1) {
			p_p_b_y |= 0xfffff000;
		}

		float delta = wait / 100000.0;

		debug("position of ball on plate %d %d", p_p_b_x, p_p_b_y);


		// calculate rotation vector
		float t_p2b_ra_x = p_p_b_y / sqrt(p_p_b_x * p_p_b_x + p_p_b_y * p_p_b_y);
		float t_p2b_ra_y = - p_p_b_x / sqrt(p_p_b_x * p_p_b_x + p_p_b_y * p_p_b_y);

		debug("rotation vector %f %f", t_p2b_ra_x, t_p2b_ra_y);


		// implement PID controller
		error = sqrt(p_p_b_x * p_p_b_x + p_p_b_y * p_p_b_y);
		error_diff = (error - error_last) / delta;
		error_sum += error * delta;
		if (error_sum > 100) error_sum = 100;
		if (error_sum < -100) error_sum = -100;
		error_last = error;

		debug("error is %f", error);

		float ctrl_p = 0.03 * error;
		float ctrl_i = 0 * error_diff;
		float ctrl_d = 0 * error_sum;
		float ctrl = ctrl_p + ctrl_i + ctrl_d;

		debug("controlled angle %f (%d)", ctrl, ctrl);

		uint32_t cmd_x = fltofi(t_p2b_ra_x, 10, 2);
		uint32_t cmd_y = fltofi(t_p2b_ra_y, 10, 2);
		uint32_t cmd_a = ctrl;

		for (i = 0; i < 6; i++)
			MBOX_PUT(inverse_cmd, ((cmd_x << 22) | (cmd_y << 12) | (cmd_a << 3) | (i << 0)));
	}
}
