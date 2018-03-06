/*
 * Variables.h
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#ifndef VARIABLES_H_
#define VARIABLES_H_
/*
 * Timing Variables
 */
volatile unsigned long flag_tick = 0;  	//flag for ovf of systick timer
#ifdef COUNT_TICK
volatile unsigned long l_count_tick = 0; //Count ovf of systick timer
#endif

#endif /* VARIABLES_H_ */
