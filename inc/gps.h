/*
 * gps.h
 *
 *  Created on: 01.05.2018
 *      Author: Admin
 */

#ifndef GPS_H_
#define GPS_H_


#include "stdint.h"

// ***** DEFINES *****
#define dma_buf_size	512

// neo 6m
// classes
#define nav 0x01
#define rxm 0x02
#define inf 0x04
#define ack 0x05
#define cfg 0x06
#define mon 0x0A
#define aid 0x0B
#define tim 0x0D
#define esf 0x10

// NAV ID
#define posllh 		0x02
#define timeutc		0x21
#define clock 		0x22
#define sol 		0x06
#define velned 		0x12
#define svinfo		0x30


// MON ID
#define hw			0x09

// ID CFG
#define id_cfg		0x00


// ***** FUNCTIONS *****
void gps_init ();
void gps_task ();

void gps_handle_nav();
void gps_config();

void gps_send_bytes(void* ptr, uint8_t no_bytes);
void gps_set_msg_rate(uint8_t msg_class, uint8_t msg_id, uint8_t rate);



//float gps_getf();
//void gps_handle_vtg();
//void gps_handle_gga();

void gps_set_baud(unsigned int baud);


#endif /* GPS_H_ */
