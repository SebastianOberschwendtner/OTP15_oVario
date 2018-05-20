/*
 * vario.c
 *
 *  Created on: 06.04.2018
 *      Author: Admin
 */

// ***** Includes *****
#include "vario.h"
#include "sound.h"
#include "Variables.h"




float temp_climb;

datafusion_T* p_ipc_v_df_data;
T_sound_command vario_command;

uint8_t  volu 	= 0;
uint16_t freque = 0;
uint8_t  peri 	= 0;



// ***** Functions *****
void vario_init(void)
{
	p_ipc_v_df_data = ipc_memory_get(did_DATAFUSION);
}



void vario_task(void)
{

	/*
Top:
peri = 20;
Sound = 2000;

Bottom:
peri = 100
sound = 500;

	 */


	temp_climb = p_ipc_v_df_data->climbrate_filt;

	if(temp_climb > 5)
		temp_climb = 5;

	if(temp_climb < -2)
		temp_climb = -2;



	if(temp_climb > 0.2)
	{
		// Set Unmute
		vario_command.command 	= sound_cmd_set_unmute;
		vario_command.data		= 0;

		ipc_queue_push(&vario_command, 5, did_SOUND);

		// Set Beep
		vario_command.command 	= sound_cmd_set_beep;
		vario_command.data		= 0;

		ipc_queue_push(&vario_command, 5, did_SOUND);


		// Set Frequency
		freque = (uint16_t)((temp_climb)/5*1500 + 300);
		volu = 50;

		vario_command.command 	= sound_cmd_set_frequ;
		vario_command.data		= (uint32_t)freque;

		ipc_queue_push(&vario_command, 5, did_SOUND);


		// Set Period
		if(temp_climb < 1)
			temp_climb = 1;
		peri = (uint8_t)(100 - (temp_climb/5*80));

		vario_command.command 	= sound_cmd_set_period;
		vario_command.data		= (uint32_t)peri;

		ipc_queue_push(&vario_command, 5, did_SOUND);
	}
	else if(0)//(temp_climb < 0.3)&&(temp_climb > -1))
	{
		// Set Unmute
		vario_command.command 	= sound_cmd_set_unmute;
		vario_command.data		= 0;

		ipc_queue_push(&vario_command, 5, did_SOUND);


		// Set Continous
		vario_command.command 	= sound_cmd_set_cont;
		vario_command.data		= 0;

		ipc_queue_push(&vario_command, 5, did_SOUND);


		// Set Frequency
		freque = (uint16_t)((temp_climb)/5*1500 + 300);
		volu = 100;

		vario_command.command 	= sound_cmd_set_frequ;
		vario_command.data		= (uint32_t)freque;

		ipc_queue_push(&vario_command, 5, did_SOUND);

		// Set Volume
		vario_command.command 	= sound_cmd_set_vol;
		vario_command.data		= (uint32_t)volu;

		ipc_queue_push(&vario_command, 5, did_SOUND);
	}
	else
	{
		vario_command.command 	= sound_cmd_set_mute;
		vario_command.data		= 0;
		ipc_queue_push(&vario_command, 5, did_SOUND);
	}


}

