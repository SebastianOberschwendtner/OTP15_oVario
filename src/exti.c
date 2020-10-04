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
 * @file    exti.c
 * @author  JK
 * @version V1.0
 * @date    12-March-2018
 * @brief   Handles the interrupts to capture the button inputs.
 ******************************************************************************
 */

// ***** Includes *****
#include "exti.h"

// ***** Variables *****
volatile uint8_t testvar = 0;
T_command ExtiCmd;

// ***** Functions *****
/**
 * @brief Initialize the interrupts
 */
void exti_init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIOC's AHB interface clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	/* Enable SYSCFG's APB interface clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Configure PA0 pin in input mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Connect EXTI Line to pins */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource0);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource1);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource2);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource3);

	/* Configure EXTI line0 */
	EXTI_InitStructure.EXTI_Line 	= EXTI_Line0 | EXTI_Line1 | EXTI_Line2 | EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode 	= EXTI_Mode_Interrupt;
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
};

/**
 * @brief Interrupthandler for EXTI0
 * @details interrupt handler
 */
void EXTI0_IRQHandler (void)
{
	T_command temp;
	temp.did  		= did_KEYPAD;
	temp.cmd		= cmd_gui_eval_keypad;
	temp.data 		= data_KEYPAD_pad_LEFT;
	temp.timestamp 	= TIM5->CNT;
	ipc_queue_push(&temp, 10, did_GUI);
	EXTI_ClearITPendingBit(EXTI_Line0);


//	//Send infobox
//	ExtiCmd.did 		= did_KEYPAD;
//	ExtiCmd.cmd			= cmd_gui_set_std_message;
//	ExtiCmd.data 		= data_info_keypad_0;
//	ExtiCmd.timestamp 	= TIM5->CNT;
//	ipc_queue_push(&ExtiCmd, 10, did_GUI);
};

/**
 * @brief Interrupthandler for EXTI1
 * @details interrupt handler
 */
void EXTI1_IRQHandler (void)
{
	T_command temp;
	temp.did  		= did_KEYPAD;
	temp.cmd		= cmd_gui_eval_keypad;
	temp.data 		= data_KEYPAD_pad_DOWN;
	temp.timestamp 	= TIM5->CNT;
	ipc_queue_push(&temp, 10, did_GUI);
	EXTI_ClearITPendingBit(EXTI_Line1);

//	//Send infobox
//	ExtiCmd.did 		= did_KEYPAD;
//	ExtiCmd.cmd			= cmd_gui_set_std_message;
//	ExtiCmd.data 		= data_info_keypad_1;
//	ExtiCmd.timestamp 	= TIM5->CNT;
//	ipc_queue_push(&ExtiCmd, 10, did_GUI);
};

/**
 * @brief Interrupthandler for EXTI2
 * @details interrupt handler
 */
void EXTI2_IRQHandler (void)
{
	T_command temp;
	temp.did  		= did_KEYPAD;
	temp.cmd		= cmd_gui_eval_keypad;
	temp.data 		= data_KEYPAD_pad_UP;
	temp.timestamp 	= TIM5->CNT;
	ipc_queue_push(&temp, 10, did_GUI);
	EXTI_ClearITPendingBit(EXTI_Line2);

//	//Send infobox
//	ExtiCmd.did 		= did_KEYPAD;
//	ExtiCmd.cmd			= cmd_gui_set_std_message;
//	ExtiCmd.data 		= data_info_keypad_2;
//	ExtiCmd.timestamp 	= TIM5->CNT;
//	ipc_queue_push(&ExtiCmd, 10, did_GUI);
};

/**
 * @brief Interrupthandler for EXTI3
 * @details interrupt handler
 */
void EXTI3_IRQHandler (void)
{
	T_command temp;
	temp.did  		= did_KEYPAD;
	temp.cmd		= cmd_gui_eval_keypad;
	temp.data 		= data_KEYPAD_pad_RIGHT;
	temp.timestamp 	= TIM5->CNT;
	ipc_queue_push(&temp, 10, did_GUI);
	EXTI_ClearITPendingBit(EXTI_Line3);

//	//Send infobox
//	ExtiCmd.did 		= did_KEYPAD;
//	ExtiCmd.cmd			= cmd_gui_set_std_message;
//	ExtiCmd.data 		= data_info_keypad_3;
//	ExtiCmd.timestamp 	= TIM5->CNT;
//	ipc_queue_push(&ExtiCmd, 10, did_GUI);
};
