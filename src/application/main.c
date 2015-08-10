#include "recobop.h"

#include "reconos.h"
#include "reconos_app.h"
#include "mbox.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <signal.h>

#define MAX_ANGLE 60
#define SLEEP 20000

volatile struct recobop_info rb_info;

inline double radians(double deg) {
	return deg * (M_PI / 180.0);
}

static void exit_signal(int sig) {
	reconos_cleanup();

	printf("[recobop] aborted\n");

	exit(0);
}

int main(int argc, char **argv) {
	int a, i;
	int x, y;
	uint32_t s, c, pos;
	uint32_t m = 0xffffffff;
	int pos_x, pos_y;
	char line[128];
	struct reconos_thread *rt;
	printf("Hello World\n");
	printf("int is %d and float is %d\n", sizeof(int), sizeof(float));

	reconos_init();
	reconos_app_init();

	signal(SIGINT, exit_signal);
	signal(SIGTERM, exit_signal);
	signal(SIGABRT, exit_signal);

	printf("Initializing Info\n");
	rb_info.ctrl_touch_wait = 1000000;

	reconos_thread_createi_hwt_performance((void *)&rb_info);
	reconos_thread_createi_hwt_servo((void *)&rb_info);
	reconos_thread_createi_swt_control((void *)&rb_info);
	reconos_thread_createi_hwt_inverse((void *)&rb_info);
	reconos_thread_createi_hwt_inverse((void *)&rb_info);
	reconos_thread_createi_hwt_inverse((void *)&rb_info);
	reconos_thread_createi_hwt_touch((void *)&rb_info);
	//reconos_thread_createi_hwt_vga((void *)&rb_info);
	reconos_thread_createi_swt_web((void *)&rb_info);
	reconos_thread_createi_swt_power((void *)&rb_info);
	reconos_thread_createi_swt_saw((void *)&rb_info);

#if 0
	while (1) {
		printf("%f\n", rbi_saw_power(&rb_info));
		usleep(50000);
	}
#endif

#if 0
	printf("Resetting platform ...\n");
	for (i = 0; i < 6; i++)
		mbox_put(inverse_cmd_ptr, 0 | 0 << 22 | 0 << 12 | 0 << 3 | i << 0);
	sleep(1);
#endif

#if 0
	printf("Testing touch controller ...\n");
	while(1) {
		pos = mbox_get(touch_pos_ptr);
		pos_x = (pos >> 12) & 0xfff;
		if (((pos_x >> 11) & 0x1) == 1) {
			pos_x |= m << 12;
		}
		pos_y = (pos >> 0) & 0xfff;
		if (((pos_y >> 11) & 0x1) == 1) {
			pos_y |= m << 12;
		}
		printf("touch position is %d,%d\n", pos_x, pos_y);
		mbox_get(touch_pos_ptr);
	}
#endif

#if 0
	printf("Testing control algorithm ...\n");
	for (a = 0; a < 10000000; a+=1) {
		x = cos(a * M_PI / 180) * 2000;
		y = sin(a * M_PI / 180) * 2000;
		printf("%d,%d\n", x, y);
		mbox_put(touch_pos_ptr, 0 | (x & 0xfff) << 12 | (y & 0xfff) << 0);
		mbox_put(touch_pos_ptr, 1000000);
		usleep(10000);
	}
#endif

	while(1) {
		sleep(1000);
	}

	reconos_cleanup();
	reconos_app_cleanup();
}