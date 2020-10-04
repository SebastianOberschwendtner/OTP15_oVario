/**
 * joVario Firmware
 * Copyright (c) 2020 Sebastian Oberschwendtner, sebastian.oberschwendtner@gmail.com
 * Copyright (c) 2020 Jakob Karpfinger, kajacky@gmail.com
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
/**
 ******************************************************************************
 * @file    gps.h
 * @author  JK
 * @version V1.1
 * @date    01-May-2018
 * @brief   Talks to the GPS and module and reads its data.
 ******************************************************************************
 */

#ifndef GPS_H_
#define GPS_H_

#include "oVario_Framework.h"

// ***** DEFINES *****
#define dma_buf_size	512

// neo 6m
// classes
#define nav             0x01
#define rxm             0x02
#define inf             0x04
#define ack             0x05
#define cfg             0x06
#define mon             0x0A
#define aid             0x0B
#define tim             0x0D
#define esf             0x10

// NAV ID
#define posllh 	        0x02
#define timeutc		    0x21
#define clock 		    0x22
#define sol 		    0x06
#define velned 		    0x12
#define svinfo		    0x30


// MON ID
#define hw			    0x09

// ID CFG
#define id_cfg		    0x00


// ***** FUNCTIONS *****
void    gps_register_ipc    (void);
void    gps_get_ipc         (void);
void    gps_init            (void);
void    gps_task            (void);

void    gps_handle_nav      (void);
void    gps_config          (void);

void    gps_send_bytes      (void* ptr, uint8_t no_bytes);
void    gps_set_msg_rate    (uint8_t msg_class, uint8_t msg_id, uint8_t rate);



//float gps_getf();
//void gps_handle_vtg();
//void gps_handle_gga();

void    gps_set_baud        (unsigned int baud);
void    gps_SetSysTime      (void);
void    gps_SetSysDate      (void);


#endif /* GPS_H_ */
