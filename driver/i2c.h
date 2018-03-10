/*
 * i2c.h
 *
 *  Created on: 24.02.2018
 *      Author: Sebastian
 */

#ifndef I2C_H_
#define I2C_H_
#include "oVario_Framework.h"


void init_i2c(void);
void i2c_send_char(unsigned char ch_address, unsigned char ch_data);

#endif /* I2C_H_ */
