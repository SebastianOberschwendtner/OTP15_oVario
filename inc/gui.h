/*
 * gui.h
 *
 *  Created on: 10.03.2018
 *      Author: Admin
 */

#ifndef GUI_H_
#define GUI_H_

#include "stm32f4xx.h"

//***** Defines*****
enum states
{
	Gui_Initscreen 	= 0,
	Gui_Vario 		= 1,
	Gui_Menu 		= 2,
	Gui_Settings 	= 3,
	Gui_BMS 		= 4,
	Gui_GPS 		= 5,
	GUI_MS5611 		= 6
};
#define num_states 7


// Functions
void gui_init			(void);
void gui_task 			(void);

void fkt_Initscreen 	(void);
void fkt_Vario 			(void);
void fkt_BMS			(void);
void fkt_GPS			(void);
void fkt_Menu 			(void);
void fkt_Settings		(void);
void fkt_MS5611 		(void);
void fkt_runtime_errors	(void);
void fkt_infobox		(void);


void gui_gauge      (float value, float min, float max);
void draw_graph		(uint8_t x, uint8_t y);

#endif /* GUI_H_ */
