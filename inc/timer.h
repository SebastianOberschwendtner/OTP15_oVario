/*
 * timer.h
 *
 *  Created on: 16.03.2018
 *      Author: Admin
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "stm32f4xx.h"
#include "stm32f4xx_tim.h"

void timer_init();
void tic();
uint32_t toc();

#endif /* TIMER_H_ */
