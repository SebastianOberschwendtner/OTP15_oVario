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
int32_t pressure = 0;
uint64_t T2;
uint64_t Off2;
uint64_t Sens2;
int32_t calib_pressure = 0;
int32_t ipc_pressure = 1;
int64_t Off, Sens;

ms5611_T* msdata;

TASK_T task_ms5611;			//Task struct for ms6511-task
T_command rxcmd_ms5611;		//Command struct to receive ipc commands
T_command txcmd_ms5611;		//Command struct to send ipc commands



// ***** Functions *****
//inline first!
/*
 * Get the return value of the last finished ipc task which was called.
 */
inline unsigned long ms5611_get_call_return(void)
{
    return rxcmd_ms5611.data;
};

/*
 * Calculate the pressure after the raw data was read from the sensor.
 * D1 has to be read from the sensor!
 */
inline unsigned long ms5611_convert_pressure(void)
{
	Off = (long long)C2 * 65536 + ((long long)C4 * dT) / 128;
	Sens = (long long)C1 * 32768 + ((long long)C3 * dT) / 256;

	if (Temp < 2000)
	{
		T2 = ((uint64_t)dT) >> 31;
		Off2 = (5 * (Temp - 2000) * (Temp - 2000)) >> 1;
		Sens2 = (5 * (Temp - 2000) * (Temp - 2000)) >> 2;

		if (Temp < -1500)
		{
			Off2 = Off2 + (7 * (Temp + 1500) * (Temp + 1500));
			Sens2 = Sens2 + ((11 * (Temp + 1500) * (Temp + 1500)) >> 1);
		}
	}
	else
	{
		T2 = 0;
		Off2 = 0;
		Sens2 = 0;
	}

	Temp = Temp - T2;
	Off = Off - Off2;
	Sens = Sens - Sens2;

	pressure = (D1 * Sens / 2097152 - Off) / 32768;
	return pressure;
};

/*
 * Register everything relevant for IPC
 */
void ms5611_register_ipc(void)
{
	//register memory
	msdata = ipc_memory_register(sizeof(ms5611_T), did_MS5611);
	//Command queue
	ipc_register_queue(5*sizeof(T_command),did_MS5611);

	//Initialize task struct
    arbiter_clear_task(&task_ms5611);
    arbiter_set_command(&task_ms5611, BARO_CMD_INIT);

    //Initialize receive command struct
    rxcmd_ms5611.did           = did_MS5611;
    rxcmd_ms5611.cmd           = 0;
    rxcmd_ms5611.data          = 0;
    rxcmd_ms5611.timestamp     = 0;
};

// /*
//  * Get everything relevant for IPC
//  */
// void ms5611_get_ipc(void)
// {
// 	// get the ipc pointer addresses for the needed data
// 	p_ipc_sys_sd_data = ipc_memory_get(did_SDIO);
// 	p_ipc_sys_bms_data = ipc_memory_get(did_BMS);
// };

/***********************************************************
 * TASK MS5611
 ***********************************************************
 * 
 * 
 ***********************************************************
 * Execution:	interruptable
 * Wait: 		Yes
 * Halt: 		Yes
 **********************************************************/
void ms5611_task(void)
{
	//Check commands in queue
    ms5611_check_commands();

    //When the task wants to wait
    if(task_ms5611.wait_counter)
        task_ms5611.wait_counter--; //Decrease the wait counter
    else    //Execute task when wait is over
    {
        //Perform command action when the task does not wait for other tasks to finish
        if (task_ms5611.halt_task == 0)
        {
            //Perform action according to active state
            switch (arbiter_get_command(&task_ms5611))
            {
            case CMD_IDLE:
				ms5611_idle();
				break;

			case BARO_CMD_INIT:
				ms5611_init();
				break;

			case BARO_CMD_GET_TEMPERATURE:
				ms5611_get_temperature();
				break;

			case BARO_CMD_GET_PRESSURE:
				ms5611_get_pressure();
				break;

			default:
				break;
			}
		}
	}

	//Check for errors here
};

/*
 * Check the commands in the igc queue and call the corresponding command
 */
void ms5611_check_commands(void)
{
    //Check commands
    if (ipc_get_queue_bytes(did_MS5611) >= sizeof(T_command)) // look for new command in keypad queue
    {
		ipc_queue_get(did_MS5611, sizeof(T_command), &rxcmd_ms5611); // get new command
		switch (rxcmd_ms5611.cmd)									 // switch for command
		{
		case cmd_ipc_signal_finished:
			//Called task is finished
			task_ms5611.halt_task -= rxcmd_ms5611.did;
			break;
		
		case BARO_CMD_GET_PRESSURE:
			//Get the pressure
			arbiter_set_command(&task_ms5611, BARO_CMD_GET_PRESSURE);
			break;

		default:
			break;
		}
	}
};

/**********************************************************
 * Idle Command for MS5611
 **********************************************************
 * The Idle command checks for new request to measure
 * the pressure.
 * 
 * Argument:	none
 * Return:		none
 * 
 * Should/Can not be called directly via the arbiter.
 **********************************************************/
void ms5611_idle(void)
{
	//  //Check commands
    // if (ipc_get_queue_bytes(did_MS5611) >= sizeof(T_command)) // look for new command in keypad queue
    // {
	// 	ipc_queue_get(did_MS5611, sizeof(T_command), &rxcmd_ms5611); // get new command
	// 	switch (rxcmd_ms5611.cmd)									 // switch for command
	// 	{
	// 	case BARO_CMD_GET_PRESSURE:
	// 		//Get the pressure
	// 		arbiter_callbyreference(&task_ms5611, BARO_CMD_GET_PRESSURE,0);
	// 		break;

	// 	default:
	// 		break;
	// 	}
	// }
};

/**********************************************************
 * Initialize the MS5611
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * call-by-reference
 **********************************************************/
void ms5611_init(void)
{
	//Perform the command action
	switch(arbiter_get_sequence(&task_ms5611))
	{
		case SEQUENCE_ENTRY:
			//Reset the sensor
			ms5611_call_task(I2C_CMD_SEND_CHAR,0x1E,did_I2C);
			//Wait for sensor reset, 5ms
			task_ms5611.wait_counter = MS2TASKTICK(10,LOOP_TIME_TASK_MS6511);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_C0);
			break;

		case BARO_SEQUENCE_GET_C0:
			//Read the first calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xA0, did_I2C);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_C1);
			break;

		case BARO_SEQUENCE_GET_C1:
			//Get the return value
			C0 = ms5611_get_call_return();
			//Get next calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xA2, did_I2C);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_C2);
			break;

		case BARO_SEQUENCE_GET_C2:
			//Get the return value
			C1 = ms5611_get_call_return();
			//Get next calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xA4, did_I2C);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_C3);
			break;

		case BARO_SEQUENCE_GET_C3:
			//Get the return value
			C2 = ms5611_get_call_return();
			//Get next calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xA6, did_I2C);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_C4);
			break;

		case BARO_SEQUENCE_GET_C4:
			//Get the return value
			C3 = ms5611_get_call_return();
			//Get next calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xA8, did_I2C);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_C5);
			break;

		case BARO_SEQUENCE_GET_C5:
			//Get the return value
			C4 = ms5611_get_call_return();
			//Get next calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xAA, did_I2C);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_C6);
			break;

		case BARO_SEQUENCE_GET_C6:
			//Get the return value
			C5 = ms5611_get_call_return();
			//Get next calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xAC, did_I2C);
			arbiter_set_sequence(&task_ms5611,BARO_SEQUENCE_GET_CRC_MS);
			break;

		case BARO_SEQUENCE_GET_CRC_MS:
			//Get the return value
			C6 = ms5611_get_call_return();
			//Get next calibration constant
			ms5611_call_task(I2C_CMD_READ_INT, 0xAE, did_I2C);
			arbiter_set_sequence(&task_ms5611,SEQUENCE_FINISHED);
			break;

		case SEQUENCE_FINISHED:
			//Get the return value
			CRC_MS = ms5611_get_call_return();
			//Exit the command
			arbiter_return(&task_ms5611,1);
			break;
			break;

		default:
			break;
	}
};

/**********************************************************
 * Read Temperature from MS5611
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * call-by-reference
 **********************************************************/
void ms5611_get_temperature(void)
{
	// Perform the command action
	switch (arbiter_get_sequence(&task_ms5611))
	{
	case SEQUENCE_ENTRY:
		//Send the request to read temperature to the sensor
		ms5611_call_task(I2C_CMD_SEND_CHAR, 0x58, did_I2C);
		//Goto next sequence, because the sensor needs some time to sample the temperature
		//-> wait for 10ms before reading the adc of the ms5611
		task_ms5611.wait_counter = MS2TASKTICK(10, LOOP_TIME_TASK_MS6511);
		arbiter_set_sequence(&task_ms5611, BARO_SEQUENCE_READ_TEMP);
		break;

	case BARO_SEQUENCE_READ_TEMP:
		//Request the readout of the temperature
		ms5611_call_task(I2C_CMD_READ_24BIT, 0x00, did_I2C);
		//Goto next state
		arbiter_set_sequence(&task_ms5611, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Get the returned message
		D2 = ms5611_get_call_return();
		//Calculate the temperature
		dT = D2 - ((long)C5 << 8);
		Temp = 2000 + ((long long)dT * C6) / 8388608;
		//Exit the command
		arbiter_return(&task_ms5611, 1);
		break;

	default:
		break;
	}
};

/**********************************************************
 * Read Pressure from MS5611
 **********************************************************
 * 
 * All the data is written to the global variables defined
 * at the top^ and to the ipc memory.
 * 
 * Argument:	none
 * Return:		none
 * 
 * call-by-reference
 **********************************************************/
void ms5611_get_pressure(void)
{
	//Perform the command action
	switch (arbiter_get_sequence(&task_ms5611))
	{
		case SEQUENCE_ENTRY:
			//Read the temperature and goto next sequence when temperature was read
			if(arbiter_callbyreference(&task_ms5611, BARO_CMD_GET_TEMPERATURE, 0))
				arbiter_set_sequence(&task_ms5611, BARO_SEQUENCE_READ_TEMP);
			break;

		case BARO_SEQUENCE_READ_TEMP:
			//Send the request to read pressure to the sensor
			ms5611_call_task(I2C_CMD_SEND_CHAR, 0x48, did_I2C);
			//Goto next sequence, because the sensor needs some time to sample the pressure
			//-> wait for 10ms before reading the adc of the ms5611
			task_ms5611.wait_counter = MS2TASKTICK(10, LOOP_TIME_TASK_MS6511);
			arbiter_set_sequence(&task_ms5611, BARO_SEQUENCE_READ_PRES);
			break;

		case BARO_SEQUENCE_READ_PRES:
			//Request the readout of the pressure
			ms5611_call_task(I2C_CMD_READ_24BIT, 0x00, did_I2C);
			//Goto next state
			arbiter_set_sequence(&task_ms5611, SEQUENCE_FINISHED);
			break;

		case SEQUENCE_FINISHED:
			//Get the returned message
			D1 = ms5611_get_call_return();

			//Calculate the pressure
			pressure = ms5611_convert_pressure();

			//Update the IPC memory
			msdata->pressure = pressure;
			msdata->temperature = Temp;
			msdata->timestamp = TIM5->CNT;
			msdata->Off = Off;
			msdata->Off2 = Off2;
			msdata->Sens = Sens;
			msdata->Sens2 = Sens2;

			//Exit the command
			arbiter_return(&task_ms5611, 1);
			break;

		default:
			break;
	}
};

/*
 * Call a other task via the ipc queue.
 */
void ms5611_call_task(unsigned char cmd, unsigned long data, unsigned char did_target)
{
    //Set the command and data for the target task
    txcmd_ms5611.did = did_MS5611;
    txcmd_ms5611.cmd = cmd;
    txcmd_ms5611.data = data;

    //Push the command
    ipc_queue_push(&txcmd_ms5611, sizeof(T_command), did_target);

    //Set wait counter to wait for called task to finish
    task_ms5611.halt_task += did_target;
};


// function to read temperature
// int32_t get_temp_MS()
// {
// 	catch_temp_MS();
// 	int32_t Temp_temp = Temp;
// 	return Temp_temp;
// }
#endif /* MS5611_C_ */
