/*
 * oVario_Framework.c
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#include "oVario_Framework.h"
#include "Variables.h"

SYS_T* sys;

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

	//Activate FPU
	SCB->CPACR = (1<<23) | (1<<22) | (1<<21) | (1<<20);

	//register system struct
	sys = ipc_memory_register(8,did_SYS);
	set_time(20,15,0);
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
 * Init LED ports
 * PB1: OUT LED_RED
 * PB0: OUT LED_GREEN
 */
void init_led(void)
{
	//Enable clock
	gpio_en(GPIO_B);

	//Set output
	GPIOB->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0;
}
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
 * The coutn value is calculated using the system cock speed F_CPU.
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
 * Set the time
 */
void set_time(unsigned char ch_hour, unsigned char ch_minute, unsigned char ch_second)
{
	sys->time = (ch_hour<<SYS_TIME_HOUR_pos) | (ch_minute<<SYS_TIME_MINUTE_pos) | ((ch_second)<<SYS_TIME_SECONDS_pos);
};

/*
 * Get seconds of time
 */
unsigned char get_seconds(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_SECONDS_pos) & 0x3F);
};

/*
 * Get minutes of time
 */
unsigned char get_minutes(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_MINUTE_pos) & 0x3F);
};

/*
 * Get hours of time
 */
unsigned char get_hours(void)
{
	return (unsigned char)((sys->time>>SYS_TIME_HOUR_pos) & 0x1F);
};

/*
 * Set the date
 */
void set_date(unsigned char ch_day, unsigned char ch_month, unsigned int i_year)
{
	sys->date = (unsigned int)(((unsigned char)(i_year-1980)<<SYS_DATE_YEAR_pos) | (ch_month<<SYS_DATE_MONTH_pos) | (ch_day<<SYS_DATE_DAY_pos));
};

/*
 * Get Day of date
 */
unsigned char get_day(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_DAY_pos) & 0x1F);
};

/*
 * Get Month of date
 */
unsigned char get_month(void)
{
	return (unsigned char)((sys->date>>SYS_DATE_MONTH_pos) & 0x1F);
};

/*
 * Get Day of date
 */
unsigned int get_year(void)
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
