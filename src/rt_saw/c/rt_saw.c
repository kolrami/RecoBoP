#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

#include <math.h>
#include <unistd.h>


THREAD_ENTRY() {
	struct recobop_info *rb_info;

	THREAD_INIT();
	rb_info = (struct recobop_info *)GET_INIT_DATA();

	int x, y;
	float angle;
	int t = 0;

	while (1) {
		x = rbi_saw_pos_x(rb_info);
		y = rbi_saw_pos_y(rb_info);
		angle = rbi_ctrl_angle(rb_info);

		uint32_t wait = rb_info->ctrl_touch_wait;
		if (angle <= 0.5) {
			wait += 100000;
		} else {
			wait /= 2;
		}

		if (wait < 1500000) {
			wait = 1500000;
		} else if (wait > 10000000) {
			wait = 10000000;
		}

		//printf("%d %f %f\n", t, wait / 100000.0, rbi_saw_power(rb_info));
		//printf("%d %d %d %f %f\n", t, x, y, angle, wait / 100000.0);
		rb_info->ctrl_touch_wait = wait;

		usleep(rb_info->ctrl_touch_wait / 100);
		t += rb_info->ctrl_touch_wait / 100;
	}

	THREAD_EXIT();
}
