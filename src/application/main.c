#include "recobop.h"

#include "reconos.h"
#include "reconos_app.h"
#include "mbox.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define MAX_ANGLE 60
#define SLEEP 20000

volatile struct recobop_info rb_info;

inline double radians(double deg) {
	return deg * (M_PI / 180.0);
}

int main(int argc, char **argv) {
	int a, i;
	int x, y;
	uint32_t s, c;
	char line[128];
	printf("Hello World\n");

	reconos_init();
	reconos_app_init();

	printf("Initializing Info\n");
	rb_info.ctrl_touch_wait = 10000;

	reconos_thread_createi_hwt_servo((void *)&rb_info);
	//reconos_thread_createi_hwt_touch((void *)&rb_info);
	//reconos_thread_createi_hwt_vga((void *)&rb_info);
	reconos_thread_createi_swt_control((void *)&rb_info);
	reconos_thread_createi_swt_inverse((void *)&rb_info);
	//reconos_thread_createi_swt_web((void *)&rb_info);

	printf("Resetting platform ...\n");
	for (i = 0; i < 6; i++)
		mbox_put(inverse_cmd_ptr, 0 | 0 << 22 | 0 << 12 | 0 << 3 | i << 0);
	sleep(1);

#if 0
	printf("Testing touch controller ...\n");
	mbox_put(touch_pos_ptr, 0 | (100 & 0xfff) << 12 | (100 & 0xfff) << 0);
#endif

#if 1
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

	while(1);

	reconos_cleanup();
	reconos_app_cleanup();
}