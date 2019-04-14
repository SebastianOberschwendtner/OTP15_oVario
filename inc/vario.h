/*
 * vario.h
 *
 *  Created on: 06.04.2018
 *      Author: Admin
 */

#ifndef VARIO_H_
#define VARIO_H_
//Git ist scheiße!
//*********** Includes **************
#include "stm32f4xx.h"
#include "ipc.h"
#include "did.h"

//*********** Defines **************
#define pi 			3.1415926
#define r_earth		6378137

//*********** Functions **************
void vario_init (void);
void vario_task (void);

float vario_wgs84_distance(float lat1, float lon1, float lat2, float lon2);


#endif /* VARIO_H_ */
