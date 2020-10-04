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
 * @file    error.h
 * @author  JK
 * @version V1.0
 * @date    10-March-2018
 * @brief   The global error variable and defines.
 ******************************************************************************
 */

#ifndef ERROR_H_
#define ERROR_H_


//*********** Errors **************
#define err_no_memory_left  (1<<0)
#define err_queue_overrun	(1<<1)
#define err_bms_fault		(1<<2)
#define err_coloumb_fault	(1<<3)
#define err_baro_fault		(1<<4)
#define err_sd_fault		(1<<5)

//Bit defines
#define SENSOR_ERROR_BMS		(1<<3)
#define SENSOR_ERROR_GAUGE		(1<<7)
#define SENSOR_ERROR			(1<<7)







#endif /* ERROR_H_ */
