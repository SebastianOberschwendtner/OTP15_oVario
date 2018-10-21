/*
 * logging.c
 *
 *  Created on: 01.09.2018
 *      Author: Sebastian
 */
#include "logging.h"
#include "sdio.h"
#include "Variables.h"
#include "ipc.h"

//ipc structs
SDIO_T* sd;
SYS_T* sys;

//private structs
FILE_T Log;
LOG_SENSOR_T sensor[LOG_MAX_SENSORS];
LOG_INFO_T LogInfo;

/*
 * Include sensor value in log stream.
 * The log intervall is not used yet, it is for future features.
 * The sensor name must consist of LOG_SENS_NAME_LENGTH characters!
 */
void log_include(void* pointer, unsigned char length, unsigned char intervall, char* sensor_name)
{
	//Check for available sensors
	if(LogInfo.ch_sensor_count < LOG_MAX_SENSORS)
	{
		sensor[LogInfo.ch_sensor_count].address = (char*)pointer;
		sensor[LogInfo.ch_sensor_count].size = length;
		sensor[LogInfo.ch_sensor_count].intervall = intervall;

		//Add the sensor name to header --> max. LOG_SENS_NAME_LENGTH character!
		for(unsigned char character = 0; character < LOG_SENS_NAME_LENGTH; character++)
			LogInfo.header[LOG_SENS_NAME_LENGTH*LogInfo.ch_sensor_count + character] = *(sensor_name + character);
		LogInfo.header[LOG_SENS_NAME_LENGTH*LogInfo.ch_sensor_count + 3] = 0;	//Terminate string

		LogInfo.ch_sensor_count++;
	}
};

/*
 * create the log file
 */
void log_create(void)
{
	unsigned long l_count = 0;
	char pch_name[9] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00};

	//get sd state
	sd = ipc_memory_get(did_SDIO);
	sys = ipc_memory_get(did_SYS);

	//Only log if sd-card is detected
	if(sd->state & SD_CARD_DETECTED)
	{
		//initialize name
		Log.name[11] = 0;
		sys_strcpy(pch_name,LOG_NAME);

		//create file on sd-card
		sdio_read_root();

		//Check for existing log and create appending numbering
		do
		{
			l_count++;
			sys_num2str(pch_name+LOG_NUMBER_START, l_count, LOG_NUMBER_LENGTH);
		}while(sdio_get_fileid(&Log, pch_name, LOG_EXTENSION));

		//create new log with lognumber in filename
		sdio_mkfile(&Log, pch_name, LOG_EXTENSION);
		LogInfo.ch_log_open = 1;

		/*Write header to log
		 * LOGxxxxx by JoVario - AR
		 * Date: 23.02.2018
		 * Sens: SxxSxxSxx
		 * Byte: 08 08 08
		 */
		sdio_string2file(&Log, pch_name);
		sdio_string2file(&Log, " by JoVario - AR\n");
		sdio_string2file(&Log, "Date: ");
		sdio_num2file(&Log, (unsigned long)get_day_utc(), 2);
		sdio_byte2file(&Log, '.');
		sdio_num2file(&Log, (unsigned long)get_month_utc(), 2);
		sdio_byte2file(&Log, '.');
		sdio_num2file(&Log, (unsigned long)get_year_utc(), 4);
		sdio_byte2file(&Log, '\n');
		sdio_string2file(&Log, "Sens: ");
		sdio_string2file(&Log, LogInfo.header);
		sdio_byte2file(&Log, '\n');
		sdio_string2file(&Log, "Byte: ");
		for(unsigned char count = 0; count < LogInfo.ch_sensor_count; count++)
		{
			sdio_num2file(&Log, sensor[count].size, 2);
			sdio_byte2file(&Log, ' ');
		}
		sdio_byte2file(&Log, '\n');
	}
};

/*
 * Log data
 */
//TODO Add support for error variable.
void log_exe(void)
{
	//Check if sd-card is inserted and log is not finished
	if((sd->state & SD_CARD_DETECTED) && LogInfo.ch_log_open)
	{
		//Log every sensore
		for(unsigned char count = 0; count<LogInfo.ch_sensor_count; count++)
		{
			//Log every byte in the sensor
			for(unsigned char byte = 0; byte<sensor[count].size; byte++)
			{
				sdio_byte2file(&Log, *(sensor[count].address + byte));
			}
		}
	}
};

/*
 * Write an additional character to current log stream.
 * Intended for csv-formats etc.
 */
void log_write_char(unsigned char ch_byte)
{
	sdio_byte2file(&Log, ch_byte);
};

/*
 * Finish log and save to sd-card
 */
void log_finish(void)
{
	//Check if sd-card is inserted and log is not finished
	if((sd->state & SD_CARD_DETECTED) && LogInfo.ch_log_open)
	{
		//Write data to sd-card
		sdio_write_file(&Log);
		//Close file
		sdio_fclose(&Log);
		LogInfo.ch_log_open = 0;
		//remove sd card
		sdio_set_inactive();
	}
};

