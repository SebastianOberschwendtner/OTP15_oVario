/**
  ******************************************************************************
  * @file    main.c
  * @author  SO
  * @version V1.0
  * @date    25-Februar-2018
  * @brief   Default main function.
  ******************************************************************************
*/

#include "oVario_Framework.h"
#include "i2c.h"
			

int main(void)
{
	init_clock();
	init_systick_ms(100);

	for(;;);
}
