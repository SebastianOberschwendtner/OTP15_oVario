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
#include "exti.h"
#include "ipc.h"
#include "timer.h"

uint32_t error_var = 0;
unsigned long l_count_tick = 0;

void* 	ptest[20];

uint8_t test;
uint8_t test2 = 0;

#pragma pack(push, 1)
typedef struct
{
	uint8_t	 pad;
	uint32_t timestamp;
}T_keypad_ipc;
#pragma pack(pop)

T_keypad_ipc keypad_ipc[10];
T_keypad_ipc* p_keypad_ipc;



int main(void)
{
	init_clock();
	init_systick_ms(SYSTICK);
	init_led();
	exti_init();
	timer_init();

	set_led_red(ON);
	init_lcd();

	lcd_set_cd(DATA);
	for(l_count_tick = 0;l_count_tick < 64;l_count_tick++)
		spi_send_char(255);
	for(l_count_tick = 0;l_count_tick < 16;l_count_tick++)
		spi_send_char(0);
	for(l_count_tick = 0;l_count_tick < 64;l_count_tick++)
		spi_send_char(255);
	for(l_count_tick = 0;l_count_tick < 1600;l_count_tick++)
		spi_send_char(128);

	set_led_red(OFF);
	set_led_green(ON);

	l_count_tick = 0;


	while(1)
	{
		if(ipc_get_queue_bytes(did_KEYPAD) > 4)
		{
			p_keypad_ipc = ipc_queue_get(did_KEYPAD, 5);

			keypad_ipc[test2].pad 		= p_keypad_ipc->pad;
			keypad_ipc[test2].timestamp = p_keypad_ipc->timestamp;

			test2 += 1;
			if(test2 > 10)
				test2 = 0;

			switch(p_keypad_ipc->pad)
			{
			case 0:
				set_led_green(ON);
				break;
			case 1:
				set_led_green(OFF);
				break;
			case 2:
				set_led_red(ON);
				break;
			case 3:
				set_led_red(OFF);
				break;
			default:
				break;
			}
		}

		if(TICK_PASSED)
		{
			//	set_led_green(TOGGLE);
			l_count_tick++;
			if(l_count_tick == 5)
			{
				//set_led_red(OFF);
			}
			else if (l_count_tick == 10)
			{
				//set_led_red(ON);
				l_count_tick = 0;
			}
		}
	}
	return 0;
}
