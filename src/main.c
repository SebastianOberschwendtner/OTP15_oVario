/**
 ******************************************************************************
 * @file    main.c
 * @author  SO
 * @version V2.0
 * @date    19-September-2020
 * @brief   Default main function.
 ******************************************************************************
 */

//****** Includes ******
#include "oVario_Framework.h" //<--- define your hardware setup here

//****** Variables ******
uint32_t error_var = 0;		//Global errors variable
T_command txcmd_main;		//Command struct to send ipc commands

//****** Functions ******
/**
  * @brief Interrupt handler for Sytick timer. Handles the schedule
  * @details interrupt handler
  */
void SysTick_Handler(void)
{
	//***** Run scheduler *****
	run_scheduler();
};

/**
 * @brief main function of the Vario.
 */
int main(void)
{
	//***** Initialize core ****
	///@todo Make the following cleaner
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
	
	//***** Initialize peripherals *****
	///@todo Eventually move all the following initializations to the corresponding tasks
	init_lcd();
	gui_bootlogo();
	exti_init();
	sound_init();
	timer_init();
	
	set_led_red(ON);	
	wait_systick(10);
	set_led_red(OFF);

	gps_init();
	gui_init();

	//***** initialize the scheduler *****
	init_scheduler();
	schedule(TASK_GROUP_BACKGROUND, SCHEDULE_100us);//run background task every 100us
	schedule(TASK_GROUP_CORE, SCHEDULE_100ms); 		//run core task every 100ms
	schedule(TASK_GROUP_AUX,  SCHEDULE_1ms);		//run aux task every systick for now
	schedule(TASK_GROUP_1Hz,  SCHEDULE_1s);			//run every 1s

	while(1)
	{
		//***** TASK_GROUP_AUX ******
		if (run(TASK_GROUP_AUX))
		{
			// set_led_red(ON);
			sdio_task();
			igc_task();
			// ms5611_task();
			bms_task();
			
			// set_led_red(OFF);
		}

		//***** TASK_GROUP_CORE ******
		if (run(TASK_GROUP_CORE))
		{
			// set_led_green(ON);
			datafusion_task();
			vario_task();
			system_task();
			// sound_task();
			gui_task();
			gps_task();
			// set_led_green(OFF);
		}

		//***** TASK_GROUP_1Hz ******
		if (run(TASK_GROUP_1Hz))
		{
			//Read the status of the bms once a second
			txcmd_main.did = 0xFF;
			txcmd_main.cmd = BMS_CMD_GET_ADC;
			txcmd_main.data = 0;
			txcmd_main.timestamp = 0;
			// ipc_queue_push(&txcmd_main, sizeof(T_command), did_BMS);
		}

		//***** Background Tasks *****
		if (run(TASK_GROUP_BACKGROUND))
		{
			md5_task();
			i2c_task();
		}
	}
	return 0;
};

