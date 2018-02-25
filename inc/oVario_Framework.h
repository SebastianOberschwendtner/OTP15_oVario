/*
 * oVario_Framework.h
 *
 *  Created on: 25.02.2018
 *      Author: Sebastian
 */

#ifndef OVARIO_FRAMEWORK_H_
#define OVARIO_FRAMEWORK_H_
/*
 * Defines for system
 */
//CPU Speed
#define	F_CPU			168000000UL

//PLL variables
#define PLL_M			25
#define PLL_N			336
#define PLL_P			2
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



void init_clock(void);
void en_gpio(unsigned char ch_port);
void init_systick_ms(unsigned int i_ticktime);

#endif /* OVARIO_FRAMEWORK_H_ */
