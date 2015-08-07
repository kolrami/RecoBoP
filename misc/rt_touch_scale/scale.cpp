#include "ap_int.h"

ap_uint<12> x_avg[16], y_avg[16];

void scale(ap_uint<12> x_u, ap_uint<12> y_u,
           ap_int<12> &x_s, ap_int<12> &y_s,
           ap_uint<4> average) {
	for (int i = 15; i > 0; i--) {
		x_avg[i] = x_avg[i - 1];
		y_avg[i] = y_avg[i - 1];
	}
	x_avg[0] = x_u;
	y_avg[0] = y_u;

	ap_uint<16> sum_x = 0, sum_y = 0;
	for (int i = 0; i < average; i++) {
		sum_x += x_avg[i];
		sum_y += y_avg[i];
	}

	x_s = (((sum_x / average) - 2048) * -1) - 160;
	y_s = (((sum_y / average) - 2048) * 0.754) + 145;
}
