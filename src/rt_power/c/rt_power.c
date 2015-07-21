#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

// be careful to use the include from the i2c-tools (v3.1)
#include "i2c-dev.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

void *rt_power(void * data) {
	int i2c;
	char buf[10];

	int vsense, vin;

	THREAD_INIT();

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

		usleep(10000);
	}

	close(i2c);

	THREAD_EXIT();
}