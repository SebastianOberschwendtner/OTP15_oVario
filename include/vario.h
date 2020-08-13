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
void vario_init (void);
void vario_task (void);

float vario_wgs84_distance(float lat1, float lon1, float lat2, float lon2);


#endif /* VARIO_H_ */

