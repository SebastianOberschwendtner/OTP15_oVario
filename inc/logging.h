/*
 * logging.h
 *
 *  Created on: 01.09.2018
 *      Author: Sebastian
 */

#ifndef LOGGING_H_
#define LOGGING_H_

//*********** Includes **************
#include "stm32f4xx.h"
#include "oVario_Framework.h"
#include "did.h"
#include "ipc.h"
#include "error.h"
#include "Variables.h"

//*********** Defines **************
//filename defines
#define LOG_NAME				"LOG     "
#define LOG_EXTENSION			"JOV"
#define LOG_NUMBER_START		3	//Must fit the namestring with max. 8 characters: LOG_NUMBER_START = Length(LOG_NAME)
#define LOG_NUMBER_LENGTH		5	//Must fit the namestring with max. 8 characters: LOG_NUMBER_LENGTH = 8 - Length(LOG_NAME)

//Sensors
#define LOG_MAX_SENSORS			9	//Value is not changed in Variables.h => change manually
#define LOG_SENS_NAME_LENGTH	3   //Value is not changed in Variables.h => change manually

//*********** Functions **************
void log_include(void* pointer, unsigned char length, unsigned char intervall, char* sensor_name);
void log_create(void);
void log_exe(void);
void log_write_char(unsigned char ch_byte);
void log_finish(void);

#endif /* LOGGING_H_ */
