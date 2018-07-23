/*
 * AS5600.c
 *
 *  Created on: 15.04.2018
 *      Author: dk
 */

#include "mgos.h"
#include "mgos_i2c.h"

#define AS5600_ADR 0x36
#define RAW_ANGLE_HI 0x0C
#define RAW_ANGLE_LO 0x0D

int as5600_get_raw_angle(void) {
	return mgos_i2c_read_reg_w(mgos_i2c_get_global(), AS5600_ADR, RAW_ANGLE_HI);
}
