/*
 * i2c.h
 *
 *  Created on: 24.02.2018
 *      Author: Sebastian
 */

#ifndef I2C_H_
#define I2C_H_
#include "oVario_Framework.h"

// ***** Defines *****
//I2C addresses of sensors
#define i2c_addr_MS5611 0xEE


// ***** Functions *****
void init_i2c(void);
void i2c_send_char(unsigned char ch_address, unsigned char ch_data);
void i2c_send_int(unsigned char ch_address, unsigned int i_data);
void i2c_send_int_LSB(unsigned char ch_address, unsigned int i_data);
unsigned char i2c_read_char(unsigned char ch_address, unsigned char ch_command);
unsigned int i2c_read_int(unsigned char ch_address, unsigned char ch_command);
unsigned int i2c_read_int_LSB(unsigned char ch_address, unsigned char ch_command);
unsigned long i2c_read_24bit(unsigned char ch_address, unsigned char ch_command);
unsigned long i2c_read_long(unsigned char ch_address, unsigned char ch_command);

#endif /* I2C_H_ */
