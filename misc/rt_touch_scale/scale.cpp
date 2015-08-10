#include "ap_int.h"

void scale(ap_uint<12> x_u, ap_uint<12> y_u,
           ap_int<12> &x_s, ap_int<12> &y_s) {
	x_s = ((x_u - 2048) * -1) - 160;
	y_s = ((y_u - 2048) * 0.754) + 145;
}
