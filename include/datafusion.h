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
 * @file    datafusion.h
 * @author  JK
 * @version V1.1
 * @date    21-March-2018
 * @brief   Handles the datafusion between the GPS the barometer and the vario
 * 			task.
 ******************************************************************************
 */

#ifndef DATAFUSION_H_
#define DATAFUSION_H_

#include "oVario_Framework.h"

// ***** Defines *****
#define climbavtime 15

// ***** Functions *****
void    datafusion_register_ipc    (void);
void    datafusion_get_ipc         (void);
void    datafusion_init            (void);
void    datafusion_task            (void);


void    wind_grad                  (void);
void    wind_collect_samples       (void);
int8_t  wind_turn_dir            (float hdg_last, float hdg);

float   df_sqr                    (float x);
float   df_abs                    (float x);
float   df_min                    (float x, float y);



#endif /* DATAFUSION_H_ */
