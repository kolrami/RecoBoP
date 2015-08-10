#ifdef __RECONOS__
#include "reconos_thread.h"
#include "reconos_calls.h"
#endif

#include "ap_int.h"
#include "ap_fixed.h"
#include "math.h"

#define KP -0.08
#define KI -0.00004
#define KD -25

#define MC 8
#define MCL 3

static ap_fixed<22,12> average(ap_fixed<22,12> *data) {
	ap_fixed<22 + MCL,15 + MCL> sum = 0;

	for (int i = 0; i < MC; i++) {
		sum += data[i];
	}

	return sum / MC;
}

#ifdef __RECONOS__
THREAD_ENTRY() {
	THREAD_INIT();
#else
	ap_uint<32> control(ap_uint<32> pos, ap_uint<32> wait) {
#endif

	ap_fixed<22,12> error_x, error_x_last, error_x_diff, error_x_sum = 0;
	ap_fixed<22,12> error_y, error_y_last, error_y_diff, error_y_sum = 0;

	ap_fixed<22,12> error_x_diff_m[MC], error_y_diff_m[MC];

#ifdef __RECONOS__
	while (1) {
#endif

#ifdef __RECONOS__
		ap_uint<32> pos = MBOX_GET(touch_pos);
		ap_uint<32> wait = MBOX_GET(touch_pos);
		MBOX_PUT(performance_perf, (ap_uint<8>("10", 16), ap_uint<24>(0)));
#endif

		ap_uint<12> pos_x = pos(23, 12);
		ap_uint<12> pos_y = pos(11, 0);

		ap_int<12> p_p_b_x, p_p_b_y;
		for (int i = 0; i < 12; i++) {
			p_p_b_x[i] = pos_x[i];
			p_p_b_y[i] = pos_y[i];
		}

		ap_ufixed<22,12> delta = wait / 100000;

#ifndef __SYNTHESIS__
		printf("position of ball on plate %d %d\n", p_p_b_x.to_int(), p_p_b_y.to_int());
#endif


		// implement PID controller for x
		error_x = p_p_b_x;
		error_x_diff = (error_x - error_x_last) / delta;
		error_x_sum += error_x * delta;
		error_x_last = error_x;

		for (int i = MC - 1; i > 0; i--) {
			error_x_diff_m[i] = error_x_diff_m[i - 1];
			error_x_diff_m[i] = error_x_diff_m[i - 1];
		}
		error_x_diff_m[0] = error_x_diff;
		error_x_diff_m[0] = error_x_diff;
		error_x_diff = average(error_x_diff_m);

		ap_fixed<22,12> ctrl_x_p = ap_fixed<22,12>(KP) * error_x;
		ap_fixed<22,12> ctrl_x_i = ap_fixed<22,12>(KI) * error_x_sum;
		ap_fixed<22,12> ctrl_x_d = ap_fixed<22,12>(KD) * error_x_diff;
		ap_fixed<22,12> ctrl_x = ctrl_x_p + ctrl_x_i + ctrl_x_d;


		// implement PID controller for y
		error_y = p_p_b_y;
		error_y_diff = (error_y - error_y_last) / delta;
		error_y_sum += error_y * delta;
		error_y_last = error_y;

		for (int i = MC - 1; i > 0; i--) {
			error_y_diff_m[i] = error_y_diff_m[i - 1];
			error_y_diff_m[i] = error_y_diff_m[i - 1];
		}
		error_y_diff_m[0] = error_y_diff;
		error_y_diff_m[0] = error_y_diff;
		error_y_diff = average(error_y_diff_m);

		ap_fixed<22,12> ctrl_y_p = ap_fixed<22,12>(KP) * error_y;
		ap_fixed<22,12> ctrl_y_i = ap_fixed<22,12>(KI) * error_y_sum;
		ap_fixed<22,12> ctrl_y_d = ap_fixed<22,12>(KD) * error_y_diff;
		ap_fixed<22,12> ctrl_y = ctrl_y_p + ctrl_y_i + ctrl_y_d;


		// calculate plate rotation
		ap_ufixed<22,12> len = sqrt(ctrl_x * ctrl_x + ctrl_y * ctrl_y);

		ap_fixed<10,2> t_p2b_ra_x = -ctrl_y / len;
		ap_fixed<10,2> t_p2b_ra_y = ctrl_x / len;

		ap_uint<10> cmd_x, cmd_y;
		// ugly workaround for bit selection
		for (int i = 0; i < 10; i++) {
			cmd_x[i] = t_p2b_ra_x[i];
			cmd_y[i] = t_p2b_ra_y[i];
		}
		ap_uint<9> cmd_a = len;

#ifdef __RECONOS__
		MBOX_PUT(performance_perf, (ap_uint<8>("11", 16), ap_uint<24>(0)));
		for (int i = 0; i < 6; i++) {
			MBOX_PUT(inverse_cmd, (cmd_x, cmd_y, cmd_a, (ap_uint<3>)i));
		}
#else
		return (cmd_x, cmd_y, cmd_a, (ap_uint<3>)0);
#endif

#ifdef __RECONOS__
	}
#endif
}
