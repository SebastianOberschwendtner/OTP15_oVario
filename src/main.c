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


unsigned long l_count_tick = 0;


int main(void)
{
	init_clock();
	init_systick_ms(50);
	init_led();
	spi_init();

	set_led_red(ON);
	for(l_count_tick = 0;l_count_tick<=400000;l_count_tick++);
	//spi_send_char(0xE2);		//Reset
	for(l_count_tick = 0;l_count_tick<=40000;l_count_tick++);
	spi_send_char(0xF1);		//Set com end
	spi_send_char(0x7F);
	spi_send_char(0x25);		//Set Temp comp
	spi_send_char(0xC0);		//LCD Mapping
	spi_send_char(0x02);
	spi_send_char(0x81);		//Set Contrast
	spi_send_char(0x8F);
	spi_send_char(0xA9);		//Display enable
	spi_send_char(0xA5);		//All pixel on



	set_led_green(ON);

	while(1)
	{

		if(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
		{
			set_led_green(ON);
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
