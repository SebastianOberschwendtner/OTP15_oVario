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

uint32_t error_var = 0;
unsigned long l_count_tick = 0;

int32_t testmain = 0;



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


	wait_systick(10);
	init_BMS();

	gps_init();

	datafusion_init();
	vario_init();
	gui_init();


	while(1)
	{
		if(TICK_PASSED)
		{
			sound_task();
			ms5611_task();
			datafusion_task();
			vario_task();
			gui_task();
			gps_task();


			BMS_get_adc();
			BMS_gauge_get_adc();



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
