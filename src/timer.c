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
 * @file    timer.c
 * @author  JK
 * @version v2.0.0
 * @date    16-March-2018
 * @brief   Provides timing for the application.
 ******************************************************************************
 */

//****** Includes ******
#include "timer.h"

//****** Variables ******
uint32_t timestamp;

//****** Functions ******
/**
 * @brief Initialize the timer which is used for timestamps.
 * Runs every 1 us.
 */
void timer_init(){
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	// Timer disable
	TIM_Cmd(TIM5, DISABLE);

	// Timer init
	TIM_TimeBaseStructure.TIM_Period 			= 0xFFFFFFFF;
	TIM_TimeBaseStructure.TIM_Prescaler 		= 8399;
	TIM_TimeBaseStructure.TIM_ClockDivision 	= TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode 		= TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	// Timer enable
	TIM_Cmd(TIM5, ENABLE);

	// RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// /* Configure USART3_Tx and USART3_Rx as alternate function */
	// GPIO_InitTypeDef GPIO_InitStructure;
	// GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	// GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	// GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	// GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	// GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	// GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief Start time measurement
 */
void tic()
{
	timestamp = TIM5->CNT;
};

/**
 * @brief Stop time measurement
 * @return Result of measurement
 */
uint32_t toc()
{
	uint32_t time = TIM5->CNT - timestamp;
	return time;
};

