/**
 ******************************************************************************
 * @file    timer.h
 * @author  JK
 * @version V1.0
 * @date    16-March-2018
 * @brief   Provides timing for the application.
 ******************************************************************************
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "oVario_Framework.h"

void timer_init();
void tic();
uint32_t toc();

#endif /* TIMER_H_ */
