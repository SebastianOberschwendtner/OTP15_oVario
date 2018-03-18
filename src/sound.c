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

T_keypad* sound_command;
T_sound_state sound_state;
uint32_t timet = 0;


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

	// Timer init
	TIM_TimeBaseStructure.TIM_Period 			= 10500;//20995;
	TIM_TimeBaseStructure.TIM_Prescaler 		= 7;
	TIM_TimeBaseStructure.TIM_ClockDivision 	= 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseStructure.TIM_CounterMode 		= TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM2, ENABLE);


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


	sound_state.volume 		= 100;
	sound_state.mute   		= 0;
	sound_state.frequency 	= 1000;
}


// Set frequency and volume
// Frequency [Hz]
// Volume [%]; 0-100%
void sound_set_frequ_vol(uint16_t frequency, uint8_t volume)
{
	// 105 = 0.01ms; 	1000Hz
	uint32_t reload = 100000/((uint32_t)frequency) * 105;//10500;
	uint32_t compare = (reload / 2) * 100 / volume;

	TIM_SetAutoreload(TIM2, reload);
	TIM_SetCompare1(TIM2, compare);
}



void sound_task(void)
{
	if(ipc_get_queue_bytes(did_KEYPAD) > 4)
	{
		sound_command = ipc_queue_get(did_KEYPAD,5);

		if((sound_command->timestamp - timet)>2000)
		{
			timet = sound_command->timestamp;

			switch(sound_command->pad)
			{
			case 0:// mute/ unmute
				sound_state.mute ^= 1;
				break;
			case 1:	// pitch up
				sound_state.frequency += 500;
				break;
			case 2: // pitch down
				if(sound_state.frequency > 500)
					sound_state.frequency -= 500;
				break;
			case 3:
				break;

			default:
				break;
			}
		}
	}
	sound_set_frequ_vol(sound_state.frequency, sound_state.volume * sound_state.mute);
}

