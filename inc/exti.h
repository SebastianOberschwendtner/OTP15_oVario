/*
 * exti.h
 *
 *  Created on: 12.03.2018
 *      Author: Admin
 */

#ifndef EXTI_H_
#define EXTI_H_

// ***** Includes ****
#include "stm32f4xx.h"

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "misc.h"
#include "did.h"
#include "ipc.h"

// ***** Prototypes *****

#pragma pack(push, 1)
typedef struct
{
	uint8_t	 pad;
	uint32_t timestamp;
}T_keypad;
#pragma pack(pop)
// ***** Functions *****
void exti_init(void);



#endif /* EXTI_H_ */
