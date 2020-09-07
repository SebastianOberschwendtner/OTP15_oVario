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
// unsigned long tempcount = 0;

void SysTick_Handler(void)
{
	//***** Run scheduler *****
	run_scheduler();
};

int main(void)
{
	//***** Initialize core ****
	//TODO Make the following cleaner
	init_clock();
	SysTick_Config(8400000UL);
	init_gpio();

	//**** Register everything for IPC *****
	sys_register_ipc();
	ms5611_register_ipc();
	datafusion_register_ipc();
	bms_register_ipc();
	gps_register_ipc();
	sound_register_ipc();
	vario_register_ipc();
	gui_register_ipc();
	sdio_register_ipc();
	igc_register_ipc();
	md5_register_ipc();

	//***** Get everything for IPC *****
	sys_get_ipc();
	datafusion_get_ipc();
	vario_get_ipc();
	gps_get_ipc();
	gui_get_ipc();
	sdio_get_ipc();
	igc_get_ipc();
	
	//***** intialize peripherals *****
	//TODO Eventually move all the following initalizations to the corresponding tasks
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
	gui_init();


	//***** initialize the scheduler *****
	init_scheduler();
	schedule(TASK_CORE,	100/SYSTICK); 		//run core task every 100ms
	schedule(TASK_AUX,	1);					//run aux task every systick for now
	//TODO Change the rate of the aux task
	schedule(TASK_1Hz,  1000/SYSTICK);	//run every 1s
	set_led_red(OFF);



	while(1)
	{
		//***** TASK_AUX ******
		if (run(TASK_AUX))
		{
			set_led_red(ON);
			sdio_task();
			igc_task();
			set_led_red(OFF);
		}

		//***** TASK_CORE ******
		if (run(TASK_CORE))
		{
			set_led_green(ON);
			ms5611_task();
			datafusion_task();
			vario_task();
			i2c_reset_error();
			system_task();
			// sound_task();
			gui_task();
			gps_task();
			BMS_task();
			set_led_green(OFF);
		}

		//***** TASK_1Hz ******
		if (run(TASK_1Hz))
		{
		}

		// tempcount++;

		// if(tempcount == 168000)
		// {
		// 	tempcount = 0;

		// }
		md5_task();
	}
	return 0;
};

