/*
 * stm32f4xx_it.c
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */
#include "stm32f4xx_it.h"

extern volatile unsigned long flag_tick;
#ifdef COUNT_TICK
extern volatile unsigned long l_count_tick;
#endif
/*
 * Interrupt handler for systick timer
 */
void SysTick_Handler(void)
{
	flag_tick = 1;
#ifdef COUNT_TICK
	l_count_tick++;
#endif
}
