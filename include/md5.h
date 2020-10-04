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
 * @file    md5.h
 * @author  SO
 * @version V2.0
 * @date    19-September-2020
 * @brief   Calculates the md5-hash. Uses the IPC command interface.
 ******************************************************************************
 */

#ifndef MD5_H_
#define MD5_H_
//*********** Includes **************
#include "oVario_Framework.h"

//*********** defines for task **************
//Commands
#define MD5_CMD_APPEND_CHAR         1 // call-by-value, nargs = 2
#define MD5_CMD_APPEND_STRING       2 // call-by-value, nargs = 2
#define MD5_CMD_FINALIZE            3 // called via ipc

//Sequences for commands

//*********** defines for hash **************


//*********** Functions **************
void            md5_task            (void);
void            md5_check_commands            (void);
void            md5_register_ipc    (void);
void            md5_initialize      (MD5_T* hash, unsigned long* key);
unsigned long   md5_leftrotate      (unsigned long x, unsigned long c);
void            md5_append_char     (void);
void            md5_append_string   (void);
void            md5_process512      (MD5_T* hash);
void            md5_WriteLength     (MD5_T* hash);
void            md5_finalize        (MD5_T* hash);
void            md5_GetDigest       (MD5_T* hash, char* digest);


#endif /* MD5_H_ */
