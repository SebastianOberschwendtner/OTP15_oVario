/*
 * datafusion.h
 *
 *  Created on: 21.03.2018
 *      Author: Admin
 */

#ifndef DATAFUSION_H_
#define DATAFUSION_H_

#include "stm32f4xx.h"

// ***** Defines *****
#define climbavtime 15

// ***** Functions *****
void datafusion_init(void);
void datafusion_task(void);



#endif /* DATAFUSION_H_ */
