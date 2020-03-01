/*
 * oVario_Framework.c
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#include "oVario_Framework.h"
#include "Variables.h"

SDIO_T* p_ipc_sys_sd_data;
BMS_T* 	p_ipc_sys_bms_data;

SYS_T* sys;
T_command SysCmd;

//Important!: When there is no softbutton installed, initialize the sys_state with NOSTATES
#ifndef SOFTSWITCH
uint8_t sys_state = NOSTATES;
#else
uint8_t sys_state = INITIATE;
#endif
uint8_t count = 0;

/*
 * System task, which monitors whether the system should be shutdown
 */
void system_task(void)
{
	// perform action according to current state of system
	switch(sys_state)
	{
	// do not use state machine
	case NOSTATES:
		break;

	// Initiate the sys state machine
	case INITIATE:
		// get the ipc pointer addresses for the needed data
		p_ipc_sys_sd_data		= ipc_memory_get(did_SDIO);
		p_ipc_sys_bms_data 		= ipc_memory_get(did_BMS);

		// to prevent unwanted behaviour engage the watchdog
		sys_watchdog(ON);

		// when battery gauge is active, read the old capacity from the gauge flash
		if (p_ipc_sys_bms_data->charging_state & STATUS_GAUGE_ACTIVE)
			p_ipc_sys_bms_data->old_capacity = (signed int)BMS_gauge_read_flash_int(MAC_INFO_BLOCK_addr);

		// goto run state
		sys_state = RUN;
		break;

	// Normal Run state
	case RUN:
		// Pet the watchdog
		sys_watchdog(PET);

		// Keep the system on
		GPIOC->BSRRL = GPIO_BSRR_BS_6;
		// When power switch is off, initiate shutdown procedure
		// Unless there is input power present or no battery is present
		if((!SHUTDOWN_SENSE)&&!(p_ipc_sys_bms_data->charging_state & STATUS_INPUT_PRESENT)&&(p_ipc_sys_bms_data->charging_state & STATUS_BAT_PRESENT))
				sys_state = SHUTDOWN;
		break;

	// When the power switch was switched
	case SHUTDOWN:
		// Pet the watchdog
		sys_watchdog(PET);

		if(!SHUTDOWN_SENSE)	// Check whether the power switch is still in the off position
		{
				// When no log is going on, but a sd-card is inserted, eject the card
				SysCmd.did 		= did_SYS;
				SysCmd.cmd 		= cmd_igc_eject_card;
				SysCmd.timestamp 	= TIM5->CNT;
				ipc_queue_push((void*)&SysCmd, 10, did_IGC);

				sys_state = FILECLOSING;
		}
		else // When the power switch just bounced, go back to run state
			sys_state = RUN;
		break;

	// Wait until the sd card is ejected
	case FILECLOSING:
		// Pet the watchdog
		sys_watchdog(PET);

		// Check whether there is no sd card active
		if(!(p_ipc_sys_sd_data->state & SD_CARD_DETECTED))
			sys_state = SAVESOC;
		break;

	// Save current state of charge of the battery
	case SAVESOC:
		// Pet the watchdog
		sys_watchdog(PET);

		count++;
		if(count == 5)
		{
			count = 0;
			sys_state = HALTSYSTEM;
			//Send infobox
			SysCmd.did 			= did_SYS;
			SysCmd.cmd			= cmd_gui_set_std_message;
			SysCmd.data 		= data_info_shutting_down;
			SysCmd.timestamp 	= TIM5->CNT;
			ipc_queue_push(&SysCmd, 10, did_GUI);

			// when battery gauge is active, save the capacity to the the gauge flash
			if (p_ipc_sys_bms_data->charging_state & STATUS_GAUGE_ACTIVE)
				BMS_gauge_send_flash_int(MAC_INFO_BLOCK_addr, (unsigned int)p_ipc_sys_bms_data->discharged_capacity);
		}
		break;

	// Halt the system and cut the power
	case HALTSYSTEM:
		// Pet the watchdog
		sys_watchdog(PET);

		count++;
		if(count == 5){
			p_ipc_sys_bms_data->crc = i2c_read_char(i2c_addr_BMS_GAUGE, MAC_SUM_addr);
			p_ipc_sys_bms_data->len = i2c_read_char(i2c_addr_BMS_GAUGE, MAC_LEN_addr);
		// Shutdown power and set PC6 low
		GPIOC->BSRRH =GPIO_BSRR_BS_6;
		}
		break;

	default:
		sys_state = NOSTATES;
		break;
	}
};

/*
 * Init Clock
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

	//register system struct
	sys = ipc_memory_register(sizeof(SYS_T),did_SYS);
	sys->TimeOffset = 0;
	set_time(20,15,00);
	set_date(23,2,2018);

};

/*
 * Enable Clock for GPIO
 */
void gpio_en(unsigned char ch_port)
{
	unsigned long l_temp = RCC->AHB1ENR;
	RCC->AHB1ENR = l_temp | (1<<ch_port);
};

/*
 * Init systick timer
 * i_ticktime: time the flag_tick is set in ms
 */
void init_systick_ms(unsigned long l_ticktime)
{
#ifndef F_CPU
# warning "F_CPU has to be defined"
# define F_CPU 1000000UL
#endif
	unsigned long l_temp = (F_CPU/8000)*l_ticktime;

	//Check whether the desired timespan is possible with the 24bit SysTick timer
	if(l_temp <= 0x1000000)
		SysTick->LOAD = l_temp;
	else
		SysTick->LOAD = 0x1000000;
	SysTick->VAL = 0x00;
	// Systick from AHB
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
}

/*
 * Init system GPIO ports and pins
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

/*
 * Set green LED
 */
void set_led_green(unsigned char ch_state)
{
	if(ch_state == ON)
		GPIOB->BSRRL = GPIO_BSRR_BS_0;
	else if(ch_state == TOGGLE)
		GPIOB->ODR ^= GPIO_ODR_ODR_0;
	else
		GPIOB->BSRRH = (GPIO_BSRR_BR_0>>16);
}
/*
 * Set red LED
 */
void set_led_red(unsigned char ch_state)
{
	if(ch_state == ON)
		GPIOB->BSRRL = GPIO_BSRR_BS_1;
	else if(ch_state == TOGGLE)
		GPIOB->ODR ^= GPIO_ODR_ODR_1;
	else
		GPIOB->BSRRH = (GPIO_BSRR_BR_1>>16);
}
/*
 * Wait for a certain time in ms
 * The function counts up to a specific value to reach the desired wait time.
 * The count value is calculated using the system clock speed F_CPU.
 */
void wait_ms(unsigned long l_time)
{
	unsigned long l_temp = CURRENT_TICK;
	signed long l_target = l_temp-((F_CPU/8000)*l_time);
	if(l_target<0)
	{
		wait_systick(1);
		while(SysTick->VAL>(((F_CPU/8000)*SYSTICK)+l_target));
	}
	else
	{
		while(SysTick->VAL>l_target);
	}
}
/*
 * Wait for a certain amount of SysTicks
 */
void wait_systick(unsigned long l_ticks)
{
	for(unsigned long l_count = 0;l_count<l_ticks; l_count++)
		while(!TICK_PASSED);
}

/*
 * Set the timezone using dates, i.e. sommer and winter time
 * Problem: the change is always on the last sunday of October/March, we do not track the weekday yet.
 * So the switchof the timezones only takes the month into account.
 */
void set_timezone(void)
{
	unsigned char month = get_month_utc();

	if( (month > 3) && (month <= 10) )
		sys->TimeOffset = 2;
	else
		sys->TimeOffset = 1;
};

/*
 * Set the time in UTC!
 */
void set_time(unsigned char ch_hour, unsigned char ch_minute, unsigned char ch_second)
{
	sys->time = (ch_hour<<SYS_TIME_HOUR_pos) | (ch_minute<<SYS_TIME_MINUTE_pos) | ((ch_second)<<SYS_TIME_SECONDS_pos);
};

/*
 * Get seconds of time
 */
unsigned char get_seconds_utc(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_SECONDS_pos) & 0x3F);
};
unsigned char get_seconds_lct(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_SECONDS_pos) & 0x3F);
};

/*
 * Get minutes of time
 */
unsigned char get_minutes_utc(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_MINUTE_pos) & 0x3F);
};
unsigned char get_minutes_lct(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_MINUTE_pos) & 0x3F);
};

/*
 * Get hours of time
 */
unsigned char get_hours_utc(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_HOUR_pos) & 0x1F);
};
unsigned char get_hours_lct(void)
{
	return (unsigned char)(((sys->time>>SYS_TIME_HOUR_pos) & 0x1F) + sys->TimeOffset);
};



/*
 * Set the date in UTC!
 */
void set_date(unsigned char ch_day, unsigned char ch_month, unsigned int i_year)
{
	sys->date = (unsigned int)(((unsigned char)(i_year-1980)<<SYS_DATE_YEAR_pos) | (ch_month<<SYS_DATE_MONTH_pos) | (ch_day<<SYS_DATE_DAY_pos));
};

/*
 * Get Day of date
 */
unsigned char get_day_utc(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_DAY_pos) & 0x1F);
};
unsigned char get_day_lct(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_DAY_pos) & 0x1F);
};

/*
 * Get Month of date
 */
unsigned char get_month_utc(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_MONTH_pos) & 0xF);
};
unsigned char get_month_lct(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_MONTH_pos) & 0xF);
};

/*
 * Get Day of date
 */
unsigned int get_year_utc(void)
{
	return (unsigned int)(((sys->date>>SYS_DATE_YEAR_pos) & 0x3F) + 1980);
};
unsigned int get_year_lct(void)
{
	return (unsigned int)(((sys->date>>SYS_DATE_YEAR_pos) & 0x3F) + 1980);
};

/*
 * Compare two strings. The first string sets the compare length
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

/*
 * Copy one string to another. The second string is copied to the first string.
 * The functions returns 0 if string2 did not fit completely in string1.
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

/*
 * Write a number with a specific amount of digits to a stringin dec format.
 * This function copies directly to the input string.
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

/*
 * Copy memory.
 * length euqals the whole data length in bytes!
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

/*
 * Write a number with a specific amount of digits to a string in hex format.
 * This function copies directly to the input string.
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

/*
 * Function to handle the watchdog timer. you can activate, reset and deactivate the timer.
 * Timeout is fixed to approx. 500ms.
 */
void sys_watchdog(unsigned char action)
{
	// check which action to perform
	switch(action)
	{
	case ON:
		IWDG->KR  = 0x5555;	//Enable write to PR
		IWDG->PR  = 1;		// Set prescaler to 8
		IWDG->KR  = 0x5555;	//Enable write to RL
		IWDG->RLR = 0x3FF;	// Set relaod value to 2047 -> gives a timeout of approx. 512ms
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
}
