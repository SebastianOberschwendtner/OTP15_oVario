/*
 * igc.h
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
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
#define IGC_ERROR			    255

//Commands of the igc task
#define IGC_CMD_CREATE_LOG      1
#define IGC_CMD_CREATE_HEADER   2
#define IGC_CMD_WRITE_LOG       3
#define IGC_CMD_FINISH_LOG      4

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
void                    igc_task                    (void);
void                    igc_check_commands           (void);
void                    igc_idle                    (void);
unsigned char           igc_IsValidCharacter        (unsigned char character);
unsigned char           igc_IncludeInGrecord        (char* in);
void                    igc_register_ipc            (void);
void                    igc_call_task               (unsigned char cmd, unsigned long data, unsigned char did_target);
// inline unsigned long    igc_get_call_return         (void);
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
