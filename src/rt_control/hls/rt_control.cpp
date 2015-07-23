#ifdef __RECONOS__
#include "reconos_thread.h"
#include "reconos_calls.h"
#endif

#include "ap_int.h"
#include "ap_fixed.h"
#include "math.h"

#ifdef __RECONOS__
THREAD_ENTRY() {
	THREAD_INIT();
#else
	ap_uint<32> control(ap_uint<32> pos, ap_uint<32> delta) {
#endif

		ap_ufixed<22,12> error, error_last, error_diff, error_sum = 0;

#ifdef __RECONOS__
		while (1) {
#endif

#ifdef __RECONOS__
		ap_uint<32> pos = MBOX_GET(touch_pos);
		ap_uint<32> delta = MBOX_GET(touch_pos);
#endif

		ap_uint<12> pos_x = pos(23, 12);
		ap_uint<12> pos_y = pos(11, 0);

		ap_int<12> p_p_b_x, p_p_b_y;
		for (int i = 0; i < 12; i++) {
			p_p_b_x[i] = pos_x[i];
			p_p_b_y[i] = pos_y[i];
		}

#ifndef __SYNTHESIS__
		printf("position of ball on plate %d %d\n", p_p_b_x.to_int(), p_p_b_y.to_int());
#endif


		// calculate rotation vector
		ap_fixed<10,2> t_p2b_ra_x = p_p_b_y / sqrt(p_p_b_x * p_p_b_x + p_p_b_y * p_p_b_y);
		ap_fixed<10,2> t_p2b_ra_y = - p_p_b_x / sqrt(p_p_b_x * p_p_b_x + p_p_b_y * p_p_b_y);

#ifndef __SYNTHESIS__
		printf("rotation vector %f %f\n", t_p2b_ra_x.to_float(), t_p2b_ra_y.to_float());
#endif


		// implement PID controller
		error = sqrt(p_p_b_x * p_p_b_x + p_p_b_y * p_p_b_y);
		error_diff = (error - error_last) / delta;
		error_sum += error * delta;
		if (error_sum > 100) error_sum = 100;
		if (error_sum < -100) error_sum = -100;
		error_last = error;

#ifndef __SYNTHESIS__
		printf("error is %f\n", error.to_float());
#endif

		ap_ufixed<22,12> ctrl_p = ap_ufixed<22,12>(0.03) * error;
		ap_ufixed<22,12> ctrl_i = ap_ufixed<22,12>(0) * error_diff;
		ap_ufixed<22,12> ctrl_d = ap_ufixed<22,12>(0) * error_sum;
		ap_ufixed<22,12> ctrl = ctrl_p + ctrl_i + ctrl_d;

#ifndef __SYNTHESIS__
		printf("controlled angle %f (%d)\n", ctrl.to_float(), ctrl.to_int());
#endif

		ap_uint<10> cmd_x, cmd_y;
		// ugly workaround for bit selection
		for (int i = 0; i < 10; i++) {
			cmd_x[i] = t_p2b_ra_x[i];
			cmd_y[i] = t_p2b_ra_y[i];
		}

		ap_uint<9> cmd_a = ctrl;

#ifdef __RECONOS__
		for (int i = 0; i < 6; i++)
			MBOX_PUT(inverse_cmd, (cmd_x, cmd_y, cmd_a, (ap_uint<3>)i));
#else
		return (cmd_x, cmd_y, cmd_a, (ap_uint<3>)0);
#endif

#ifdef __RECONOS__
	}
#endif
}
