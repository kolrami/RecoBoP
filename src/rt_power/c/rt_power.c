#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

// be careful to use the include from the i2c-tools (v3.1)
#include "i2c-dev.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

THREAD_ENTRY() {
	struct recobop_info *rb_info;

	int i2c;
	int vsense, vin;

	THREAD_INIT();
	rb_info = (struct recobop_info *)GET_INIT_DATA();

	i2c = open("/dev/i2c-0", O_RDWR);
	if (i2c < 0) {
		panic("Failed to open i2c device");
	}

	if (ioctl(i2c, I2C_SLAVE, 0x6a) < 0) {
		close(i2c);
		panic("Faield to acquire bus access");
	}

	if (i2c_smbus_write_byte_data(i2c, 0x06, 0x0) < 0) {
		close(i2c);
		panic("Failed to setup control register");
	}

	while(1) {
		vsense = (i2c_smbus_read_byte_data(i2c, 0x00) << 4) | (i2c_smbus_read_byte_data(i2c, 0x01) >> 4);
		vin = (i2c_smbus_read_byte_data(i2c, 0x02) << 4) | (i2c_smbus_read_byte_data(i2c, 0x03) >> 4);	

		debug("Voltage: V_in = %fV, V_sense = %fmV => P = %fW", vin * 0.025, vsense * 0.02, vsense * vin * 0.00005);
		rb_info->saw_vin = vin * 0.025;
		rb_info->saw_vsense = vsense * 0.02;
		rb_info->saw_power = vin * vsense * 0.00005;

		usleep(200000);
	}

	close(i2c);

	THREAD_EXIT();
}
