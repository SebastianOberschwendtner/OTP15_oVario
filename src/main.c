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
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_flash.h"


uint32_t error_var = 0;
unsigned long l_count_tick = 0;
uint32_t reset_reason = 0;

RCC_ClocksTypeDef RCC_Clocks;
uint8_t retVal = 0;

int main(void)
{


	init_clock();
	init_systick_ms(SYSTICK);
	init_led();
	RCC_GetClocksFreq(&RCC_Clocks);

	//FLASH_OP_BOR
	//FLASH_OB_BORConfig


	//while(!(FLASH_OB_GetBOR() == OB_BOR_LEVEL3));

	FLASH_OB_Unlock();
	FLASH_OB_BORConfig(OB_BOR_LEVEL2);
	FLASH_OB_Launch();
	FLASH_OB_Lock();



	retVal = FLASH_OB_GetBOR();


	reset_reason = RCC->CSR;

	RCC->CSR |= RCC_CSR_RMVF;

	//reset_reason = RCC->CSR;

	set_led_red(ON);
	init_lcd();
	gui_bootlogo();
	exti_init();
	sound_init();
	timer_init();
	init_i2c();

	MS5611_init();
	init_sdio();

	wait_systick(10);
	init_BMS();

	gps_init();
	datafusion_init();
	vario_init();
	gui_init();

	//init_igc();


	while(1)
	{
		if(TICK_PASSED)
		{
			i2c_reset_error();
			sound_task();
			ms5611_task();
			datafusion_task();
			vario_task();
			gui_task();
			gps_task();

			BMS_set_charge_current(1000);
			BMS_task();

			l_count_tick++;
			if(l_count_tick == 5)
			{
				set_led_red(OFF);
			}
			else if (l_count_tick == 10)
			{
				set_led_red(ON);
				//igc_task();
				l_count_tick = 0;
			}
		}
	}
	return 0;
}

