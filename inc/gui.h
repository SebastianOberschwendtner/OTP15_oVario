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
//struct for infobox message
typedef struct
{
	uint8_t 	message;
	uint8_t		msg_payload;
	uint16_t	lifetime;
}InfoBox_T;

//struct for keys and key function assignment
typedef struct
{
	uint8_t pressed;
	uint8_t show_functions;
	uint8_t function[4];
	char func_name[36]; //8 characters + 1 stop character per key
}Key_T;

enum states
{
	Gui_Initscreen 	= 0,
	Gui_Vario 		= 1,
	Gui_Menu 		= 2,
	Gui_Settings 	= 3,
	Gui_BMS 		= 4,
	Gui_GPS 		= 5,
	Gui_MS5611 		= 6,
	Gui_Datafusion  = 7
};
#define num_states 8

//********commands which can be sent from the gui******
// commands have to be defined in fkt_set_ipc_command() and in fkt_get_cmd_string()
// the commands are assigned to the key in fkt_assign_key_function()
#define gui_cmd_nextmenu		1
#define gui_cmd_startmenu		2
#define gui_cmd_stopigc			3
#define gui_cmd_otgon			4
#define gui_cmd_otgoff			5
#define gui_cmd_togglesinktone	6
#define gui_cmd_togglebottombar 7
#define gui_cmd_startigc		8
#define gui_cmd_resetcapacity	9


// Functions
void gui_init					(void);
void gui_task 					(void);

void fkt_Initscreen 			(void);
void fkt_Vario 					(void);
void fkt_BMS					(void);
void fkt_GPS					(void);
void fkt_Menu 					(void);
void fkt_Settings				(void);
void fkt_MS5611 				(void);
void fkt_Datafusion				(void);
void fkt_eval_ipc				(void);
void fkt_assign_key_function	(void);
void fkt_eval_keys				(void);
void fkt_set_ipc_command		(uint8_t command_number);
char* fkt_get_cmd_string		(uint8_t command_number);
void fkt_display_key_functions	(void);
void fkt_runtime_errors			(void);
void fkt_infobox				(void);
void gui_bootlogo				(void);
void gui_status_bar				(void);


void gui_gauge      (float value, float min, float max);
void draw_graph		(uint8_t x, uint8_t y);

#endif /* GUI_H_ */
