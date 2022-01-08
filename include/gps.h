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
#define ubx_class_nav             0x01
#define ubx_class_rxm             0x02
#define ubx_class_inf             0x04
#define ubx_class_ack             0x05
#define ubx_class_cfg             0x06
#define ubx_class_mon             0x0A
#define ubx_class_aid             0x0B
#define ubx_class_tim             0x0D
#define ubx_class_esf             0x10

// ID NAV
#define ubx_ID_posllh 	        0x02
#define ubx_ID_navstatus        0x03
#define ubx_ID_timeutc		    0x21
#define ubx_ID_clock 		    0x22
#define ubx_ID_sol 		        0x06
#define ubx_ID_velned 		    0x12
#define ubx_ID_svinfo		    0x30

// ID CFG
#define ubx_ID_msg          0x01
#define ubx_ID_rate         0x08

//  ID MON
#define ubx_ID_hw           0x09

// ID CFG
#define ubx_ID_cfg		    0x00

// ACK ID
#define ubx_ID_nack         0x00
#define ubx_ID_ack          0x01


#define gps_buffer_size 512


// ***** FUNCTIONS *****
void    gps_register_ipc    (void);
void    gps_get_ipc         (void);
uint8_t gps_init            (void);
void    gps_task            (void);

void    gps_handle_nav      (void);
uint8_t gps_handle_ack      (void);
void    gps_config          (uint32_t baudrate);
void gps_config_rates       (void);

void    gps_send_bytes      (void* ptr, uint8_t no_bytes);
void    gps_set_msg_rate    (uint8_t msg_class, uint8_t msg_id, uint8_t rate);
void    gps_set_meas_rate   (uint16_t  rate);

void gps_get_meas_rate      (void);

void gps_poll_nav_stats     (void);

void gps_send_UBX           (uint8_t class, uint8_t ID, uint8_t pl_length, uint8_t* data);

//float gps_getf();
//void gps_handle_vtg();
//void gps_handle_gga();

void    gps_set_baud        (unsigned int baud);
void    gps_SetSysTime      (void);
void    gps_SetSysDate      (void);




//****** Typedefs ******
#pragma pack(push, 1)
typedef struct
{
	uint8_t start;
	uint8_t tID[2];
	uint8_t msgContID[3];
} T_hdr_nmea;

typedef struct
{
	float course;
	float v_knots;
	float v_kmh;
	uint8_t fix;
	uint16_t checksum;
} T_VTG_nmea;

typedef struct
{
	uint16_t sync;
	uint8_t class;
	uint8_t id;
	uint16_t length;
} T_UBX_hdr;

typedef struct
{
	uint32_t iTow;	 // GPS Millisecond Time of Week
	int32_t velN;	 // NED north velocity
	int32_t velE;	 // NED east velocity
	int32_t velD;	 // NED down velocity
	uint32_t speed;	 // Speed (3-D)
	uint32_t gspeed; // Ground Speed (2-D)
	int32_t heading; // Heading of motion 2-D
	uint32_t sAcc;	 // Speed Accuracy Estimate
	uint32_t cAcc;	 // Course / Heading Accuracy Estimate
} T_UBX_VELNED;

typedef struct
{
	uint32_t iTow;
	uint32_t fTow;
	uint16_t week;
	uint8_t GPSFix;
} T_UBX_SOL;

typedef struct
{
	uint32_t iTow;
	uint8_t gpsFix;
	uint8_t NavStatFlag;
	uint8_t FixStat;
	uint8_t NavStatFlag2;
	uint32_t ttff;
	uint32_t msss;
} T_UBX_STATUS;

typedef struct
{
	uint32_t iTow;
	int32_t lon;
	int32_t lat;
	int32_t height; // Above Ellipsoid
	int32_t hMSL;	// MSL
	uint32_t hAcc;
	uint32_t vAcc;
} T_UBX_POSLLH;

typedef struct
{
	uint32_t iTOW;
	uint32_t tAcc;
	int32_t nano;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t valid;
} T_UBX_TIMEUTC;

typedef struct
{
	uint8_t portID;
	uint8_t reserved;
	uint16_t txReady;
	uint32_t Mode;
	uint32_t baudrate;
	uint16_t inProtomask;
	uint16_t outProtomask;
	uint16_t flags;
	uint16_t reserved2;
} T_CFG_PRT;

typedef struct
{
	uint8_t msgClass;
	uint8_t msgID;
	uint8_t rate;
} T_CFG_MSG;

typedef struct
{
	uint16_t measRate;
	uint16_t navRate;
	uint16_t timeRef;
} T_CFG_RATE;

typedef enum
{
	gps_state_idle = 0,
	gps_state_init = 1,
	gps_state_device_found = 2,
	gps_state_running = 3,
	gps_state_lost = 4
} gps_state_enum;

#pragma pack(pop)

#endif /* GPS_H_ */
