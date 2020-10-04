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
 * @file    vario.h
 * @author  JK
 * @version V1.1
 * @date    06-April-2018
 * @brief   Tells you whether you are climbing or sinking.
 ******************************************************************************
 */

#ifndef VARIO_H_
#define VARIO_H_

//*********** Includes **************
#include "oVario_Framework.h"

//*********** Defines **************
#define pi 			3.1415926
#define r_earth		6378137

#define frequ_base_climb	300
#define frequ_base_0climb	300
#define frequ_base_sink		200

#define frequ_rate_climb	200
#define frequ_rate_0climb	200
#define frequ_rate_sink		50

#define frequ_srate_base_climb	1
#define frequ_srate_base_0climb	1
#define frequ_srate_base_sink	4

//*********** Functions **************
void    vario_register_ipc      (void);
void    vario_get_ipc           (void);
void    vario_task              (void);

float   vario_wgs84_distance    (float lat1, float lon1, float lat2, float lon2);


#endif /* VARIO_H_ */

