/*
 * ms5611.c
 *
 *  Created on: 16.03.2018
 *      Author: Admin
 */

#ifndef MS5611_C_
#define MS5611_C_

#include "MS5611.h"
#include "Variables.h"

// ***** Variables *****
uint32_t C0,C1,C2,C3,C4,C5,C6,CRC_MS,D1,D2;
int32_t dT, Temp;
int32_t pressure = 0;
uint64_t T2;
uint64_t Off2;
uint64_t Sens2;
int32_t calib_pressure = 0;
int32_t ipc_pressure = 1;
int64_t Off, Sens;

ms5611_T* msdata;

extern unsigned long error_var;



// ***** Functions *****

void MS5611_init()
{
	i2c_reset_error();
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
	wait_ms(100ul);

	msdata = ipc_memory_register(sizeof(ms5611_T), did_MS5611);
	//Check communication status
	if(i2c_get_error())
	{
		msdata->com_err = (unsigned char)0xFFFF;
		error_var |= err_baro_fault;
		i2c_reset_error();
	}
}

// Task
void ms5611_task()
{
	msdata->pressure 	= get_pressure_MS();
	msdata->temperature	= Temp;
	msdata->timestamp 	= TIM5->CNT;

	msdata->Off 	= Off;
	msdata->Off2 	= Off2;
	msdata->Sens	= Sens;
	msdata->Sens2 	= Sens2;
}


// function to read & calculate pressure
int32_t get_pressure_MS()
{
	catch_temp_MS();
	//If no communication error occurred
	if(!(msdata->com_err & SENSOR_ERROR))
	{
		//Reset i2c error
		i2c_reset_error();
		// Convert D1 OSR = 4096
		i2c_send_char(i2c_addr_MS5611, 0x48);

		wait_ms(10ul);

		// ADC Read
		D1 = (int32_t)i2c_read_24bit(i2c_addr_MS5611,0x00);

		// ADC Read

		Off 	= (long long)C2 * 65536 + ((long long)C4 * dT ) / 128;
		Sens 	= (long long)C1 * 32768 + ((long long)C3 * dT) / 256;

		if (Temp < 2000)
		{
			T2 		= ((uint64_t) dT )>> 31;
			Off2 	= (5 * (Temp - 2000) * (Temp - 2000)) >> 1;
			Sens2 	= (5 * (Temp - 2000) * (Temp - 2000)) >> 2;

			if (Temp < -1500)
			{
				Off2 	= Off2 + (7 * (Temp + 1500) * (Temp + 1500));
				Sens2 	= Sens2 + ((11 * (Temp + 1500) * (Temp + 1500)) >> 1);
			}
		}
		else
		{
			T2 		= 0;
			Off2 	= 0;
			Sens2 	= 0;
		}

		Temp 	= Temp - T2;
		Off 	= Off - Off2;
		Sens 	= Sens - Sens2;

		pressure 	= (D1 * Sens / 2097152 - Off)/32768;
		return pressure;

		//check communication error
		msdata->com_err += i2c_get_error();
	}
	else
	{
		return 0;
		error_var |= err_baro_fault;
	}
}


// function to read temperature
void catch_temp_MS()
{
	//If no communication error occurred
	if(!(msdata->com_err & SENSOR_ERROR))
	{
		//Reset i2c error
		i2c_reset_error();
		// Convert D2 OSR = 4096
		i2c_send_char(i2c_addr_MS5611, 0x58);
		wait_ms(10ul);

		// Read ADC
		D2 = (int32_t)i2c_read_24bit(i2c_addr_MS5611,0x00);

		dT = D2-((long)C5<<8);
		Temp = 2000 + ((long long)dT * C6)/8388608;
		//check communication error
		msdata->com_err += i2c_get_error();
	}
	else
	{
		error_var |= err_baro_fault;
	}
}


// function to read temperature
int32_t get_temp_MS()
{
	catch_temp_MS();
	int32_t Temp_temp = Temp;
	return Temp_temp;
}
#endif /* MS5611_C_ */
