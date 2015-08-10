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
	int x_diff, y_diff;
	int x_last = 0, y_last = 0;

	while (1) {
		x = rbi_saw_pos_x(rb_info);
		y = rbi_saw_pos_y(rb_info);

		x_diff = x_last - x;
		y_diff = y_last - y;

		printf("Checking %d %d %d %d...\n", abs(x), abs(y), abs(x_diff), abs(y_diff));

		if (abs(x) < 30 && abs(y) < 30 && abs(x_diff) < 10 && abs(y_diff) < 10) {
			printf("Reducing ...\n");
			rb_info->ctrl_touch_wait = 10000000;
		} else {
			printf("Increasing ...\n");
			rb_info->ctrl_touch_wait = 1000000;
		}

		x_last = x;
		y_last = y;

		usleep(1000000);
	}

	THREAD_EXIT();
}
