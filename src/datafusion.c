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
#include "math.h"
#include "arm_math.h"
#include "logging.h"

// ***** Variables *****
ms5611_T* ipc_df_data;
datafusion_T* df_data;
GPS_T* ipc_df_gps_data;



// ***** Internal Functions *****
void wind_estimator (void);


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

uint8_t flagfirst = 1;
uint8_t timecnt = 0;
uint8_t tcnt = 0;

// ***** Functions *****

void datafusion_init(void)
{
	ipc_df_data 	= ipc_memory_get(did_MS5611);
	df_data			= ipc_memory_register(121 + 201 + 48 + 17 + 122,did_DATAFUSION);
	ipc_df_gps_data = ipc_memory_get(did_GPS);


	char log_baro[] = {"bar"};
	log_include(&df_data->climbrate_filt, 4 ,1, &log_baro[0]);
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
		yi1 = 0;
		ui1 = 0;//u * ts;
		yi2 = u * ts;

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
		df_data->glide = ipc_df_gps_data->speed_kmh / 3.6 / (-y);
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


	wind_estimator ();
}

uint8_t 	data_cnt = 0; 			// how many valid data points in a row
float 		psi[128]; 				// heading
float 		vgps[128];				// gps speed
uint8_t 	data_ptr = 0; 			// data pointer to data array "psi"
int8_t 		dir_turn = 0;			// turning direction: 1 right; -1 left
int8_t 		dir_last = 0;			// turning direction of last data point
float 		hdg_last = 0;           // heading of last data point
float 		hdg = 0; 				// heading of actual data point

void wind_estimator ()
{
//	for (uint8_t cnta = 0; cnta < 60; cnta++)
//	{
//		psi[cnta] = (float)5 * cnta;
//		vgps[cnta] = 10;
//	}
//	data_cnt = 60;

	wind_collect_samples();
	wind_grad();
}




// nur wenn v > 10kmh?

void wind_collect_samples(void)
{
	if(ipc_df_gps_data->fix == 3)
	{
		hdg = ipc_df_gps_data->heading_deg;

		if(df_min(df_abs(hdg - hdg_last), 360 - df_abs(hdg - hdg_last)) > 10) // check for 5 degrees away
		{
			// get measurement
			dir_turn = wind_turn_dir(hdg_last, hdg);

			// determine same direction as last measurement and safe hdg if so
			if(1)//dir_turn == dir_last) 					// yeah flying same direction
			{
				psi[data_ptr]	= hdg;
				vgps[data_ptr]	= ipc_df_gps_data->speed_kmh / 3.6;

				data_cnt++;
				if(data_cnt > 60)//127)
					data_cnt = 60;

				data_ptr++;
				if(data_ptr > 60)// 127)
					data_ptr = 0;
			}
			else 	//damnit flying something -> reset everything
			{
				data_cnt = 0;
				data_ptr = 0;
				dir_last = dir_turn;
			}
			hdg_last = hdg;
		}
	}
	df_data->Wind.cnt = data_cnt;
}


int8_t wind_turn_dir (float hdg_last, float hdg)	// calc turning direction
{
	float df_temp = hdg_last - hdg;

	int8_t dir_temp = 0;
	if(df_temp > 0)
		dir_temp = -1;

	if(df_temp < 0)
		dir_temp = 1;

	if(df_temp > 180)
		dir_temp = 1;

	if(df_temp < -180)
		dir_temp = -1;

	return dir_temp;
}

float wgx0 = 0;
float wgy0 = 0;
float R0 = 8;

void wind_grad(void)	// calc
{
	if(data_cnt > 10) // Only calc if more than 10 datapoints
	{
		// Local variables
		volatile float J 		= 0;

		volatile float x1 		= wgx0 + 0.1;
		volatile float y1 		= wgy0 + 0.1;
		volatile float R1 		= R0 + 0.1;

		volatile float x 		= 2;
		volatile float y 		= 1;

		volatile float dJx 		= 0;
		volatile float dJy 		= 0;
		volatile float dJR 		= 0;

		volatile float wetemp 	= 0;

		volatile float h 		= 0.001;
		volatile float r_dJ 	= 0;
		volatile float s 		= 0;
		volatile float J_old 	= 0;

		volatile float dH 		= 1;

		volatile uint16_t itcnt = 0;

		// Iteration
		while((dH > 0.0001)&&(itcnt < 100))
		{
			itcnt++;
			J = 0;
			for(uint8_t cnt = 0; cnt < data_cnt; cnt++)
			{
				// Convert to kartesic coordinates
				x = sin(psi[cnt] / 180 * 3.14) * vgps[cnt];
				y = cos(psi[cnt] / 180 * 3.14) * vgps[cnt];

				df_data->Wind.WE_x[cnt] = (int8_t)(x * 5);
				df_data->Wind.WE_y[cnt] = (int8_t)(y * 5);


				// Calc Cost and Cost derivative
				wetemp = df_sqr(x-wgx0) + df_sqr(y-wgy0) - df_sqr(R0);

				J += df_sqr(wetemp);

				dJx += -4 * (wetemp) * (x-wgx0);
				dJy += -4 * (wetemp) * (y-wgy0);
				dJR += -4 * (wetemp) * R0;
			}

			// Calc step width
			arm_sqrt_f32(df_sqr(wgx0 - x1) + df_sqr(wgy0-y1) + df_sqr(R0-R1),&h);

			// Normal the derivatives
			arm_sqrt_f32(df_sqr(dJx) + df_sqr(dJy) + df_sqr(dJR), &r_dJ);

			dJx /= r_dJ;
			dJy /= r_dJ;
			dJR /= r_dJ;

			// Save old x
			dH = df_abs(R0 - R1);
			x1 = wgx0;
			y1 = wgy0;
			R1 = R0;

			// If Cost gets bigger -> half step width
			if(J > J_old)
				s +=1;

			J_old = J;

			// Calc new x
			wgx0 = wgx0 - dJx * h / s;
			wgy0 = wgy0 - dJy * h / s;
			R0 = R0 - dJR * h / s;
		}


		// Calc Wind
		float wind, winddir;

		// Calc Wind Speed
		arm_sqrt_f32(df_sqr(wgx0) + df_sqr(wgy0),&wind);

		// Calc wind direction
		winddir = asin(wgx0/wind) / 3.14 * 180;

		if(winddir > 0)
		{
			if(wgy0 < 0)
				winddir +=90;
		}
		else
		{
			if(wgy0 > 0)
				winddir = 360 + winddir;
			else
				winddir = 270 + winddir;
		}

		// Write to ipc data
		df_data->Wind.W_mag 		= wind;
		df_data->Wind.W_dir 		= winddir;
		df_data->Wind.W_Va 			= R0;
		df_data->Wind.iterations 	= itcnt;
	}
}

float df_sqr(float x)
{
	return x * x;
}

float df_abs(float x)
{
	if(x > 0)
		return x;
	else
		return -x;
}

float df_min(float x, float y)
{
	if(x > y)
		return y;
	else
		return x;
}
