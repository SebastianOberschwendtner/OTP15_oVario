/*
 * sound.c
 *
 *  Created on: 12.03.2018
 *      Author: Admin
 */
// ***** Includes *****
#include "sound.h"
#include "Variables.h"


// ***** Variables *****

T_sound_state sound_state;
uint32_t timet = 0;
T_command sound_command;
volatile uint32_t temp = 0;

// ***** Functions *****

void sound_init()
{
	// Init Portx peripheral Clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// GPIO Init
	// Buzzer Ausgang: PA5

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd	= GPIO_PuPd_DOWN;
	GPIO_InitStruct.GPIO_Speed	= GPIO_Fast_Speed;

	GPIO_Init(GPIOA, &GPIO_InitStruct);


	// Alternative-Funktion mit dem IO-Pin verbinden
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_TIM2);


	// Timer Init
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  		 TIM_OCInitStructure;

	// Clock enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	//RCC_ClocksTypeDef RCC_Clocks;

	//RCC_GetClocksFreq(&RCC_Clocks);

	// Timer for Buzzer Output PWM
	TIM_TimeBaseStructure.TIM_Period 			= 10500;//20995;
	TIM_TimeBaseStructure.TIM_Prescaler 		= 7;
	TIM_TimeBaseStructure.TIM_ClockDivision 	= 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseStructure.TIM_CounterMode 		= TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM2, ENABLE);

	// OC Init
	TIM_OCInitStructure.TIM_OCMode 			= TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState 	= TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState 	= TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity 		= TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OCNPolarity 	= TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState 	= TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState 	= TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_Pulse 			= 0;

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_Cmd(TIM2, ENABLE);
	TIM_CtrlPWMOutputs(TIM2, ENABLE);
	TIM_SetCompare1(TIM2, 0);

	// Timer for Beep Init
	// Clock enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseStructure.TIM_Period 			= 20000;	// 20000 = 1Hz
	TIM_TimeBaseStructure.TIM_Prescaler 		= 2099;
	TIM_TimeBaseStructure.TIM_ClockDivision 	= 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseStructure.TIM_CounterMode 		= TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_Cmd(TIM3, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	// NVIC Init
	NVIC_InitTypeDef   NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel 						= TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority 	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority 			= 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd 					= ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Predefine state
	sound_state.volume 		= 100;
	sound_state.mute   		= 0;
	sound_state.frequency 	= 1000;
	sound_state.mode 		= sound_mode_beep;
	sound_state.period 		= 200;

	ipc_register_queue(50, did_SOUND);
}


// Set frequency and volume
// Frequency [Hz]
// Volume [%]; 0-100%
void sound_set_frequ_vol(uint16_t frequency, uint8_t volume, uint8_t period)
{
	// 105 = 0.01ms; 	1000Hz
	uint32_t reload = 100000/((uint32_t)frequency) * 105;//10500;
	uint32_t compare = (reload / 2) * volume / 100;

	TIM_SetAutoreload(TIM2, reload);
	TIM_SetCompare1(TIM2, compare);

	reload = period * 200;			// 20000 = 1Hz
	TIM_SetAutoreload(TIM3, reload);

	if(TIM3->CNT > reload)
		TIM3->CNT = 0;
}

void sound_task(void)
{
	while(ipc_get_queue_bytes(did_SOUND) > 9)
	{
		if(ipc_queue_get(did_SOUND, 10, &sound_command))
		{
			switch(sound_command.cmd)
			{
			case sound_cmd_set_frequ:
				sound_state.frequency = (uint16_t)sound_command.data;
				break;

			case sound_cmd_set_vol:
				sound_state.volume = (uint16_t)sound_command.data;
				break;

			case sound_cmd_set_louder:
				if(sound_state.volume >= 100-(uint8_t)sound_command.data)
					sound_state.volume += (uint8_t)sound_command.data;
				else
					sound_state.volume = 100;
				break;

			case sound_cmd_set_quieter:
				if(sound_state.volume >= (uint8_t)sound_command.data)
					sound_state.volume -= (uint8_t)sound_command.data;
				else
					sound_state.volume = 0;
				break;

			case sound_cmd_set_mute:
				sound_state.mute = 1;
				break;

			case sound_cmd_set_unmute:
				sound_state.mute = 0;
				break;

			case sound_cmd_set_beep:
				sound_state.mode = sound_mode_beep;
				break;

			case sound_cmd_set_cont:
				sound_state.mode = sound_mode_cont;
				break;

			case sound_cmd_set_period:
				sound_state.period = (uint8_t)sound_command.data;
				break;

			default:
				break;
			}
		}
	}
	if(sound_state.mode != sound_mode_beep)
		sound_state.beep = 1;


	sound_set_frequ_vol(sound_state.frequency, sound_state.volume * (sound_state.mute^1) * sound_state.beep, sound_state.period);
}

void TIM3_IRQHandler (void)
{
	if(sound_state.mode == sound_mode_beep)
		sound_state.beep ^= 1;

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
}

