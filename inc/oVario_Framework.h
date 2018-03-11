/*
 * oVario_Framework.h
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#ifndef OVARIO_FRAMEWORK_H_
#define OVARIO_FRAMEWORK_H_

#include "stm32f4xx.h"


/*
 * Defines for system
 */
//CPU Speed
#define	F_CPU			168021840UL //Measured with osci
//Define SysTick time in ms
#define SYSTICK			100
//I2C clock speed
#define I2C_CLOCK		400000UL

//Makro for SysTick status
#define TICK_PASSED		(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)

//PLL variables
#define PLL_M			25
#define PLL_N			336
#define PLL_P			0	//Equals P=2, see datasheet of stm32
#define PLL_Q			7

//Port numbers of GPIOs
#define	GPIO_A			0
#define	GPIO_B			1
#define GPIO_C			2
#define	GPIO_D			3
#define	GPIO_E			4
#define	GPIO_F			5
#define	GPIO_G			6
#define	GPIO_H			7
#define	GPIO_I			8

//state defines
#define ON				1
#define OFF				0
#define TOGGLE			3



void init_clock(void);
void gpio_en(unsigned char ch_port);
void init_systick_ms(unsigned long l_ticktime);
void init_led(void);
void set_led_green(unsigned char ch_state);
void set_led_red(unsigned char ch_state);
void wait_ms(unsigned long l_time);
void wait_systick(unsigned long l_ticks);

#endif /* OVARIO_FRAMEWORK_H_ */
