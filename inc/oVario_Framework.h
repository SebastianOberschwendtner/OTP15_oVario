/*
 * oVario_Framework.h
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#ifndef OVARIO_FRAMEWORK_H_
#define OVARIO_FRAMEWORK_H_

#include "stm32f4xx.h"
#include "did.h"
#include "ipc.h"


/*
 * Defines for system
 */
//CPU Speed
#define	F_CPU			168021840UL //Measured with osci
//Define SysTick time in ms
#define SYSTICK			100
//I2C clock speed
#define I2C_CLOCK		10000UL

//BMS parameters
#define MAX_BATTERY_VOLTAGE	4200 //[mV]
#define MIN_BATTERY_VOLTAGE	2800 //[mV]
#define MAX_CURRENT			3000 //[mA]
#define OTG_VOLTAGE			5000 //[mV]
#define OTG_CURRENT			3000 //[mA]

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

//Positions for date and time bits
#define SYS_DATE_YEAR_pos		9
#define SYS_DATE_MONTH_pos		5
#define SYS_DATE_DAY_pos		0

#define SYS_TIME_HOUR_pos		11
#define SYS_TIME_MINUTE_pos		5
#define SYS_TIME_SECONDS_pos	0



void init_clock(void);
void gpio_en(unsigned char ch_port);
void init_systick_ms(unsigned long l_ticktime);
void init_led(void);
void set_led_green(unsigned char ch_state);
void set_led_red(unsigned char ch_state);
void wait_ms(unsigned long l_time);
void wait_systick(unsigned long l_ticks);
void set_time(unsigned char ch_hour, unsigned char ch_minute, unsigned char ch_second);
void set_date(unsigned char ch_day, unsigned char ch_month, unsigned int i_year);

#endif /* OVARIO_FRAMEWORK_H_ */
