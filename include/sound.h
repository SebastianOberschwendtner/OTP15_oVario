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
 * @file    sound.c
 * @author  JK
 * @version V1.1
 * @date    12-March-2018
 * @brief   Produces the sound of the vario.
 ******************************************************************************
 */

#ifndef SOUND_H_
#define SOUND_H_

//*********** Includes **************
#include "oVario_Framework.h"

//*********** Defines **************
#define sound_mode_mute         0
#define sound_mode_cont			1
#define sound_mode_beep			2
#define sound_mode_beep_short	3

#define sound_mute	1
#define sound_beep 	2
#define sound_cont	3


//*********** Functions **************
void sound_register_ipc (void);
void sound_init         ();
void sound_task         ();

#endif /* SOUND_H_ */
