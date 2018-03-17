/*
 * timer.c
 *
 *  Created on: 16.03.2018
 *      Author: Admin
 */

#include "timer.h"

uint32_t timestamp;

//Timer läuft in 100 us Takt
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

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Configure USART3_Tx and USART3_Rx as alternate function */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void tic(){
	timestamp = TIM5->CNT;
}

uint32_t toc(){
	uint32_t time = TIM5->CNT - timestamp;
	return time;
}

