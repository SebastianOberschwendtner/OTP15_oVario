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
	Gui_Initscreen,
	Gui_Vario,
	Gui_Menu,
	Gui_Settings
};

// Functions
void set_gui 		(void);

void fkt_Initscreen (void);
void fkt_Vario 		(void);
void fkt_Menu 		(void);
void fkt_Settings	(void);

#endif /* GUI_H_ */
