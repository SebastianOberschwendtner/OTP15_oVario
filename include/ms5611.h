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
 * @file    ms5611.h
 * @author  JK
 * @version V2.0
 * @date    19-September-2020
 * @brief   Talks to the barometer IC MS5611 via I2C. Uses the IPC command
 * 			communication and the arbiter.
 ******************************************************************************
 */

#ifndef MS5611_H_
#define MS5611_H_

#include "oVario_Framework.h"

//***** Defines *****
//Commands for arbiter
#define BARO_CMD_INIT               1   // call-by-reference
#define BARO_CMD_GET_TEMPERATURE    2   // call-by-reference
#define BARO_CMD_GET_PRESSURE       3   // call-by-reference

//Sequences for commands
// INIT
#define BARO_SEQUENCE_GET_C0        1
#define BARO_SEQUENCE_GET_C1        2
#define BARO_SEQUENCE_GET_C2        3
#define BARO_SEQUENCE_GET_C3        4
#define BARO_SEQUENCE_GET_C4        5
#define BARO_SEQUENCE_GET_C5        6
#define BARO_SEQUENCE_GET_C6        7
#define BARO_SEQUENCE_GET_CRC_MS    8
// GET_TEMPERATURE
#define BARO_SEQUENCE_READ_TEMP     9
// GET_PRESSURE
#define BARO_SEQUENCE_READ_PRES     10


// **** Functions ****
void    ms5611_register_ipc     (void);
void    ms5611_task             (void);
void    ms5611_check_commands   (void);
void    ms5611_idle             (void);
void    ms5611_init             (void);
void    ms5611_get_temperature  (void);
void    ms5611_get_pressure     (void);
void    ms5611_call_task        (unsigned char cmd, unsigned long data, unsigned char did_target);


//A function to read and calculate the temperature
// void catch_temp_MS();

// int32_t get_temp_MS();

//A function to read and calculate the pressure
// int32_t get_pressure_MS();

// void ms5611_task();

#endif /* MS5611_H_ */
