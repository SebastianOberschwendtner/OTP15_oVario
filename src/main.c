/**
 ******************************************************************************
 * @file    main.c
 * @author  SO
 * @version V1.0
 * @date    25-Februar-2018
 * @brief   Default main function.
 ******************************************************************************
 */

#include "oVario_Framework.h" //<--- define your hardware setup here

uint32_t error_var = 0;

int main(void)
{
	// Initialize core
	init_clock();
	init_systick_ms(SYSTICK);
	init_gpio();

	//Register all ipc memory
	sdio_register_ipc();

	//Get all ipc memory
	//TODO Add get function here...
	
	//intialize peripherals
	set_led_red(ON);
	init_lcd();
	gui_bootlogo();
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

	
	// init_igc();

	//initialize the scheduler
	init_scheduler();
	schedule(TASK_CORE,	1); 			//run core task every systick
	schedule(TASK_AUX,	1);				//run aux task every systick for now
	//TODO Change the rate of the aux task
	schedule(TASK_1Hz,  1000/SYSTICK);	//run every 1s
	set_led_red(OFF);

	while(1)
	{
		
		if(TICK_PASSED)
		{
			//***** Run scheduler *****
			run_scheduler();

			//***** TASK_AUX ******
			if(run(TASK_AUX,TICK_PASSED))
			{
				// set_led_red(ON);
				sdio_task();
				// set_led_red(OFF);
			}

			//***** TASK_CORE ******
			if(run(TASK_CORE,TICK_PASSED))
			{				
				ms5611_task();
				datafusion_task();
				vario_task();
				i2c_reset_error();
				system_task();
				// sound_task();
				gui_task();
				gps_task();
				BMS_task();
			}

			//***** TASK_1Hz ******
			if (run(TASK_1Hz,TICK_PASSED))
			{
				// set_led_red(TOGGLE);
				// igc_task();
			}
		}
	}
	return 0;
}

