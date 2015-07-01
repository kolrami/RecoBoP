#include "reconos.h"
#include "reconos_app.h"
#include "mbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define MAX_ANGLE 90
#define SLEEP 1000

int main(int argc, char **argv) {
	int a, i;
	char line[128];
	printf("Hello World\n");

	reconos_init();
	reconos_app_init();

	reconos_thread_create_hwt_servo();
	reconos_thread_create_hwt_inverse();

	for (i = 0; i < 6; i++)
		mbox_put(inverse_cmd_ptr, 0 | 1 << 22 | 0 << 12 | 0 << 3 | i << 0);
	sleep(1);


#if 1
	while(1) {
		for (a = 0; a < MAX_ANGLE; a++) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | 1 << 22 | 0 << 12 | a << 3 | i << 0);
			usleep(SLEEP);
		}

		for (a = MAX_ANGLE; a > 0; a--) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | 1 << 22 | 0 << 12 | a << 3 | i << 0);
			usleep(SLEEP);
		}

		for (a = 0; a > -MAX_ANGLE; a--) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | -1 << 22 | 0 << 12 | -a << 3 | i << 0);
			usleep(SLEEP);
		}

		for (a = -MAX_ANGLE; a < 0; a++) {
			for (i = 0; i < 6; i++)
				mbox_put(inverse_cmd_ptr, 0 | -1 << 22 | 0 << 12 | -a << 3 | i << 0);
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