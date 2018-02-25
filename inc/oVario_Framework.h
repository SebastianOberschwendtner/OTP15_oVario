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
#define	F_CPU		168000000UL

//PLL variables
#define PLL_M		25
#define PLL_N		336
#define PLL_P		2
#define PLL_Q		7

//Port numbers of GPIOs
#define	A			0
#define	B			1
//#define	C			2
#define	D			3
#define	E			4
#define	F			5
#define	G			6
#define	H			7
#define	I			8



void init_clock(void);
void en_gpio(unsigned char ch_port);
void init_systick_ms(unsigned int i_ticktime);

#endif /* OVARIO_FRAMEWORK_H_ */
