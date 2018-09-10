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
#include "arm_math.h"

//Git ist scheiße!

float temp_climb;

datafusion_T* p_ipc_v_df_data;
T_command vario_command;

uint8_t  volu 	= 0;
uint16_t freque = 0;
uint8_t  peri 	= 0;
uint8_t state_sinktone = 1;



// ***** Functions *****
void vario_init(void)
{
	p_ipc_v_df_data = ipc_memory_get(did_DATAFUSION);
	ipc_register_queue(200, did_VARIO);
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

	// Handle Queue Commands
		T_command IPC_cmd;
		while(ipc_get_queue_bytes(did_VARIO))
		{
			ipc_queue_get(did_VARIO, 10, &IPC_cmd); 	// get new command
			switch(IPC_cmd.cmd)					// switch for pad number
			{
			case cmd_vario_set_sinktone:
				state_sinktone = 1;
				break;

			case cmd_vario_clear_sinktone:
				state_sinktone = 0;
				break;

			case cmd_vario_toggle_sinktone:
				state_sinktone ^= 1;
				break;

			default:
				break;
			}
		}

	// Sound creation
	temp_climb = p_ipc_v_df_data->climbrate_filt;

	if(temp_climb > 5)
		temp_climb = 5;

	if(temp_climb < -4)
		temp_climb = -4;



	if(temp_climb > 0.2)		// Climbing
	{
		// Set Unmute
		vario_command.cmd 	= cmd_sound_set_unmute;
		vario_command.data		= 0;

		ipc_queue_push(&vario_command, 10, did_SOUND);

		// Set Beep
		vario_command.cmd 	= cmd_sound_set_beep;
		vario_command.data		= 0;

		ipc_queue_push(&vario_command, 10, did_SOUND);


		// Set Frequency
		freque = (uint16_t)((temp_climb)/5*1500 + 300);
		volu = 50;

		vario_command.cmd 	= cmd_sound_set_frequ;
		vario_command.data		= (uint32_t)freque;

		ipc_queue_push(&vario_command, 10, did_SOUND);


		// Set Period
		if(temp_climb < 1)
			temp_climb = 1;
		peri = (uint8_t)(100 - (temp_climb/5*80));

		vario_command.cmd 	= cmd_sound_set_period;
		vario_command.data	= (uint32_t)peri;

		ipc_queue_push(&vario_command, 10, did_SOUND);
	}
	else if((temp_climb < -2) && state_sinktone) 		// Huge Sink  --> Turn on Thermals
	{
		// Set Unmute
		vario_command.cmd 	= cmd_sound_set_unmute;
		vario_command.data	= 0;

		ipc_queue_push(&vario_command, 10, did_SOUND);

		// Set Continous
		vario_command.cmd 	= cmd_sound_set_cont;
		vario_command.data	= 0;

		ipc_queue_push(&vario_command, 10, did_SOUND);


		// Set Frequency
		freque = (uint16_t)((temp_climb + 2) * 50 + 350);
		volu = 100;

		vario_command.cmd 	= cmd_sound_set_frequ;
		vario_command.data	= (uint32_t)freque;

		ipc_queue_push(&vario_command, 10, did_SOUND);

	}
	else
	{
		vario_command.cmd 	= cmd_sound_set_mute;
		vario_command.data	= 0;
		ipc_queue_push(&vario_command, 10, did_SOUND);
	}





}
// Function to calculate an estimate for the distance between two points
float vario_wgs84_distance(float lat1, float lon1, float lat2, float lon2)
{
	float dist = 0;

	float u_1 = 2 * pi * r_earth * arm_cos_f32(lat1 / 360 * pi);				// earth radius on lat1
	float u_2 = 2 * pi * r_earth * arm_cos_f32(lat2 / 360 * pi);				// earth radius on lat2

	float d_lon_12 = (lon1 - lon2) / 360 * (u_1 + u_2) / 2;			// lon distance between p1 and p2
	float d_lat_12 = 2 * pi * r_earth * (lat1 - lat2) / 360;		// lat distance between p1 and p2


	arm_sqrt_f32((d_lon_12 * d_lon_12 + d_lat_12 * d_lat_12),&dist);	// pythagoras between these distances

	return dist; //[m]
}

