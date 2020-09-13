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
	SysTick_Config(SYSTICK_TICKS);
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
	i2c_register_ipc();
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
	
	init_lcd();
	gui_bootlogo();
	exti_init();
	sound_init();
	timer_init();

	
	set_led_red(ON);	
	wait_systick(10);
	set_led_red(OFF);
	// init_BMS();

	gps_init();
	gui_init();


	//***** initialize the scheduler *****
	init_scheduler();
	schedule(TASK_GROUP_CORE, SCHEDULE_100ms); 		//run core task every 100ms
	schedule(TASK_GROUP_AUX,  SCHEDULE_1ms);		//run aux task every systick for now
	schedule(TASK_GROUP_1Hz,  SCHEDULE_1s);			//run every 1s

	while(1)
	{
		//***** TASK_GROUP_AUX ******
		if (run(TASK_GROUP_AUX))
		{
			set_led_red(ON);
			sdio_task();
			igc_task();
			ms5611_task();
			set_led_red(OFF);
		}

		//***** TASK_GROUP_CORE ******
		if (run(TASK_GROUP_CORE))
		{
			set_led_green(ON);
			datafusion_task();
			vario_task();
			system_task();
			sound_task();
			gui_task();
			gps_task();
			// BMS_task();
			set_led_green(OFF);
		}

		//***** TASK_GROUP_1Hz ******
		if (run(TASK_GROUP_1Hz))
		{
		}

		//***** Background Tasks *****
		i2c_task();
		md5_task();
	}
	return 0;
};

