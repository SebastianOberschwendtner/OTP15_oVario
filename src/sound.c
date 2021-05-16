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
 * @file    sound.c
 * @author  JK
 * @version V1.1
 * @date    12-March-2018
 * @brief   Produces the sound of the vario.
 ******************************************************************************
 */
// ***** Includes *****
#include "sound.h"

// ***** Variables *****
T_sound_state sound_state;
uint32_t timet = 0;
T_command sound_command;
volatile uint32_t temp = 0;
uint8_t beepcount = 0;

// ***** Defines *****
#define NEWSOUND

// ***** Functions *****
//********** register and get dummies for tasks ******
/**
 * @brief Register everything relevant for IPC
 */
void sound_register_ipc(void)
{
	//Register everything relevant for IPC
	ipc_register_queue(5 * sizeof(T_command), did_SOUND);
};

/**
 * @brief Initialize the sound task
 */
void sound_init()
{
	// Init Portx peripheral Clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// GPIO Init
	// Buzzer Ausgang: PA5

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Fast_Speed;

	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Alternative-Funktion mit dem IO-Pin verbinden
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_TIM2);

	// Timer Init
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	// Clock enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	//RCC_ClocksTypeDef RCC_Clocks;

	//RCC_GetClocksFreq(&RCC_Clocks);

	// Timer for Buzzer Output PWM
	TIM_TimeBaseStructure.TIM_Period = 10500; //20995;
	TIM_TimeBaseStructure.TIM_Prescaler = 7;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM2, ENABLE);

	// OC Init
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_Pulse = 0;

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_Cmd(TIM2, ENABLE);
	TIM_CtrlPWMOutputs(TIM2, ENABLE);
	TIM_SetCompare1(TIM2, 0);

// Timer for Beep Init
// Clock enable
#ifndef NEWSOUND
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = 20000; // 20000 = 1Hz
	TIM_TimeBaseStructure.TIM_Prescaler = 2099;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_Cmd(TIM3, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	// NVIC Init
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif
	// Predefine state
	sound_state.volume = 100;
	sound_state.mute = 0;
	sound_state.frequency = 1000;
	sound_state.mode = sound_mode_beep;
	sound_state.period = 200;
};

/**
 * @brief Set frequency and volume
 * @param frequency Frequency in [Hz]
 * @param volume Volume in [%]: 0-100%
 * @param period Period of sound generation
 */
void sound_set_frequ_vol(uint16_t frequency, uint8_t volume, uint8_t period)
{
	// 105 = 0.01ms; 	1000Hz
	uint32_t reload = 100000 / ((uint32_t)frequency) * 105; //10500;
	uint32_t compare = (reload / 2) * volume / 100;

	TIM_SetAutoreload(TIM2, reload);
	TIM_SetCompare1(TIM2, compare);
#ifndef NEWSOUND
	reload = period * 200; // 20000 = 1Hz = 1s

	if (sound_state.mode == sound_mode_beep_short)
		TIM_SetAutoreload(TIM3, reload / 4);
	else
	{
		TIM_SetAutoreload(TIM3, reload);
	}

	if (TIM3->CNT > reload)
		TIM3->CNT = 0;
#endif
};

/**
 * @brief the sound task
 */
uint8_t sound_tim_cnt = 0;
uint8_t fastbeep = 1;

void sound_task(void)
{
	while (ipc_get_queue_bytes(did_SOUND) > 9)
	{
		if (ipc_queue_get(did_SOUND, 10, &sound_command))
		{
			switch (sound_command.cmd)
			{
			case cmd_sound_set_frequ:
				sound_state.frequency = (uint16_t)sound_command.data;
				break;

			case cmd_sound_set_vol:
				sound_state.volume = (uint16_t)sound_command.data;
				break;

			case cmd_sound_set_louder:
				if (sound_state.volume >= 100 - (uint8_t)sound_command.data)
					sound_state.volume += (uint8_t)sound_command.data;
				else
					sound_state.volume = 100;
				break;

			case cmd_sound_set_quieter:
				if (sound_state.volume >= (uint8_t)sound_command.data)
					sound_state.volume -= (uint8_t)sound_command.data;
				else
					sound_state.volume = 0;
				break;

			case cmd_sound_set_mute:
				sound_state.mute = 1;
				break;

			case cmd_sound_set_unmute:
				sound_state.mute = 0;
				break;

			case cmd_sound_set_beep:
				sound_state.mode = sound_mode_beep;
				break;

			case cmd_sound_set_beep_short:
				sound_state.mode = sound_mode_beep_short;
				break;

			case cmd_sound_set_cont:
				sound_state.mode = sound_mode_cont;
				break;

			case cmd_sound_set_period:
				sound_state.period = (uint8_t)sound_command.data;
				break;

			default:
				break;
			}
		}
	}
	if ((sound_state.mode != sound_mode_beep) && (sound_state.mode != sound_mode_beep_short))
		sound_state.beep = 1;

		// Sound Statemachine
#ifdef NEWSOUND
	switch (sound_state.mode)
	{
	// ***** SHUT THE FUCK UP *****
	case sound_mode_mute:
		fastbeep = 1;
		break;

		// ***** BEEP *****
	case sound_mode_beep:
		if (fastbeep) // BEEP on first loop after mute
		{
			fastbeep = 0;
			sound_state.beep = 1;
			sound_tim_cnt = 0;
		}

		sound_tim_cnt++;

		// sound_state.period reflects the period time in 1/100s
		// -> 1/200s on and 1/200s off
		// this function is executed every 1/10s
		// minimum value is 2/10s -> sound_state.period = 20
		// the divisor has to be 20 for same sound (stoned variant,Jakob: 4)
		// reduced to 10 for better audial recognition of climbrate

		if (sound_tim_cnt >= sound_state.period / 10) // toggle sound
		{
			if (sound_state.mode == sound_mode_beep)
				sound_state.beep ^= 1;
			sound_tim_cnt = 0;
		}

		break;
	// ***** SHORT BEEP *****
	case sound_mode_beep_short:
		if (fastbeep) // BEEP on first loop after mute
		{
			fastbeep = 0;
			sound_state.beep = 1;
			sound_tim_cnt = 0;
		}

		sound_tim_cnt++;

		// sound_state.period reflects the period time in 1/100s
		// -> 1/200s on and 1/200s off
		// this function is executed every 1/10s
		// minimum value is 8/10s -> sound_state.period = 80

		if (sound_tim_cnt < 7) // toggle sound
		{
			sound_state.beep = 0;
		}
		else
		{
			if (sound_state.mode == sound_mode_beep_short)
				sound_state.beep = 1;
			sound_tim_cnt = 0;
		}
		break;
		// ***** MAKE ANNOYING SOUND *****
	case sound_mode_cont:
		fastbeep = 1,
		sound_state.mute = 0;
		sound_state.beep = 1;

		break;
	default:
		fastbeep = 1;
		break;
	}
#endif
	sound_set_frequ_vol(sound_state.frequency, sound_state.volume * (sound_state.mute ^ 1) * sound_state.beep, sound_state.period);
};

/**
 * @brief The interrupt handler to generate the sound
 * @details interrupt handler
 */
void TIM3_IRQHandler(void)
{
#ifndef NEWSOUND
	if (sound_state.mode == sound_mode_beep)
	{
		sound_state.beep ^= 1;
	}
	else if (sound_state.mode == sound_mode_beep_short)
	{
		if (beepcount < 7)
		{
			sound_state.beep = 0;
		}
		else
		{
			sound_state.beep = 1;
			beepcount = 0;
		}
		beepcount++;
	}
#endif
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
};
