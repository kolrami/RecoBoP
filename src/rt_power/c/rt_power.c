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
		vsense = i2c_smbus_read_word_data(i2c, 0x00) >> 4;
		vin = i2c_smbus_read_word_data(i2c, 0x02) >> 4;

		debug("Voltage: V_in = %d, V_sense = %d", vin, vsense);
		rb_info->saw_vsense = vsense;
		rb_info->saw_power = vsense / 100;

		usleep(10000);
	}

	close(i2c);

	THREAD_EXIT();
}