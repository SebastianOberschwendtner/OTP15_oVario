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

uint32_t error_var = 0;
unsigned long l_count_tick = 0;

int main(void)
{
	init_clock();
	init_systick_ms(SYSTICK);
	init_led();


	set_led_red(ON);
	init_lcd();

	lcd_clear_buffer();

	lcd_set_cursor(239,0);
	lcd_char2buffer(0xFF);
	lcd_pixel2buffer(239,127,1);
	lcd_send_buffer();

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
