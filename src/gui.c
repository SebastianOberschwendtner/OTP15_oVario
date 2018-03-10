/*
 * gui.c
 *
 *  Created on: 10.03.2018
 *      Author: Admin
 */


//*********** Includes **************
#include "gui.h"

//*********** Variables **************
uint8_t menu = 0;
uint8_t submenu = 0;

//*********** Functions **************
void set_gui (void)
{
	switch(menu)
	{
	case Gui_Initscreen:
		fkt_Initscreen();
		break;

	case Gui_Vario:
		fkt_Vario();
		break;

	case Gui_Menu:
		fkt_Menu();
		break;

	case Gui_Settings:
		fkt_Settings();
		break;

	default:
		break;
	}

	// draw screen
}

void fkt_Initscreen (void)
{
	;
}

void fkt_Vario (void)
{
	;
}

void fkt_Menu (void)
{
	;
}

void fkt_Settings(void)
{
	;
}

