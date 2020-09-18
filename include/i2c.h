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
#define i2c_addr_MS5611 	0xEE
#define i2c_addr_BMS		0x12
#define i2c_addr_BMS_GAUGE	0xAA

//Commands for arbiter
#define I2C_CMD_INIT        1
#define I2C_CMD_SEND_CHAR   2
#define I2C_CMD_SEND_INT    3
#define I2C_CMD_SEND_24BIT  4
#define I2C_CMD_READ_INT    5
#define I2C_CMD_READ_24BIT  6

//Sequences for commands
#define I2C_SEQUENCE_WAIT       1
#define I2C_SEQUENCE_REQUEST    2
#define I2c_SEQUENCE_RX         3


// ***** Functions *****
void            i2c_register_ipc        (void);
void            i2c_task                (void);
void 			i2c_init_peripheral     (void);
void            i2c_idle                (void);
void            i2c_send_char           (void);
void            i2c_send_int            (void);
void            i2c_send_24bit          (void);
void            i2c_read_int            (void);
void            i2c_read_24bit          (void);
unsigned char   i2c_decode_did          (unsigned char did);
unsigned long   i2c_transmit_nbytes     (unsigned char* data, unsigned long nbytes);
void            i2c_dma_transmit        (void* data, unsigned long nbytes);
unsigned long   i2c_receive_nbytes      (unsigned char* data, unsigned long nbytes);
void            i2c_dma_receive         (void* data, unsigned long nbytes);

#endif /* I2C_H_ */
