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
//#include "sound.h"
//#include "exti.h"
//#include "timer.h"
#include "i2c.h"
//#include "ms5611.h"
//#include "datafusion.h"
#include "Variables.h"
#include "BMS.h"
#include "sdio.h"

uint32_t error_var = 0;
unsigned long l_count_tick = 0;
unsigned int i_temp = 0;

BMS_T* tBMS;

int main(void)
{
	init_clock();
	init_systick_ms(SYSTICK);
	init_led();

	init_i2c();

	wait_systick(30);

	init_lcd();
	init_BMS();

	//init_sdio();
	//exti_init();
	//sound_init();
	//timer_init();
	//MS5611_init();

	BMS_adc_start();

	wait_systick(1);


	tBMS = ipc_memory_get(did_BMS);
	BMS_get_status();
	//BMS_set_otg(ON);

	while(1)
	{
		if(TICK_PASSED)
		{

			lcd_set_cursor(0,16);
			lcd_string2buffer("Bat Voltage: ");
			lcd_num2buffer(tBMS->battery_voltage,5);
			lcd_string2buffer("mV");

			lcd_set_cursor(0,32);
			lcd_string2buffer("USB Voltage: ");
			lcd_num2buffer(tBMS->input_voltage,5);
			lcd_string2buffer("mV");

			lcd_set_cursor(0,48);
			lcd_string2buffer("Charge:      ");
			lcd_num2buffer(tBMS->charge_current,5);
			lcd_string2buffer("mA");

			lcd_set_cursor(0,64);
			lcd_string2buffer("Current:    ");
			lcd_signed_num2buffer(tBMS->current,5);
			lcd_string2buffer("mA");

			lcd_set_cursor(0,80);
			lcd_string2buffer("USB Current: ");
			lcd_num2buffer(tBMS->input_current,5);
			lcd_string2buffer("mA");

			lcd_set_cursor(0,96);
			lcd_string2buffer("Discharged: ");
			lcd_signed_num2buffer(tBMS->discharged_capacity,4);
			lcd_string2buffer("mAh");

			lcd_set_cursor(0,112);
			lcd_string2buffer("State: ");
			if(tBMS->charging_state & STATUS_FAST_CHARGE)
				lcd_string2buffer("Charging");
			else
				lcd_string2buffer("Charged ");

			lcd_send_buffer();
			BMS_get_adc();
			BMS_gauge_get_adc();
			BMS_charge_start();
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
