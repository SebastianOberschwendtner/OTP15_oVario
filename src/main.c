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
#include "vario.h"
#include "gui.h"
#include "Variables.h"
#include "gps.h"
#include "BMS.h"
#include "sdio.h"
#include "logging.h"
#include "igc.h"
#include "stm32f4xx.h"
#include "adc.h"

uint32_t error_var = 0;
unsigned long l_count_tick = 0;

ADC_T* ipc_main_adc_data;

int main(void)
{

	init_clock();
	init_systick_ms(SYSTICK);
	init_led();

	set_led_red(ON);
	//init_lcd();
	//gui_bootlogo();
	//exti_init();
	sound_init();
	timer_init();
	init_i2c();
	UB_ADC1_SINGLE_Init();

	MS5611_init();
	//init_sdio();

	wait_systick(10);
	//init_BMS();

	//gps_init();
	datafusion_init();
	vario_init();
	//	gui_init();

	//init_igc();

	//	log_create();

	if(error_var == 0)
		set_led_red(OFF);
	else
		set_led_red(ON);


	ipc_main_adc_data = ipc_memory_get(did_ADC);


	while(1)
	{
		if(TICK_PASSED)
		{
			i2c_reset_error();
			sound_task();
			ms5611_task();
			datafusion_task();
			vario_task();
			adc_task();
			//gui_task();




			l_count_tick++;
			if(l_count_tick == 5)
			{
				if(ipc_main_adc_data->voltage > 4.0)
				{
					set_led_green(ON);
					set_led_red(OFF);
				}
				else if(ipc_main_adc_data->voltage > 3.5)
				{
					set_led_green(ON);
					set_led_red(ON);
				}
				else
				{
					set_led_red(ON);
					set_led_green(OFF);
				}

			}
			else if (l_count_tick == 10)
			{
				if(ipc_main_adc_data->i_charge < 0.1)
				{
					set_led_green(OFF);
					set_led_red(OFF);
				}
				else
				{
					;
				}

				l_count_tick = 0;
			}
		}
	}
	return 0;
}

