/*
 * ms5611.h
 *
 *  Created on: 16.03.2018
 *      Author: Admin
 */

#ifndef MS5611_H_
#define MS5611_H_

#include "oVario_Framework.h"


// **** Functions ****
void MS5611_init();

//A function to read and calculate the temperature
void catch_temp_MS();

int32_t get_temp_MS();

//A function to read and calculate the pressure
int32_t get_pressure_MS();
void ms5611_task();

#endif /* MS5611_H_ */
