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
#include "spi.h"

uint8_t error = 0;
unsigned long l_count_tick = 0;


int main(void)
{
	init_clock();
	init_systick_ms(100);
	init_led();
	init_spi();

	set_led_red(ON);
	set_led_green(OFF);
	wait_ms(100);
	set_led_red(ON);
	set_led_green(OFF);

	while(1)
	{
		if(TICK_PASSED)
		{
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
