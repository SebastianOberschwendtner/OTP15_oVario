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
 * @file    igc.h
 * @author  SO
 * @version V2.0
 * @date    19-September-2020
 * @brief   The task which handles the IGC logging.
 ******************************************************************************
 */

#ifndef IGC_H_
#define IGC_H_
//*********** Includes **************
#include "oVario_Framework.h"

//*********** Defines **************
#define IGC_MANUF_ID		"XCS"		//The igc code has to mimic the XCSOAR header!
#define IGC_LOGGER_ID		"AAA"		//The unique id of the logger for the A-record
#define IGC_PILOT_NAME		"Sebastian Oberschwendtner"
#define IGC_GLIDER_TYPE		"Paraglider"
#define IGC_GLIDER_ID		"none"
#define IGC_FIRMWARE_VER	"V1.0"
#define IGC_HARDWARE_VER	"REVA"
#define IGC_FR_TYPE			"XCSOAR,XCSOAR Android 6.5 Mar  8 2013"	//Important to mimic XCSOAR header!
#define IGC_GPS_RX			"uBlox, NEO-6M,16,max9000m"				//TODO Add correct GPS information (MaxAlt + Channels)
#define IGC_BARO_MANUF		"MS5611,max10000m"						//TODO Add correct Baro information (MaxAlt)

//G-Record
#define IGC_HASH_NUMBER			4
#define IGC_DIGEST_CHARPERLINE	16

//States for igc file
#define IGC_LOG_CLOSED          1
#define IGC_LOG_OPEN            2
#define IGC_LOGGING             3
#define IGC_LOG_BUSY            4
#define IGC_LOG_FINISHED        5
#define IGC_ERROR			    255

//Commands of the igc task
#define IGC_CMD_CREATE_LOG      1   // call-by-reference
#define IGC_CMD_CREATE_HEADER   2   // call-by-reference
#define IGC_CMD_WRITE_LOG       3   // call-by-reference
#define IGC_CMD_FINISH_LOG      4   // call-by-reference

//Sequences for igc commands
//Command CREATE_LOG
#define IGC_SEQUENCE_CREATE_FILENAME    1
//Command CREATE_HEADER
#define IGC_SEQUENCE_LINE_1             2
#define IGC_SEQUENCE_LINE_2             3
#define IGC_SEQUENCE_LINE_3             4
#define IGC_SEQUENCE_LINE_4             5
#define IGC_SEQUENCE_LINE_5             6
#define IGC_SEQUENCE_LINE_6             7
#define IGC_SEQUENCE_LINE_7             8
#define IGC_SEQUENCE_LINE_8             9
#define IGC_SEQUENCE_LINE_9             10
#define IGC_SEQUENCE_LINE_10            11
#define IGC_SEQUENCE_LINE_11            12
#define IGC_SEQUENCE_LINE_12            13
#define IGC_SEQUENCE_LINE_13            14
//Command CLOSE
#define IGC_SEQUENCE_GET_DIGEST         15
#define IGC_SEQUENCE_WRITE_DIGEST       16
#define IGC_SEQUENCE_WRITE_FILE         17

//*********** Functions **************
void                    igc_register_ipc            (void);
void                    igc_get_ipc                 (void);
void                    igc_task                    (void);
void                    igc_check_commands          (void);
void                    igc_idle                    (void);
void                    igc_write_log               (void);
unsigned char           igc_IsValidCharacter        (unsigned char character);
unsigned char           igc_IncludeInGrecord        (char* in);
void                    igc_call_task               (unsigned char cmd, unsigned long data, unsigned char did_target);
void                    igc_create_log              (void);
void                    igc_create_header           (void);
void                    igc_CommitCharacter         (unsigned char character);
void                    igc_CommitLine              (char* line);
void                    igc_WriteLine               (void);
void                    igc_NewRecord               (unsigned char type);
void                    igc_AppendString            (char* string);
void                    igc_AppendNumber            (unsigned long number, unsigned char digits);
void                    igc_FRecord                 (void);
void                    igc_BRecord                 (void);
void                    igc_close                   (void);
unsigned char           igc_get_state               (void);

#endif /* IGC_H_ */
