/*
 * exti.c
 *
 *  Created on: 12.03.2018
 *      Author: Admin
 */


// ***** Includes *****
#include "exti.h"
#include "oVario_Framework.h"
#include "sound.h"
#include "Variables.h"

// ***** Functions *****
volatile uint8_t testvar = 0;


void exti_init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIOC's AHB interface clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	/* Enable SYSCFG's APB interface clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Configure PA0 pin in input mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Connect EXTI Line to pins */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource0);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource1);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource2);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource3);

	/* Configure EXTI line0 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1 | EXTI_Line2 | EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// Config EXTI0 NVIC
	NVIC_InitTypeDef   NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel 						= EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority 	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority 			= 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd 					= ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Config EXTI1 NVIC
	NVIC_InitStructure.NVIC_IRQChannel 						= EXTI1_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	// Config EXTI2 NVIC
	NVIC_InitStructure.NVIC_IRQChannel 						= EXTI2_IRQn;
	NVIC_Init(&NVIC_InitStructure);

	// Config EXTI3 NVIC
	NVIC_InitStructure.NVIC_IRQChannel 						= EXTI3_IRQn;
	NVIC_Init(&NVIC_InitStructure);


	ipc_register_queue(80,did_KEYPAD);
}

void EXTI0_IRQHandler (void)
{
	T_keypad temp;
	temp.pad = 0;
	temp.timestamp = TIM5->CNT;
	ipc_queue_push(&temp, 5, did_KEYPAD);
	EXTI_ClearITPendingBit(EXTI_Line0);
}

void EXTI1_IRQHandler (void)
{
	T_keypad temp;
	temp.pad = 1;
	temp.timestamp = TIM5->CNT;
	ipc_queue_push(&temp, 5, did_KEYPAD);
	EXTI_ClearITPendingBit(EXTI_Line1);
}

void EXTI2_IRQHandler (void)
{
	T_keypad temp;
	temp.pad = 2;
	temp.timestamp = TIM5->CNT;
	ipc_queue_push(&temp, 5, did_KEYPAD);
	EXTI_ClearITPendingBit(EXTI_Line2);
}

void EXTI3_IRQHandler (void)
{
	T_keypad temp;
	temp.pad = 3;
	temp.timestamp = TIM5->CNT;
	ipc_queue_push(&temp, 5, did_KEYPAD);
	EXTI_ClearITPendingBit(EXTI_Line3);
}
