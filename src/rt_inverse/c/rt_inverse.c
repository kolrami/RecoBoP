#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>

// definitions of stewart platform

// coordinates of platform joints in base coordinates
int p_p_j_x[6] = {51,-51,-64,-13,13,64};
int p_p_j_y[6] = {44,44,22,-66,-66,22};

// transformation to base coordinates
float t_p2b_t_z = 80 + 21;

// transformation to servo coordinates
float t_b2s_rz_sin[6] = {0.00000000,0.00000000,-0.86602540,-0.86602540,0.86602540,0.86602540};
float t_b2s_rz_cos[6] = {1.00000000,1.00000000,-0.50000000,-0.50000000,-0.50000000,-0.50000000};
int t_b2s_s_x[6] = {-1,1,-1,1,-1,1};
int t_b2s_s_y[6] = {1,1,1,1,1,1};
int t_b2s_s_z[6] = {1,-1,1,-1,1,-1};
int t_b2s_t_x = 33;
int t_b2s_t_y = -69;
int t_b2s_t_z = 0;

// leg and arm lengths
int c_leg = 115;
int c_legs = 115 * 115;
int c_arm = 20;

// sine and cosine lookup tables
#define TRIG_COUNT 2048
#define TRIG_ADDR 11
#define TRIG_MIN_ANGLE_DEG 0
#define TRIG_MAX_ANGLE_DEG 204.7
#define TRIG_STEP 0.1
static inline float radians(float deg) {
	return deg * (M_PI / 180.0);
}
static inline float sin_lut(unsigned int a) {
	return sin(radians(a * TRIG_STEP));
}
static inline float cos_lut(unsigned int a) {
	return cos(radians(a * TRIG_STEP));
}

THREAD_ENTRY() {
	int i;

	THREAD_INIT();

	while (1) {
		uint32_t data = MBOX_GET(inverse_cmd);

		float t_p2b_ra_x = fitofl((data >> 22) & 0x3ff, 10, 2);
		float t_p2b_ra_y = fitofl((data >> 12) & 0x3ff, 10, 2);
		float t_p2b_ra_sin = sin_lut((data >> 3) & 0x1ff);
		float t_p2b_ra_cos = cos_lut((data >> 3) & 0x1ff);
		int leg = (data >> 0) & 0x7;

		debug("x rot %f and y rot %f", t_p2b_ra_x, t_p2b_ra_y);
		debug("sine %f and cosine %f", t_p2b_ra_sin, t_p2b_ra_cos);

		float p_b_j_x, p_b_j_y, p_b_j_z;
		float p_s_j_x, p_s_j_y, p_s_j_z;

		float v_s_aj_x, v_s_aj_y, v_s_aj_z;
		float v_s_aj_l_min, v_s_aj_l;
		unsigned int v_s_aj_l_mina;


		// transform into base coordinatesystem
		// rotate around ra_x, ra_y, ra_z by ra_sin/ra_cos
		p_b_j_x =   (t_p2b_ra_x * t_p2b_ra_x * (1 - t_p2b_ra_cos) + 1          * t_p2b_ra_cos) * p_p_j_x[leg]
		          + (t_p2b_ra_x * t_p2b_ra_y * (1 - t_p2b_ra_cos) - 0          * t_p2b_ra_sin) * p_p_j_y[leg];
		p_b_j_y =   (t_p2b_ra_x * t_p2b_ra_y * (1 - t_p2b_ra_cos) + 0          * t_p2b_ra_sin) * p_p_j_x[leg]
		          + (t_p2b_ra_y * t_p2b_ra_y * (1 - t_p2b_ra_cos) + 1          * t_p2b_ra_cos) * p_p_j_y[leg];
		p_b_j_z =   (t_p2b_ra_x * 0          * (1 - t_p2b_ra_cos) - t_p2b_ra_y * t_p2b_ra_sin) * p_p_j_x[leg]
		          + (t_p2b_ra_y * 0          * (1 - t_p2b_ra_cos) + t_p2b_ra_x * t_p2b_ra_sin) * p_p_j_y[leg];

		debug("joint for base coord after rotation %f %f %f", p_b_j_x, p_b_j_y, p_b_j_z);

		// translate by t_Z
		p_b_j_z = p_b_j_z + t_p2b_t_z;

		debug("joint in base coord %f %f %f", p_b_j_x, p_b_j_y, p_b_j_z);


		// transform into servo coordinatesystem based on leg id
		// rotate around z axis by rz_sin/rz_cos
		p_s_j_x = t_b2s_rz_cos[leg] * p_b_j_x - t_b2s_rz_sin[leg] * p_b_j_y + 0 * p_b_j_z;
		p_s_j_y = t_b2s_rz_sin[leg] * p_b_j_x + t_b2s_rz_cos[leg] * p_b_j_y + 0 * p_b_j_z;
		p_s_j_z = 0                 * p_b_j_x + 0                 * p_b_j_y + 1 * p_b_j_z;

		// scale axis by s_x, s_y and s_z
		p_s_j_x = t_b2s_s_x[leg] * p_s_j_x;
		p_s_j_y = t_b2s_s_y[leg] * p_s_j_y;
		p_s_j_z = t_b2s_s_z[leg] * p_s_j_z;

		// translate by t_x, t_y and t_z
		p_s_j_x = p_s_j_x + t_b2s_t_x;
		p_s_j_y = p_s_j_y + t_b2s_t_y;
		p_s_j_z = p_s_j_z + t_b2s_t_z;

		debug("joint in servo coord %f %f %f", p_s_j_x, p_s_j_y, p_s_j_z);


		// find angle for leg length
		v_s_aj_l_min = FLT_MAX;
		for (i = 0; i < TRIG_COUNT; i++) {
			// calculate vector from arm to joint
			v_s_aj_x = p_s_j_x - sin_lut(i) * c_arm;
			v_s_aj_y = p_s_j_y - 0;
			v_s_aj_z = p_s_j_z - cos_lut(i) * c_arm;
			v_s_aj_l = v_s_aj_x * v_s_aj_x + v_s_aj_y * v_s_aj_y + v_s_aj_z * v_s_aj_z;

			if (c_legs < v_s_aj_l) {
				if (v_s_aj_l - c_legs < v_s_aj_l_min) {
					v_s_aj_l_min = v_s_aj_l - c_legs;
					v_s_aj_l_mina = i;
				}
			} else {
				if (c_legs - v_s_aj_l < v_s_aj_l_min) {
					v_s_aj_l_min = c_legs - v_s_aj_l;
					v_s_aj_l_mina = i;
				}
			}
		}

		debug("angle %d with length diff %f", v_s_aj_l_mina, v_s_aj_l_min);

		MBOX_PUT(servo_cmd, ((v_s_aj_l_mina << 21) | (leg << 18) | 0));
	}
}
