/*
 * datafusion.c
 *
 *  Created on: 21.03.2018
 *      Author: Admin
 */

// ***** Includes ******
#include "datafusion.h"
#include "Variables.h"
#include "ipc.h"
#include "oVario_Framework.h"

// ***** Variables *****
ms5611_T* ipc_df_data;
datafusion_T* df_data;
GPS_T* ipc_df_gps_data;

float ps = 102500;

float dh = 0;

float Time = 0;
float u 	= 0;
float y 	= 0;
float T  	= 1;
float ts 	= (float)SYSTICK / 1000;
float d 	= 0.9;

float ui1 = 0;
float ui2 = 0;
float yi1 = 0;
float yi2 = 0;
float sub = 0;

float timeclimbarray[climbavtime * 2] = {0};
float timeclimbh[2];
float timeclimbav = 0;

uint8_t flagfirst = 1;
uint8_t timecnt = 0;
uint8_t tcnt = 0;

// ***** Functions *****

void datafusion_init(void)
{
	ipc_df_data = ipc_memory_get(did_MS5611);
	df_data		= ipc_memory_register(121 + 201 + 48,did_DATAFUSION);
	ipc_df_gps_data = ipc_memory_get(did_GPS);
}


void datafusion_task(void)
{

	// calc h
	float p = (float)ipc_df_data->pressure;
	float x = p / ps;

	//data.height
	df_data->height = 5331.2 * x * x - 18732 * x + 13414;

	// ***** PT2D Filter *****
	u = df_data->height;

	// Init Filter
	if (flagfirst)
	{
		// Init Filter
		yi1 = u;
		ui1 = u * ts;
		yi2 = -2 * d * T * u;

		// Init Averager
		/*for(uint8_t cntx = 0; cntx < (climbavtime * 10); cntx ++)
		{
			timeclimbarray[cntx] = df_data->height;

		}*/
		timeclimbh[0] = df_data->height;


		flagfirst = 0;
	}

	sub = ui1;
	ui1 += u * ts - sub;
	yi2 -= sub;

	y = (ui1 - 2 * d * T * yi1 - yi2) / (T * T);

	yi1 += y * ts;
	yi2 += yi1 * ts;
	Time += ts;

	// Debug
	df_data->climbrate_filt 	= y;
	df_data->ui1 				= ui1;
	df_data->yi1 				= yi1;
	df_data->yi2 				= yi2;
	df_data->Time 				= Time;
	df_data->sub				= sub;
	df_data->pressure 			= p;


	if(y < 0)
	{
		df_data->glide = ipc_df_gps_data->speed_kmh/3.6/(-y);
	}
	else
		df_data->glide = 99;


	if(tcnt > 4)		// running @ 10Hz, happening every 0.5s
	{
		// climb history
		df_data->hist_clib[df_data->hist_ptr] = df_data->climbrate_filt;
		df_data->hist_ptr = (df_data->hist_ptr + 1) % 50;

		// height history
		df_data->hist_h[df_data->histh_ptr] = df_data->height;

		df_data->climbrate_av = (df_data->height - df_data->hist_h[(df_data->hist_ptr + 1) % 30])/15;

		df_data->histh_ptr = (df_data->histh_ptr + 1) % 30;


		tcnt = 0;
	}
	tcnt++;
}
