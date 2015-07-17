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

inline double radians(double deg) {
	return deg * (M_PI / 180.0);
}

inline uint32_t fixed(double d) {
	int i;
	uint32_t r;
	double d_old = d;

	if (d < 0) {
		r = 1;
		d = -d;
	} else {
		r = 0;
	}

	for (i = 0; i >= -8; i--) {
		if (d >= pow(2, i)) {
			r |= 1 << (1 - i);
			d -= pow(2, i);
		}
	}

	return r;
}

int main(int argc, char **argv) {
	int a, i;
	uint32_t s, c;
	char line[128];
	printf("Hello World\n");

	reconos_init();
	reconos_app_init();

	reconos_thread_create_hwt_servo();
	//reconos_thread_create_hwt_touch();
	reconos_thread_create_hwt_vga();
	reconos_thread_create_hwt_inverse();

	printf("Resetting platform ...\n");
	for (i = 0; i < 6; i++)
		mbox_put(inverse_cmd_ptr, 0 | fixed(0) << 22 | fixed(0) << 12 | 0 << 3 | i << 0);
	sleep(1);

	printf("Outputting VGA ...\n");
	for (i = 0; i < 800; i++) {
		mbox_put(touch_pos_ptr, 0 | i << 12 | 0 << 0);
		usleep(10000);
	}


#if 0
	while(1) {
		for (a = 0; a < MAX_ANGLE; a++) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | fixed(1) << 22 | fixed(0) << 12 | a << 3 | i << 0);
			usleep(SLEEP);
		}

		for (a = MAX_ANGLE; a > 0; a--) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | fixed(1) << 22 | fixed(0) << 12 | a << 3 | i << 0);
			usleep(SLEEP);
		}

		for (a = 0; a > -MAX_ANGLE; a--) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | fixed(-1) << 22 | fixed(0) << 12 | -a << 3 | i << 0);
			usleep(SLEEP);
		}

		for (a = -MAX_ANGLE; a < 0; a++) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | fixed(-1) << 22 | fixed(0) << 12 | -a << 3 | i << 0);
			usleep(SLEEP);
		}
	}

#endif

#if 0
	while (1) {
		for (a = 95; a < 180; a++) {
			s = fixed(sin(radians(a)));
			c = fixed(cos(radians(a)));
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | c << 22 | s << 12 | MAX_ANGLE << 3 | i << 0);
			usleep(SLEEP);
		}
	}
#endif

#if 0
	while(1) {
		fgets(line, 128, stdin);
		a = strtol(line, NULL, 10);
		//for (a = 400; a <= 1600; a+=200) {
			for (i = 0; i < 6; i++) {
				mbox_put(servo_cmd_ptr, a << 21 | i << 18);
			}
		//	usleep(300000);
		//}
	}
#endif

	while(1);

	reconos_cleanup();
	reconos_app_cleanup();
}