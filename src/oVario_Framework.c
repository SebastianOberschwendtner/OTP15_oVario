/**
 * joVario Firmware
 * Copyright (c) 2020 Sebastian Oberschwendtner, sebastian.oberschwendtner@gmail.com
 * Copyright (c) 2020 Jakob Karpfinger, kajacky@gmail.com
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
/**
 ******************************************************************************
 * @file    oVario_Framework.c
 * @author  SO
 * @version V1.1
 * @date    25-February-2018
 * @brief   Handles the core system tasks and defines the schedule of the
 * 			other tasks.
 ******************************************************************************
 */
//****** Includes ******
#include "oVario_Framework.h"

//****** Variables ******
SDIO_T* p_ipc_sys_sd_data;		//IPC Data from SD-card
BMS_T* 	p_ipc_sys_bms_data;		//IPC Data from BMS
SYS_T* sys;						//IPC System Data
T_command rxcmd_system;			//IPC command struct for received commands
T_command txcmd_system;			//IPC command struct for commands to transmit
TASK_T task_system;				//Task struct for arbiter

//****** Inline Functions ******
/**
 * @brief Get the return value of the last finished ipc task which was called.
 * @return Return value of command.
 * @details inline function
 */
inline unsigned long system_get_call_return(void)
{
    return txcmd_system.data;
};

//****** Functions ******
/**
 * @brief Register everything relevant for IPC
 */
void sys_register_ipc(void)
{
	//register system struct
	sys = ipc_memory_register(sizeof(SYS_T), did_SYS);
	sys->TimeOffset = 0;
	set_time(20, 15, 00);
	set_date(23, 2, 2018);

	//register command queue
	ipc_register_queue(5 * sizeof(T_command), did_SYS);

	//Initialize the system task
	arbiter_clear_task(&task_system);
//Important!: When there is no softbutton installed, initialize the sys_state with NOSTATES
#ifndef SOFTSWITCH
	arbiter_set_command(&task_system, CMD_IDLE);
#else
	arbiter_set_command(&task_system, SYS_CMD_INIT);
#endif
};

/**
* @brief Get everything relevant for IPC
 */
void sys_get_ipc(void)
{
	// get the ipc pointer addresses for the needed data
	p_ipc_sys_sd_data = ipc_memory_get(did_SDIO);
	p_ipc_sys_bms_data = ipc_memory_get(did_BMS);
};

/**
 ***********************************************************
 * @brief TASK System
 ***********************************************************
 * 
 * 
 ***********************************************************
 * @details
 * Execution:	non-interruptable
 * Wait: 		Yes
 * Halt: 		Yes
 **********************************************************
 */
void system_task(void)
{
	//When the task wants to wait
	if (task_system.wait_counter)
		task_system.wait_counter--; //Decrease the wait counter
	else							//Execute task when wait is over
	{
		if (task_system.halt_task == 0)
		{
			//Perform action according to active state
			switch (arbiter_get_command(&task_system))
			{
			case CMD_IDLE:
				//When in idle this task does and will do nothing
				break;

			case SYS_CMD_INIT:
				system_init();
				break;

			case SYS_CMD_RUN:
				system_run();
				break;

			case SYS_CMD_SHUTDOWN:
				system_shutdown();
				break;

			default:
				break;
			}
		}
		else
			system_check_semaphores();
	}
	///@todo Check for errors here
};

/**
 * @brief Check the semaphores in the ipc queue.
 * 
 * This command is called before the command action. Since the system is non-
 * interruptable, this command checks only for semaphores.
 * 
 * @details Other commands are checked in the idle command.
 */
void system_check_semaphores(void)
{
    //Check semaphores for BMS
    if (ipc_get_queue_bytes(did_SYS) >= sizeof(T_command))      // look for new command in keypad queue
	{
		ipc_queue_get(did_SYS, sizeof(T_command), &txcmd_system);  // get new command
		//Evaluate semaphores
		switch (txcmd_system.cmd)
		{
		case cmd_ipc_signal_finished:
			//The called task is finished, decrease halt counter
			task_system.halt_task -= txcmd_system.did;
			break;

		default:
			break;
		}
	}
};

/**
 **********************************************************
 * @brief Initialize the System
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details call-by-reference
 **********************************************************
 */
void system_init(void)
{
	//perform the command action
	switch (arbiter_get_sequence(&task_system))
	{
	case SEQUENCE_ENTRY:
		// to prevent unwanted behaviour engage the watchdog
		sys_watchdog(ON);

		//goto next sequence
		arbiter_set_sequence(&task_system, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		// Pet the watchdog
		sys_watchdog(PET);

		//reset sequence
		arbiter_reset_sequence(&task_system);

		// goto run state
		arbiter_set_command(&task_system, SYS_CMD_RUN);
		break;

	default:
		break;
	}
};

/**
 **********************************************************
 * @brief Keep the system running
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details call-by-reference
 **********************************************************
 */
void system_run(void)
{
	// Pet the watchdog
	sys_watchdog(PET);
	// Keep the system on
	GPIOC->BSRRL = GPIO_BSRR_BS_6;

	// When no log is going on and a sd-card is inserted, start logging
	if ((igc_get_state() == IGC_LOG_CLOSED) && (p_ipc_sys_sd_data->status & SD_CMD_FINISHED) && (p_ipc_sys_sd_data->status & SD_CARD_SELECTED))
	{
		txcmd_system.did = did_SYS;
		txcmd_system.cmd = cmd_igc_start_logging;
		txcmd_system.data = 0;
		txcmd_system.timestamp = TIM5->CNT;
		// ipc_queue_push(&txcmd_system, sizeof(T_command), did_IGC);
	}

	// When power switch is off, initiate shutdown procedure
	// Unless there is input power present or no battery is present
	if ((!SHUTDOWN_SENSE) && !(p_ipc_sys_bms_data->charging_state & STATUS_INPUT_PRESENT) && (p_ipc_sys_bms_data->charging_state & STATUS_BAT_PRESENT))
		arbiter_callbyreference(&task_system, SYS_CMD_SHUTDOWN, 0);
};

/**
 **********************************************************
 * @brief Shut the system down
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details call-by-reference
 **********************************************************
 */
void system_shutdown(void)
{
	// Pet the watchdog
	sys_watchdog(PET);

	//Perform the command action
	switch (arbiter_get_sequence(&task_system))
	{
	case SEQUENCE_ENTRY:
		if (!SHUTDOWN_SENSE) // Check whether the power switch is still in the off position
		{
			// When a log is going on, stop logging
			if (igc_get_state() == IGC_LOGGING)
			{
				txcmd_system.did = did_SYS;
				txcmd_system.cmd = cmd_igc_stop_logging;
				txcmd_system.data = 0;
				txcmd_system.timestamp = TIM5->CNT;
				ipc_queue_push((void *)&txcmd_system, 10, did_IGC);
			}
			//goto next sequence and wait there until files are closed
			arbiter_set_sequence(&task_system, SYS_SEQUENCE_FILECLOSING);
		}
		else // When the power switch just bounced, go back to run state
			arbiter_return(&task_system, 0);
		break;

	case SYS_SEQUENCE_FILECLOSING:
		// Wait until files are closed
		if ((igc_get_state() == IGC_LOG_CLOSED) || (igc_get_state() == IGC_LOG_FINISHED))
		{
			//Wai until sd-card is ejected
			if (sdio_set_inactive())
			{
				//Send infobox
				txcmd_system.did = did_SYS;
				txcmd_system.cmd = cmd_gui_set_std_message;
				txcmd_system.data = data_info_shutting_down;
				txcmd_system.timestamp = TIM5->CNT;
				ipc_queue_push(&txcmd_system, 10, did_GUI);

				//Set wait
				task_system.wait_counter = 5;
				//goto next sequence
				arbiter_set_sequence(&task_system, SYS_SEQUENCE_SAVESOC);
			}
		}
		break;

	case SYS_SEQUENCE_SAVESOC:
			// when battery gauge is active, save the capacity to the the gauge flash
			if (p_ipc_sys_bms_data->charging_state & STATUS_GAUGE_ACTIVE)
				system_call_task(BMS_CMD_SAVE_SOC, 0, did_BMS);

			//set wait
			task_system.wait_counter = 5;

			//Goto next sequence
			arbiter_set_sequence(&task_system, SYS_SEQUENCE_HALTSYSTEM);
		break;

	case SYS_SEQUENCE_HALTSYSTEM:
			// Shutdown power and set PC6 low
			GPIOC->BSRRH = GPIO_BSRR_BS_6;

			//goto next sequence
			arbiter_set_sequence(&task_system, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		break;

	default:
		break;
	}
};

/**
 * @brief Call a other task via the ipc queue.
 * @param cmd The command the called task should perform
 * @param data The data for the command
 * @param did_target The did of the ipc queue of the called task
 */
void system_call_task(unsigned char cmd, unsigned long data, unsigned char did_target)
{
	//Set the command and data for the target task
	txcmd_system.did = did_SYS;
	txcmd_system.cmd = cmd;
	txcmd_system.data = data;

	//Push the command
	ipc_queue_push(&txcmd_system, sizeof(T_command), did_target);

	//Set wait counter to wait for called task to finish
	task_system.halt_task += did_target;
};

/**
 * @brief Initialize the clocks for the peripherals and the PLL
 */
void init_clock(void)
{
	RCC->CR |= RCC_CR_HSEON;
	while(!(RCC->CR & RCC_CR_HSERDY));
	RCC->CFGR = RCC_CFGR_MCO2_1 | RCC_CFGR_MCO2_0 | RCC_CFGR_MCO2PRE_2 | RCC_CFGR_SW_HSE;
	while(!(RCC->CFGR & RCC_CFGR_SWS_HSE));
	RCC->CR = RCC_CR_HSEON;
	/*
	 * Set FLASH wait states according to datasheet
	 * -> very important, do not forget!!!
	 */
	FLASH->ACR |= FLASH_ACR_LATENCY_5WS;
	/*
	 * PLL mit HSE  25MHz
	 * PLLM = 25	1MHz
	 * PLLN = 336	336MHz
	 * PLLP = 2		168MHz
	 * PLLQ = 7		48MHz
	 */
	RCC->PLLCFGR = (1<<29) | RCC_PLLCFGR_PLLSRC_HSE | (PLL_Q<<24) | (PLL_P<<16) | (PLL_N<<6) | (PLL_M<<0);
	//Clocks aktivieren
	RCC->CR |= RCC_CR_PLLON;
	while(!(RCC->CR & RCC_CR_PLLRDY));
	RCC->CFGR = RCC_CFGR_SW_PLL;
	while(!(RCC->CFGR & RCC_CFGR_SWS_PLL));

	/*
	 * set prescalers for peripherals:
	 * Peripheral	Speed	Prescaler
	 * APB2:			84 MHz	2  (PRE2)
	 * APB1:			42 MHz  4  (PRE1)
	 * RTC:	   		 1 MHz 25  (RTCPRE)
	 */
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4 | (25<<16);

	/*
	 * Activate clock (LSI) for independent watchdog
	 */
	RCC->CSR |= RCC_CSR_LSION;

	//Activate FPU
	SCB->CPACR = (1<<23) | (1<<22) | (1<<21) | (1<<20);

};

/**
 * @brief Enable Clock for GPIO
 * @param ch_port The port where the clock should be enabled
 */
void gpio_en(unsigned char ch_port)
{
	unsigned long l_temp = RCC->AHB1ENR;
	RCC->AHB1ENR = l_temp | (1<<ch_port);
};

/**
 * @brief Init systick timer
 * @param i_ticktime The time the SysTick timer generates the ticks in ms
 */
void init_systick_ms(unsigned long l_ticktime)
{
#ifndef F_CPU
# warning "F_CPU has to be defined"
# define F_CPU 1000000UL
#endif
	unsigned long l_temp = (F_CPU/8000)*l_ticktime;

	//Check whether the desired timespan is possible with the 24bit SysTick timer
	if(l_temp <= SysTick_LOAD_RELOAD_Msk)
		SysTick->LOAD = l_temp;
	else
		SysTick->LOAD = SysTick_LOAD_RELOAD_Msk;
	NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);  /* set Priority for Systick Interrupt */
	SysTick->VAL = 0x00;
	// Systick from AHB
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
};

/**
 * @brief Init system GPIO ports and pins
 * PB1: OUT Push-Pull	LED_RED
 * PB0: OUT Push-Pull	LED_GREEN
 * PC6: OUT	Push-Pull	SHUTDOWN_OUT
 * PC7: In	Hi-Z		SHUTDOWN_SENSE
 */
void init_gpio(void)
{
	//Enable clock
	gpio_en(GPIO_B);
	gpio_en(GPIO_C);

	//Set output
	// PB0 and PB1
	GPIOB->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0;
	// PC6 and PC7
	GPIOC->MODER |= GPIO_MODER_MODER6_0;

	//Set Pin configuration
	//GPIOC->OTYPER |= GPIO_OTYPER_OT_6;

	//Set SHUTDOWN_OUT to high
	GPIOC->BSRRL = GPIO_BSRR_BS_6;
};

/**
 * @brief Set green LED
 * @param ch_state The new state of the LED (ON, OFF, TOGGLE)
 */
void set_led_green(unsigned char ch_state)
{
	if(ch_state == ON)
		GPIOB->BSRRL = GPIO_BSRR_BS_0;
	else if(ch_state == TOGGLE)
		GPIOB->ODR ^= GPIO_ODR_ODR_0;
	else
		GPIOB->BSRRH = (GPIO_BSRR_BR_0>>16);
};

/**
 * @brief Set red LED
 * @param ch_state The new state of the LED (ON, OFF, TOGGLE)
 */
void set_led_red(unsigned char ch_state)
{
	if(ch_state == ON)
		GPIOB->BSRRL = GPIO_BSRR_BS_1;
	else if(ch_state == TOGGLE)
		GPIOB->ODR ^= GPIO_ODR_ODR_1;
	else
		GPIOB->BSRRH = (GPIO_BSRR_BR_1>>16);
};

/**
 * @brief Wait for a certain time in ms
 * The function counts up to a specific value to reach the desired wait time.
 * The count value is calculated using the system clock speed F_CPU.
 * @param l_time The time to wait in ms.
 * @deprecated Should not be used anymore. Use the timing of the arbiter, when you need to wait.
 */
void wait_ms(unsigned long l_time)
{
	unsigned long l_temp = CURRENT_TICK;
	signed long l_target = l_temp-((F_CPU/1000)*l_time);
	if(l_target<0)
	{
		wait_systick((l_time*1000)/SYSTICK);
		while(SysTick->VAL > (SYSTICK_TICKS+l_target));
	}
	else
	{
		while(SysTick->VAL > l_target);
	}
};

/**
 * @brief Wait for a certain amount of SysTicks
 * @param l_ticks The time to wait in multiple of Systicks
 * @deprecated Should not be used anymore. Use the timing of the arbiter, when you need to wait.
 */
void wait_systick(unsigned long l_ticks)
{
	for(unsigned long l_count = 0;l_count<l_ticks; l_count++)
		while(!TICK_PASSED);
};

/**
 * @brief Set the timezone using dates, i.e. sommer and winter time
 * @details The change is always on the last sunday of October/March, we do not track the weekday yet.
 * So the switch of the timezones only takes the month into account.
 */
void set_timezone(void)
{
	unsigned char month = get_month_utc();

	if( (month > 3) && (month <= 10) )
		sys->TimeOffset = 2;
	else
		sys->TimeOffset = 1;
};

/**
 * @brief Set the time in UTC!
 * @param ch_hour The hour of the time
 * @param ch_minute The minute of the time
 * @param ch_second The second of the time
 * @details Remember UTC!
 */
void set_time(unsigned char ch_hour, unsigned char ch_minute, unsigned char ch_second)
{
	sys->time = (ch_hour<<SYS_TIME_HOUR_pos) | (ch_minute<<SYS_TIME_MINUTE_pos) | ((ch_second)<<SYS_TIME_SECONDS_pos);
};

/**
 * @brief Get seconds of UTC time
 * @return The seconds of the current time
 */
unsigned char get_seconds_utc(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_SECONDS_pos) & 0x3F);
};

/**
 * @brief Get seconds of local time
 * @return The seconds of the current time
 */
unsigned char get_seconds_lct(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_SECONDS_pos) & 0x3F);
};

/**
 * @brief Get minutes of UTC time
 * @return The minutes of the current time
 */
unsigned char get_minutes_utc(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_MINUTE_pos) & 0x3F);
};

/**
 * @brief Get minutes of local time
 * @return The minutes of the current time
 */
unsigned char get_minutes_lct(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_MINUTE_pos) & 0x3F);
};

/**
 * @brief Get hours of UTC time
 * @return The hour of the current time
 */
unsigned char get_hours_utc(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_HOUR_pos) & 0x1F);
};

/**
 * @brief Get hours of local time
 * @return The hour of the current time
 */
unsigned char get_hours_lct(void)
{
	return (unsigned char)(((sys->time>>SYS_TIME_HOUR_pos) & 0x1F) + sys->TimeOffset);
};

/**
 * @brief Set the date in UTC!
 * @param ch_day The day of the date
 * @param ch_month The month of the date
 * @param i_year The year of the date
 */
void set_date(unsigned char ch_day, unsigned char ch_month, unsigned int i_year)
{
	sys->date = (unsigned int)(((unsigned char)(i_year-1980)<<SYS_DATE_YEAR_pos) | (ch_month<<SYS_DATE_MONTH_pos) | (ch_day<<SYS_DATE_DAY_pos));
};

/**
 * @brief Get day of date in UTC
 * @return The day of the current date
 */
unsigned char get_day_utc(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_DAY_pos) & 0x1F);
};

/**
 * @brief Get day of date in local time
 * @return The day of the current date
 */
unsigned char get_day_lct(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_DAY_pos) & 0x1F);
};

/**
 * @brief Get month of date in UTC
 * @return The month of the current date
 */
unsigned char get_month_utc(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_MONTH_pos) & 0xF);
};

/**
 * @brief Get month of date in local time
 * @return The month of the current date
 */
unsigned char get_month_lct(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_MONTH_pos) & 0xF);
};

/**
 * @brief Get year of date in UTC
 * @return The year of the current date
 */
unsigned int get_year_utc(void)
{
	return (unsigned int)(((sys->date>>SYS_DATE_YEAR_pos) & 0x3F) + 1980);
};

/**
 * @brief Get year of date in local time
 * @return The year of the current date
 */
unsigned int get_year_lct(void)
{
	return (unsigned int)(((sys->date>>SYS_DATE_YEAR_pos) & 0x3F) + 1980);
};

/**
 * @brief Compare two strings. The first string sets the compare length.
 * @param pch_string1 First input string. Sets the compare length
 * @param pch_string2 Second input string.
 * @return Returns 1 when the strings match upon the length of the first string.
 */
unsigned char sys_strcmp(char* pch_string1, char* pch_string2)
{
	while(*pch_string1 != 0)
	{
		if(*pch_string1++ != *pch_string2++)
			return 0;
	}
	return 1;
};

/**
 * @brief Copy one string to another. The second string is copied to the first string.
 * @param pch_string1 First input string. Sets the maximum copy length.
 * @param pch_string2 The string which is copied to the first string.
 * @return Returns 0 if string2 did not fit completely in string1.
 */
unsigned char sys_strcpy(char* pch_string1, char* pch_string2)
{
	while(*pch_string2 != 0)
	{
		//check for end of string1
		if(*pch_string1 == 0)
			return 0;
		//If the end of string1 is not reached fill it with the next character of string2
		*pch_string1 = *pch_string2;
		//Increase pointers
		pch_string1++;
		pch_string2++;
	}
	return 1;
};

/**
 * @brief Write a number with a specific amount of digits to a string in dec format.
 * This function copies directly to the input string.
 * @param string The string where the number should be encoded
 * @param l_number The number to convert
 * @param ch_digits How many digits the output string should have.
 * @return Returns 1 when operation was successful.
 */
unsigned char sys_num2str(char* string, unsigned long l_number, unsigned char ch_digits)
{
	//Compute the string content
	for(unsigned char ch_count = 0; ch_count<ch_digits;ch_count++)
	{
		string[ch_digits - ch_count -1] = (unsigned char)(l_number%10)+48;
		l_number /=10;
	}
	return 1;
};

/**
 * @brief Copy memory.
 * length equals the whole data length in bytes!
 * @param data1 The pointer where the data should be copied to
 * @param data2 The pointer of the data which should be copied
 * @param length The number of bytes which should be copied
 */
void sys_memcpy(void* data1, void* data2, unsigned char length)
{
	unsigned char* temp1 = (unsigned char*)data1;
	unsigned char* temp2 = (unsigned char*)data2;

	for(unsigned char count = 0; count < length; count++)
	{
		*temp1 = *temp2;
		temp1++;
		temp2++;
	}
};

/**
 * @brief Write a number with a specific amount of digits to a string in hex format.
 * This function copies directly to the input string.
 * @param string The string where the number should be encoded
 * @param l_number The number to convert
 * @param ch_digits How many digits the output string should have.
 * @return Returns 1 when operation was successful.
 */
unsigned char sys_hex2str(char* string, unsigned long l_number, unsigned char ch_digits)
{
	unsigned char result = 0;

	//Compute the string content
	for(unsigned char ch_count = 0; ch_count<ch_digits;ch_count++)
	{
		result = (unsigned char)(l_number%16);

		//If hex character is required increase result with 7.
		if(result > 9)
			result += 39;

		string[ch_digits - ch_count -1] = result + 48;
		l_number /=16;
	}
	return 1;
};

/**
 * @brief Swap the endianess of the input value, 
 * the byte width of the data has to be specified.
 * @param data_in The input number where the bytes should be swapped
 * @param byte_width The number of relevant input bytes
 * @return The byte swapped number.
 */
unsigned long sys_swap_endian(unsigned long data_in, unsigned char byte_width)
{
	// Initialize output value
	unsigned long data_out = 0;
	// Char pointer for byte swapping
	unsigned char* data = (unsigned char*)&data_in;
	
	//Swap the bytes
	for (unsigned char i = 0; i < byte_width; i++)
		data_out += (data[byte_width - i - 1] << (8 * i));

	//Return the result
	return data_out;
};

/**
 * @brief Function to handle the watchdog timer. you can activate,
 *  reset and deactivate the timer. Timeout is fixed to approx. 500ms.
 * @param action The action to perform with the watchdog.
 */
void sys_watchdog(unsigned char action)
{
	// check which action to perform
	switch(action)
	{
	case ON:
		IWDG->KR  = 0x5555;	//Enable write to PR
		IWDG->PR  = 2;		// Set prescaler to 16
		IWDG->KR  = 0x5555;	//Enable write to RL
		IWDG->RLR = 0xFFF;	// Set relaod value -> gives a timeout of approx. 2048ms
		IWDG->KR  = 0xCCCC;	//Unleash the watchdog
		break;
	case OFF:
		break;
	case PET:
		IWDG->KR = 0xAAAA;	//pet the watchdog
		break;
	default:
		break;
	}
};
