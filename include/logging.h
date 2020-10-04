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
 * @file    logging.h
 * @author  SO
 * @version V1.0
 * @date    01-September-2018
 * @brief   Logs data to the SD-card.
 ******************************************************************************
 */

#ifndef LOGGING_H_
#define LOGGING_H_

//*********** Includes **************
#include "oVario_Framework.h"


//*********** Defines **************
//filename defines
#define LOG_NAME				"LOG     "
#define LOG_EXTENSION			"TXT"
#define LOG_NUMBER_START		3	//Must fit the namestring with max. 8 characters: LOG_NUMBER_START = Length(LOG_NAME)
#define LOG_NUMBER_LENGTH		5	//Must fit the namestring with max. 8 characters: LOG_NUMBER_LENGTH = 8 - Length(LOG_NAME)

//Sensors
#define LOG_MAX_SENSORS			9	//Value is not changed in Variables.h => change manually
#define LOG_SENS_NAME_LENGTH	3   //Value is not changed in Variables.h => change manually

//*********** Functions **************
// void log_include(void* pointer, unsigned char length, unsigned char intervall, char* sensor_name);
// void log_create(void);
// void log_exe(void);
// void log_exe_txt(void);
// void log_write_char(unsigned char ch_byte);
// void log_finish(void);
// void log_float(char* add, unsigned char ch_predecimal, unsigned char ch_dedecimal);

#endif /* LOGGING_H_ */
