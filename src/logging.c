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

FILE_T Log;
SDIO_T* sd;
/*
 * create the log file
 */
void log_create(void)
{
	unsigned long l_count = 0;
	unsigned char ch_temp = 0;
	char pch_name[9] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00};

	//get sd state
	sd = ipc_memory_get(did_SDIO);

	//Only log if sd-card is detected
	if(sd->state & SD_CARD_DETECTED)
	{
		//initialize name
		Log.name[11] = 0;
		ch_temp = sys_strcpy(pch_name,LOG_NAME);

		//create file on sd-card
		sdio_read_root();

		//Check for existing log and create appending numbering
		do
		{
			l_count++;
			ch_temp = sys_num2str(pch_name+LOG_NUMBER_START, l_count, LOG_NUMBER_LENGTH);
		}while(sdio_get_fileid(&Log, pch_name, LOG_EXTENSION));

		//create new log with lognumber in filename
		sdio_mkfile(&Log, pch_name, LOG_EXTENSION);

	}
};

/*
 * Finish log and save to sd-card
 */
void log_finish(void)
{
	//Write data to sd-card
	sdio_write_file(&Log);
	//Close file
	sdio_fclose(&Log);
};

