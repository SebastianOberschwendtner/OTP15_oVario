/*
 * DOGXL240.h
 *
 *  Created on: 28.02.2018
 *      Author: Sebastian
 */

#ifndef DOGXL240_H_
#define DOGXL240_H_

#include "spi.h"

/*
 * Defines for registers and commands
 */
#define COMMAND				0
#define DATA				1

#define SET_COL_LSB			0b00000000
#define SET_COL_MSB			0b00010000
#define SET_PAGE_LSB		0b01100000
#define SET_PAGE_MSB		0b01110000
#define TEMP_COMP			0b00100100
#define SET_PAN_LOAD		0b00101000
#define SET_POTI			0b10000001
#define SET_DISP_EN			0b10101000
#define SET_LCD_MAP			0b11000000
#define SET_LCD_BIAS		0b11101000
#define SET_COM_END			0b11110001
#define SET_PART_DISP_START	0b11110010
#define SET_PART_DISP_END	0b11110011


void init_lcd(void);
void lcd_set_cd(unsigned char ch_state);


#endif /* DOGXL240_H_ */
