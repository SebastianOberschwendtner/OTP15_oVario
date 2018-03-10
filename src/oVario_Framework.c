/*
 * oVario_Framework.c
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#include "oVario_Framework.h"

/*
 * Init Clock
 */
void init_clock(void)
{
	RCC->CR |= RCC_CR_HSEON;
	while(!(RCC->CR & RCC_CR_HSERDY));
	RCC->CFGR = RCC_CFGR_SW_HSE;
	while(!(RCC->CFGR & RCC_CFGR_SWS_HSE));
	RCC->CR = RCC_CR_HSEON;
	FLASH->ACR |= FLASH_ACR_LATENCY_1WS;
	/*
	 * PLL mit HSE  25MHz
	 * PLLM = 25	1MHz
	 * PLLN = 336	336MHz
	 * PLLP = 2		168MHz
	 * PLLQ = 7		48MHz
	 */
	RCC->PLLCFGR = 0x20000000 | RCC_PLLCFGR_PLLSRC_HSE | (PLL_Q<<24) | (PLL_P<<16) | (PLL_N<<6) | (PLL_M<<0);
	//Clocks aktivieren
	RCC->CR |= RCC_CR_PLLON;
	while(!(RCC->CR & RCC_CR_PLLRDY));
	RCC->CFGR = RCC_CFGR_SW_PLL;
	while(!(RCC->CFGR & RCC_CFGR_SWS_PLL));

	/*
	 * set prescalers for peripherals:
	 * Peripheral	Speed	Prescaler
	 * APB2:		84 MHz	2  (PRE2)
	 * APB1:		42 MHz  4  (PRE1)
	 */
	//           RTCPRE     PRE2      PRE1
	RCC->CFGR |= (5<<16) | (2<<13) | (4<<10);
};

/*
 * Enable Clock for GPIO
 */
void gpio_en(unsigned char ch_port)
{
	RCC->AHB1ENR |= (1<<ch_port);
}

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
	for(unsigned long l_count = 0;l_count<((F_CPU/1000)*l_time); l_count++);
}
/*
 * Wait for a certain amount of SysTicks
 */
void wait_systick(unsigned long l_ticks)
{
	for(unsigned long l_count = 0;l_count<l_ticks; l_count++)
		while(!TICK_PASSED);
}
