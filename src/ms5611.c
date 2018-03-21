/*
 * ms5611.c
 *
 *  Created on: 16.03.2018
 *      Author: Admin
 */

#ifndef MS5611_C_
#define MS5611_C_

#include "MS5611.h"

// ***** Variables *****
uint32_t C0,C1,C2,C3,C4,C5,C6,CRC_MS,D1,D2;
int32_t dT, Temp;
uint64_t T2;
uint64_t Off2;
uint64_t Sens2;
int32_t calib_pressure = 0;
int32_t ipc_pressure = 1;

ms5611_T msdata;

int32_t pressure_old = 1;
int32_t pressure_new = 1;
float ps = 102500;
float pressure_old_filt = 1;
float pressure_new_filt = 1;

#define climbavtime 15
float timeclimbarray[climbavtime * 10] = {0};
uint8_t timecnt = 0;
uint8_t timeidx = 0;

float dh = 0;

float Time = 0;
float u 	= 0;
float y 	= 0;
float T  	= 1;
float ts 	= 0.02;
float d 	= 0.9;

float ui1 = 0;
float ui2 = 0;
float yi1 = 0;
float yi2 = 0;
float sub = 0;

uint8_t flagfirst = 1;

uint8_t test = 0;

// ***** Functions *****

void MS5611_init()
{
	// Perform Reset
	wait_systick(5);
	i2c_send_char(i2c_addr_MS5611, 0x1E);

	wait_systick(2);

	// PROM Read
	C0 =		i2c_read_int(i2c_addr_MS5611,0xA0);							// Load constants for calculation
	C1 =		i2c_read_int(i2c_addr_MS5611,0xA2);
	C2 =		i2c_read_int(i2c_addr_MS5611,0xA4);
	C3 =		i2c_read_int(i2c_addr_MS5611,0xA6);
	C4 =		i2c_read_int(i2c_addr_MS5611,0xA8);
	C5 =		i2c_read_int(i2c_addr_MS5611,0xAA);
	C6 =		i2c_read_int(i2c_addr_MS5611,0xAC);
	CRC_MS =	i2c_read_int(i2c_addr_MS5611,0xAE);

	wait_systick(1);

	msdata.climbrate 		= 0;
	msdata.climbrate_filt 	= 0;
	msdata.hoehe			= 0;
	msdata.pressure			= 0;

	ipc_memory_register(44,did_MS5611);
}

// function to read & calculate pressure (read temperature before!)
int32_t get_pressure_MS()
{
	catch_temp_MS();
	// Convert D1 OSR = 4096
	//spi_write_pinlow(0x48);
	i2c_send_char(i2c_addr_MS5611, 0x48);

	//delay_ms(9);
	wait_ms(10ul);

	// ADC Read
	uint32_t D1 = i2c_read_24bit(i2c_addr_MS5611,0x00);
	int64_t Off = (long long)C2 * 65536 + ((long long)C4 * dT ) / 128;
	int64_t Sens = (long long)C1 * 32768 + ((long long)C3 * dT) / 256;

	if (Temp < 2000)
	{
		uint64_t T2 = ((uint64_t) dT )>> 31;
		uint64_t Off2 = (5 * (Temp - 2000)^2) >> 1;
		uint64_t Sens2 = (5 * (Temp - 2000)) >> 2;

		if (Temp < -15)
		{
			Off2 = Off2 + (7 * (Temp + 1500)^2);
			Sens2 = Sens2 + ((11 * (Temp + 1500)^2) >> 1);
		}
	}
	else
	{
		uint64_t T2 = 0;
		uint64_t Off2 = 0;
		uint64_t Sens2 = 0;
	}
	Temp 	= Temp - T2;
	Off 	= Off - Off2;
	Sens 	= Sens - Sens2;

	int32_t pressure 	= (D1*Sens/2097152 - Off)/32768;
	ipc_pressure 		= pressure;
	msdata.pressure 	= pressure;


	// calc h
	float p = (float)msdata.pressure;
	float x = p / ps;
	//msdata.height 		= 0.004109 * p * p - 14.8 * p + 10860 ;
	msdata.height = 5331.2 * x * x - 18732 * x + 13414;
	// PT2D Filter
	u = msdata.height;

	// Init Filter
	if (flagfirst)
	{
		// Init Filter
		yi1 = u;
		ui1 = u * ts;
		yi2 = -2 * d * T * u;

		// Init Averager
		for(uint8_t cntx = 0; cntx < (climbavtime * 10); cntx ++)
		{
			timeclimbarray[cntx] = msdata.height;
		}

		flagfirst = 0;
	}

	sub = ui1;
	ui1 += u * ts - sub;
	yi2 -= sub;

	y = (ui1 - 2 * d * T * yi1 - yi2)/(T * T);

	yi1 += y * ts;
	yi2 += yi1 * ts;

	Time += ts;
	// Debug
	msdata.climbrate_filt 	= y;
	msdata.ui1 = ui1;
	msdata.yi1 = yi1;
	msdata.yi2 = yi2;
	msdata.Time = Time;
	msdata.sub	= sub;
	msdata.pressure = (float)pressure;

	test = (uint8_t)((y+5)*10);

	// TimeClimb
	timecnt++;
	if (timecnt == 5)
	{
		timeclimbarray[timeidx] = msdata.height;
		msdata.climbrate_av = (timeclimbarray[timeidx] - timeclimbarray[(timeidx + 1) % (climbavtime * 10)]) / climbavtime;
		timeidx++;
		if (timeidx == (climbavtime * 10))
		{
			timeidx = 0;
		}
		timecnt = 0;
	}



	return pressure;
}


// function to read temperature
void catch_temp_MS()
{
	// Convert D2 OSR = 4096
	i2c_send_char(i2c_addr_MS5611, 0x58);
	//delay_ms(9);
	wait_ms(10ul);
	i2c_send_char(i2c_addr_MS5611, 0x58);
	// Read ADC
	D2 = i2c_read_24bit(i2c_addr_MS5611,0x00);
	dT = D2-((long)C5<<8);
	Temp = 2000 + ((long long)dT * C6)/8388608;
}


// function to read temperature
int32_t get_temp_MS()
{
	catch_temp_MS();
	int32_t Temp_temp = Temp;
	return Temp_temp;
}
#endif /* MS5611_C_ */
