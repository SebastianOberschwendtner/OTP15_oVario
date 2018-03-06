/**
 ******************************************************************************
 * @file    main.c
 * @author  SO
 * @version V1.0
 * @date    25-Februar-2018
 * @brief   Default main function.
 ******************************************************************************
 */


#include "Variables.h"
#include "stm32f4xx_it.h"
#include "oVario_Framework.h"


extern volatile unsigned long flag_tick;
#ifdef COUNT_TICK
extern volatile unsigned long l_count_tick;
#endif

int main(void)
{
	init_clock();
	init_systick_ms(50);
	init_led();

	set_led_green(OFF);
	set_led_red(OFF);

	while(1)
	{
		if(flag_tick)
		{
			flag_tick = 0;
			if(l_count_tick == 5)
			{
				set_led_green(ON);
				set_led_red(OFF);
			}
			else if (l_count_tick == 10)
			{
				set_led_green(OFF);
				set_led_red(ON);
				l_count_tick = 0;
			}
		}
	}
	return 0;
}
