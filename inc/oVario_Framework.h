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
#include "sdio.h"
#include "igc.h"
#include "BMS.h"


/*
 * Defines for system
 */
//CPU Speed
#define	F_CPU			168021840UL //Measured with osci
//Define SysTick time in ms
#define SYSTICK			100
//I2C clock speed
#define I2C_CLOCK		100000UL

//Options for hardware setup
#define SDIO_4WIRE //Use 4 wire mode of SDIO0
#define SOFTSWITCH //When the softswitch is installed

//BMS parameters
#define MAX_BATTERY_VOLTAGE	4200 //[mV]
#define MIN_BATTERY_VOLTAGE	3100 //[mV]
#define MAX_CURRENT			1000 //[mA]
#define OTG_VOLTAGE			5000 //[mV]
#define OTG_CURRENT			1000 //[mA]

/*
 * Makro for SysTick status and time conversion
 * The conversion assumes that the systick timer is clocked by APB/8.
 */
#define TICK_PASSED		(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
#define CURRENT_TICK	SysTick->VAL
#define MS2TICK(x)		((F_CPU/8000)*x)
#define US2TICK(x)		((F_CPU/8000000)*x)
#define TICK2MS(x)		(x*8000)/F_CPU
#define TICK2US(x)		(x*8000000)/F_CPU

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
#define PET				4

//sys state defines
#define INITIATE		1
#define RUN				2
#define SHUTDOWN		3
#define FILECLOSING		4
#define SAVESOC			5
#define HALTSYSTEM		6
#define NOSTATES		7

//Positions for date and time bits
#define SYS_DATE_YEAR_pos		9
#define SYS_DATE_MONTH_pos		5
#define SYS_DATE_DAY_pos		0

#define SYS_TIME_HOUR_pos		12
#define SYS_TIME_MINUTE_pos		6
#define SYS_TIME_SECONDS_pos	0

//defines for pin access
#define SHUTDOWN_SENSE			(GPIOC->IDR & (GPIO_IDR_IDR_7))


void 			system_task(void);
void 			init_clock(void);
void 			gpio_en(unsigned char ch_port);
void 			init_systick_ms(unsigned long l_ticktime);
void 			init_gpio(void);
void 			set_led_green(unsigned char ch_state);
void 			set_led_red(unsigned char ch_state);
void 			wait_ms(unsigned long l_time);
void 			wait_systick(unsigned long l_ticks);
void 			set_timezone(void);
void			set_time(unsigned char ch_hour, unsigned char ch_minute, unsigned char ch_second);
unsigned char	get_seconds_utc(void);
unsigned char 	get_seconds_lct(void);
unsigned char 	get_minutes_utc(void);
unsigned char 	get_minutes_lct(void);
unsigned char 	get_hours_utc(void);
unsigned char 	get_hours_lct(void);
void 			set_date(unsigned char ch_day, unsigned char ch_month, unsigned int i_year);
unsigned char 	get_day_utc(void);
unsigned char 	get_day_lct(void);
unsigned char 	get_month_utc(void);
unsigned char 	get_month_lct(void);
unsigned int 	get_year_utc(void);
unsigned int 	get_year_lct(void);
unsigned char 	sys_strcmp(char* pch_string1, char* pch_string2);
unsigned char 	sys_strcpy(char* pch_string1, char* pch_string2);
unsigned char 	sys_num2str(char* string, unsigned long l_number, unsigned char ch_digits);
void 			sys_memcpy(void* data1, void* data2, unsigned char length);
unsigned char 	sys_hex2str(char* string, unsigned long l_number, unsigned char ch_digits);
void 			sys_watchdog(unsigned char action);

#endif /* OVARIO_FRAMEWORK_H_ */
