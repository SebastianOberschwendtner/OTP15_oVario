/*
 * gps.h
 *
 *  Created on: 01.05.2018
 *      Author: Admin
 */

#ifndef GPS_H_
#define GPS_H_

//Git ist scheiße!
// ***** DEFINES *****
#define dma_buf_size	512

// ***** FUNCTIONS *****
void gps_init ();
void gps_task ();

float gps_getf();

void gps_handle_vtg();
void gps_handle_gga();


#endif /* GPS_H_ */
