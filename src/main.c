/**
 ******************************************************************************
 * @file    main.c
 * @author  SO
 * @version V1.0
 * @date    25-Februar-2018
 * @brief   Default main function.
 ******************************************************************************
 */

#include "oVario_Framework.h"
#include "DOGXL240.h"
#include "ipc.h"
#include "sound.h"
#include "exti.h"
#include "timer.h"
#include "i2c.h"
#include "ms5611.h"
#include "datafusion.h"
#include "Variables.h"

uint32_t error_var = 0;
unsigned long l_count_tick = 0;

int32_t testmain = 0;

uint8_t volu;
uint16_t freque;
uint8_t peri;
float temp_climb;

datafusion_T* p_ipc_df_data;
T_sound_command main_command;

int main(void)
{
	init_clock();
	init_systick_ms(SYSTICK);
	init_led();

	set_led_red(ON);
	init_lcd();
	exti_init();
	sound_init();
	timer_init();
	init_i2c();
	MS5611_init();
	datafusion_init();



	p_ipc_df_data = ipc_memory_get(did_DATAFUSION);


	while(1)
	{
		if(TICK_PASSED)
		{
			sound_task();
			ms5611_task();
			datafusion_task();




			temp_climb = p_ipc_df_data->climbrate_filt;

			if(temp_climb > 5)
				temp_climb = 5;

			if(temp_climb < -2)
				temp_climb = -2;


			/*
 Top:
 peri = 20;
 Sound = 2000;

 Bottom:
 peri = 100
 sound = 500;

			 */







			if(temp_climb > 0.3)
			{
				main_command.command 	= sound_cmd_set_unmute;
				main_command.data		= 0;
				ipc_queue_push(&main_command, 5, did_SOUND);

				freque = (uint16_t)((temp_climb)/5*1500 + 500);
				volu = 100;
				if(temp_climb < 1)
					temp_climb = 1;
				peri = (uint8_t)(100 - (temp_climb/5*80));

				// Set Frequency
				main_command.command 	= sound_cmd_set_frequ;
				main_command.data		= (uint32_t)freque;

				ipc_queue_push(&main_command, 5, did_SOUND);

				// Set Period
				main_command.command 	= sound_cmd_set_period;
				main_command.data		= (uint32_t)peri;

				ipc_queue_push(&main_command, 5, did_SOUND);
			}
			else
			{
				main_command.command 	= sound_cmd_set_mute;
				main_command.data		= 0;
				ipc_queue_push(&main_command, 5, did_SOUND);
			}

			set_led_green(TOGGLE);
			l_count_tick++;
			if(l_count_tick == 5)
			{
				set_led_red(OFF);
			}
			else if (l_count_tick == 10)
			{
				set_led_red(ON);
				l_count_tick = 0;
			}
		}
	}
	return 0;
}
