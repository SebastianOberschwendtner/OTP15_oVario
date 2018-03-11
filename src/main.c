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


unsigned long l_count_tick = 0;


int main(void)
{
	init_clock();
	init_systick_ms(SYSTICK);
	init_led();


	set_led_red(ON);
	init_lcd();

	lcd_set_cd(DATA);
	for(l_count_tick = 0;l_count_tick < 64;l_count_tick++)
		spi_send_char(0);
	for(l_count_tick = 0;l_count_tick < 64;l_count_tick++)
		spi_send_char(255);
	for(l_count_tick = 0;l_count_tick < 64;l_count_tick++)
		spi_send_char(128);
	set_led_red(OFF);
	set_led_green(ON);

	l_count_tick = 0;
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
