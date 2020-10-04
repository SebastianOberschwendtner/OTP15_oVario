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
 * @file    did.h
 * @author  JK
 * @version V1.1
 * @date    10-March-2018
 * @brief   Contains the dID defines for the IPC communication.
 ******************************************************************************
 */

#ifndef DID_H_
#define DID_H_


// ***** DeviceIDs for IPC *****
#define 	did_MS5611		0
#define 	did_LCD			1
#define 	did_KEYPAD		2
#define 	did_SOUND		3
#define 	did_DATAFUSION  4
#define 	did_BMS			5
#define		did_GPS			6
#define 	did_SDIO		7
#define		did_SYS			8
#define 	did_DIRFILE		9
#define		did_GUI			10
#define		did_GPS_DMA		11
#define		did_FILE1		12
#define		did_FILE2		13
#define 	did_VARIO		14
#define		did_IGC			15
#define     did_FILEHANDLER 16
#define     did_MD5         17
#define     did_I2C         18
#define     did_COUL        19

#endif /* DID_H_ */
