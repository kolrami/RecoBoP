#include "ap_int.h"

extern void scale(ap_uint<12> x_u, ap_uint<12> y_u,
                  ap_int<12> &x_s, ap_int<12> &y_s);

int main(int argc, char **argv) {
	ap_uint<12> x_u[5] = {12,22,3,6,5};
	ap_uint<12> y_u[5] = {2,6,1,3,0};
	ap_int<12> x_s, y_s;

	scale(100, 100, x_s, y_s);

	printf("scaled to (%d,%d)", x_s.to_int(), y_s.to_int());

	return 0;
}
