/*
 * oVario_Framework.c
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#include "oVario_Framework.h"
#include "stm32f4xx.h"
#include "Variables.h"

/*
 * Init Clock
 */
void init_clock(void)
{
	RCC->CR = RCC_CR_HSION;										//Use internal High Speed clock, 16 MHz
	while(!(RCC->CR & RCC_CR_HSIRDY));
	RCC->CFGR = RCC_CFGR_SW_HSI;
	while(!(RCC->CFGR & RCC_CFGR_SWS_HSI));
};

/*
 * Enable Clock for GPIO
 */
void en_gpio(unsigned char ch_port)
{
	RCC->AHB1ENR |= (1<<ch_port);
}

/*
 * Init systick timer
 * i_ticktime: time the flag_tick is set in ms
 */
void init_systick_ms(unsigned int i_ticktime)
{
#ifndef F_CPU
# warning "F_CPU has to be defined"
# define F_CPU 1000000UL
#endif
	unsigned long l_temp = (F_CPU/1000)*i_ticktime;

	//Check whether the desired timespan is possible with the 24bit SysTick timer
	if(l_temp <= 0x1000000)
		SysTick->LOAD = l_temp;
	else
		SysTick->LOAD = 0x1000000;
	SysTick->VAL = 0x00;
	// Systick from AHB
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

	//Enable interrupt
	NVIC_EnableIRQ(SysTick_IRQn);
}
