/**
 ******************************************************************************
 * @file    igc.c
 * @author  SO
 * @version V2.0
 * @date    19-September-2020
 * @brief   The task which handles the IGC logging.
 ******************************************************************************
 */

//****** Includes ******
#include "igc.h"

//****** Typedefs ******
#pragma pack(push, 1)
typedef struct
{
    char linebuffer[77]; //Maximum record length + LF (see FAI Appendix A2.1)
    char hashbuffer[77]; //Buffer which contains all valid characters for the hash
    unsigned char linepointer;
    unsigned char hashpointer;
    unsigned char open;
    MD5_T md5[IGC_HASH_NUMBER];
} IGCINFO_T;
#pragma pack(pop)

//****** Variables ******
//ipc structs
SDIO_T *sd;
GPS_T *GpsData;
datafusion_T *BaroData;
T_keypad p_ipc_gui_keypad_data;

//private structs
IGCINFO_T IgcInfo;
T_command rxcmd_igc;    //Command struct to receive ipc commands
T_command txcmd_igc;    //Command struct to sent ipc commands
TASK_T task_igc;

//Hash start keys
unsigned long const g_key[16] = {
    0x1C80A301, 0x9EB30b89, 0x39CB2Afe, 0x0D0FEA76,
    0x48327203, 0x3948ebea, 0x9a9b9c9e, 0xb3bed89a,
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476,
    0xc8e899e8, 0x9321c28a, 0x438eba12, 0x8cbe0aee};

/**
 * @brief Get the return value of the last finished ipc task which was called.
 * @details inline function
 */
inline unsigned long igc_get_call_return(void)
{
    return rxcmd_igc.data;
};

//****** Functions ******
/**
 * @brief register memory for igc
 */
void igc_register_ipc(void)
{
    //Initialize task struct
    arbiter_clear_task(&task_igc);
    arbiter_set_command(&task_igc, CMD_IDLE);

    //Initialize receive command struct
    rxcmd_igc.did           = did_IGC;
    rxcmd_igc.cmd           = 0;
    rxcmd_igc.data          = 0;
    rxcmd_igc.timestamp     = 0;

    //Initialize IGC data
    IgcInfo.open = IGC_LOG_CLOSED;

    //Register the command queue, holds 5 commands
    ipc_register_queue(5 * sizeof(T_command), did_IGC);
};

/**
 * @brief Get everything relevant for IPC
 */
void igc_get_ipc(void)
{
	// get the ipc pointer addresses for the needed data
    //get sd handler
    sd = ipc_memory_get(did_SDIO);
    //get gps handler
    GpsData = ipc_memory_get(did_GPS);
    //get the datafusion handler
    BaroData = ipc_memory_get(did_DATAFUSION);
};

/**
 ***********************************************************
 * @brief TASK IGC
 ***********************************************************
 * Systime has to be up to date!
 * 
 ***********************************************************
 * @details
 * Execution:	interruptable
 * Wait: 		Yes
 * Halt: 		Yes
 **********************************************************
 */
void igc_task(void)
{
    //Check commands in queue
    igc_check_commands();

    //When the task wants to wait
    if(task_igc.wait_counter)
        task_igc.wait_counter--; //Decrease the wait counter
    else    //Execute task when wait is over
    {
        //Perform command action when the task does not wait for other tasks to finish
        if (task_igc.halt_task == 0)
        {
            //Perform action according to active state
            switch (arbiter_get_command(&task_igc))
            {
            case CMD_IDLE:
                igc_idle();
                break;

            case IGC_CMD_CREATE_LOG:
                igc_create_log();
                break;

            case IGC_CMD_CREATE_HEADER:
                igc_create_header();
                break;

            case IGC_CMD_WRITE_LOG:
                igc_write_log();
                break;

            case IGC_CMD_FINISH_LOG:
                igc_close();
                break;

            default:
                break;
            }
        }
    }
    // Check for errors
    // igc_check_error();
};

/**
 * @brief Check the commands in the igc queue and call the corresponding command.
 */
void igc_check_commands(void)
{
    //Check commands
    if (ipc_get_queue_bytes(did_IGC) >= sizeof(T_command)) // look for new command in keypad queue
    {
        ipc_queue_get(did_IGC, sizeof(T_command), &rxcmd_igc); // get new command
        switch (rxcmd_igc.cmd)                  // switch for command
        {
        case cmd_ipc_signal_finished:
            //Called task is finished
            task_igc.halt_task -= rxcmd_igc.did;
            break;

        case cmd_igc_start_logging: // create logfile and start logging
            //Only when no log is active
            if(IgcInfo.open == IGC_LOG_FINISHED)
                IgcInfo.open = IGC_LOG_CLOSED;
            break;

        case cmd_igc_stop_logging: // stop logging
            //Only when a log is active
            if (IgcInfo.open == IGC_LOGGING)
            {
                IgcInfo.open = IGC_LOG_BUSY;
                arbiter_set_command(&task_igc, IGC_CMD_FINISH_LOG);
            }
            break;

        default:
            break;
        }
    }
};

/**
 ***********************************************************
 * @brief The idle state of the task.
 ***********************************************************
 *
 * It checks whether the logging state can be entered or not.
 * After a log file is created, the task automatically
 * starts logging.
 * 
 * Argument:    none
 * Return:      none
 * @details call-by-reference
 ***********************************************************
 */
void igc_idle(void)
{
    //When the log file is created, start logging
    if (IgcInfo.open == IGC_LOG_OPEN)
    {
        //Send infobox
        txcmd_igc.did = did_IGC;
        txcmd_igc.cmd = cmd_gui_set_std_message;
        txcmd_igc.data = data_info_logging_started;
        txcmd_igc.timestamp = TIM5->CNT;
        ipc_queue_push(&txcmd_igc, sizeof(T_command), did_GUI);

        //Set logging command
        arbiter_callbyreference(&task_igc, IGC_CMD_WRITE_LOG,0);
    }
    else if(IgcInfo.open == IGC_LOG_CLOSED)
    {
        //log is closed, so start a new log, when the gps has a fix and when sd-card is present
        if ((GpsData->fix > 0) && (sd->status & SD_CARD_SELECTED))
            arbiter_callbyreference(&task_igc, IGC_CMD_CREATE_LOG, 0);

    }

    //When gps has a fix, set green led
    if(GpsData->fix)
        set_led_green(ON);
};

/**
 ***********************************************************
 * @brief Create new IGC log.
 ***********************************************************
 * 
 * Arguments:   none
 * @return Return 1 when the log is created
 * @details call-by-reference
 ***********************************************************
 */
void igc_create_log(void)
{
    //Allocate memory
    char *igc_name = (char *)arbiter_malloc(&task_igc, 3);

    //Log is not created yet
    IgcInfo.open = IGC_LOG_BUSY;

    //Perform the command action
    switch (arbiter_get_sequence(&task_igc))
    {
    case SEQUENCE_ENTRY:
        //Initialize filename
        sys_num2str(igc_name, get_year_utc(), 2);
        sys_num2str(igc_name + 2, get_month_utc(), 2);
        sys_num2str(igc_name + 4, get_day_utc(), 2);
        sys_num2str(igc_name + 6, task_igc.local_counter + 1, 2);
        igc_name[8]  = 'I';
        igc_name[9]  = 'G';
        igc_name[10] = 'C';
        igc_name[11] = 0x00;

        //Push command to search for file in ipc queue
        igc_call_task(SDIO_CMD_MKFILE, (unsigned long)igc_name, did_SDIO);

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_CREATE_FILENAME);
        break;

    case IGC_SEQUENCE_CREATE_FILENAME:
        //Check whether MKFILE is finished and created the file
        if (sd->status & SD_CMD_FINISHED)
        {
            if (igc_get_call_return())
            {
                //initialize hashes with custom keys
                for (unsigned char count = 0; count < 4; count++)
                    md5_initialize(&IgcInfo.md5[count], (unsigned long *)(&(g_key[4 * count])));

                //Tell the md5 task the address of the active string
                igc_call_task(cmd_md5_set_active_string,(unsigned long)IgcInfo.hashbuffer,did_MD5);
                
                //Goto next sequence
                arbiter_set_sequence(&task_igc, SEQUENCE_FINISHED);
            }
            else
            {
                //Increase file counter
                task_igc.local_counter++;
                //File already exists, so start procedure again
                arbiter_set_sequence(&task_igc, SEQUENCE_ENTRY);
            }
        }
        break;

    case SEQUENCE_FINISHED:
        //Create the header of the file
        if (arbiter_callbyreference(&task_igc, IGC_CMD_CREATE_HEADER, 0))
        {
            //Log is created
            IgcInfo.open = IGC_LOG_OPEN;

            //Write file to sd-card
            igc_call_task(SDIO_CMD_WRITE_FILE, (unsigned long)ipc_memory_get(did_FILEHANDLER), did_SDIO);

            //exit command
            arbiter_return(&task_igc, 1);
        }

    default:
        break;
    }
};

/**
 ***********************************************************
 * @brief Create the header of the igc file. 
 ***********************************************************
 * 
 * The local counter variable is used to determine the 
 * flight number.
 * 
 * Argument:    none
 * @return Returns 1 when header is created.
 * @details call-by-reference
 ***********************************************************
 */
void igc_create_header(void)
{
    //Perform Command action
    switch (arbiter_get_sequence(&task_igc))
    {
    case SEQUENCE_ENTRY:
        //initialize line buffer
        IgcInfo.linepointer = 0;
        IgcInfo.linebuffer[76] = 0x00;

        //Set next sequence and immediately enter it
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_1);

    case IGC_SEQUENCE_LINE_1:
        //Write header as required in FAI Chapter A3.2.4
        //Manufacturer ID
        igc_NewRecord('A');
        igc_AppendString(IGC_MANUF_ID);
        igc_AppendString(IGC_LOGGER_ID);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_2);
        break;

    case IGC_SEQUENCE_LINE_2:
        /*
            * FR type
            * This has to come second, because this mimics the XCSOAR behaviour and enables that commas are included in G - records.
            * Very important !Otherwise the G - record will be invalid.
            */
        igc_NewRecord('H');
        igc_AppendString("FFTYFRTYPE:");
        igc_AppendString(IGC_FR_TYPE);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_3);
        break;

    case IGC_SEQUENCE_LINE_3:
        //Log date and flight number
        igc_NewRecord('H');
        igc_AppendString("FDTE");
        igc_AppendNumber(get_day_utc(), 2);
        igc_AppendNumber(get_month_utc(), 2);
        igc_AppendNumber(get_year_utc(), 2);
        igc_AppendString(",");
        igc_AppendNumber(task_igc.local_counter + 1, 2);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_4);
        break;

    case IGC_SEQUENCE_LINE_4:
        //Pilot in Charge
        igc_NewRecord('H');
        igc_AppendString("FPLTPILOTINCHARGE:");
        igc_AppendString(IGC_PILOT_NAME);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_5);
        break;

    case IGC_SEQUENCE_LINE_5:
        //Co-Pilot --> not yet applicable :P
        igc_NewRecord('H');
        igc_AppendString("FCM2CREW2:");
        igc_AppendString("None");
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_6);
        break;

    case IGC_SEQUENCE_LINE_6:
        //Glider type
        igc_NewRecord('H');
        igc_AppendString("FGTYGLIDERTYPE:");
        igc_AppendString(IGC_GLIDER_TYPE);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_7);
        break;

    case IGC_SEQUENCE_LINE_7:
        //Glider ID
        igc_NewRecord('H');
        igc_AppendString("FGIDGLIDERID:");
        igc_AppendString(IGC_GLIDER_ID);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_8);
        break;

    case IGC_SEQUENCE_LINE_8:
        //GPS DATUM
        igc_NewRecord('H');
        igc_AppendString("FDTMGPSDATUM:WG84");
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_9);
        break;

    case IGC_SEQUENCE_LINE_9:
        //Firmware version
        igc_NewRecord('H');
        igc_AppendString("FRFWFIRMWAREVERSION:");
        igc_AppendString(IGC_FIRMWARE_VER);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_10);
        break;

    case IGC_SEQUENCE_LINE_10:
        //Hardware version
        igc_NewRecord('H');
        igc_AppendString("FRHWHARDWAREVERSION:");
        igc_AppendString(IGC_HARDWARE_VER);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_11);
        break;

    case IGC_SEQUENCE_LINE_11:
        //GPS receiver
        igc_NewRecord('H');
        igc_AppendString("FGPSRECEIVER:");
        igc_AppendString(IGC_GPS_RX);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_12);
        break;

    case IGC_SEQUENCE_LINE_12:
        //barometer
        igc_NewRecord('H');
        igc_AppendString("FPRSPRESSALTSENSOR:");
        igc_AppendString(IGC_BARO_MANUF);
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, IGC_SEQUENCE_LINE_13);
        break;

    case IGC_SEQUENCE_LINE_13:
        //add fix accuracy to B-record from byte 36 to byte 38
        igc_NewRecord('I');
        igc_AppendString("013638FXA");
        igc_WriteLine();

        //Goto next sequence
        arbiter_set_sequence(&task_igc, SEQUENCE_FINISHED);
        break;

    case SEQUENCE_FINISHED:
        //Exit command
        arbiter_return(&task_igc, 1);
        break;

    default:
        break;
    }
};

/**
 ***********************************************************
 * @brief Log the gs track
 ***********************************************************
 * 
 * Argument:    none
 * Return:      none
 * 
 * @details call-by-reference
 ***********************************************************
 */
void igc_write_log(void)
{
    //perform the command action
    switch (arbiter_get_sequence(&task_igc))
    {
    case SEQUENCE_ENTRY:
        set_led_green(ON);
        //Log is now logging
        IgcInfo.open = IGC_LOGGING;

        //Write B-Record
        igc_BRecord();

        //Reset wait counter and wait for 1000ms or 1s
        /**
         * The actual value has to be tweaked because the scheduler timing
         * is not too precise and it has some jitter.
         * @todo make timing here more precise
         */
        task_igc.local_counter = MS2TASKTICK(780, LOOP_TIME_TASK_IGC);

        //goto wait sequence
        arbiter_set_sequence(&task_igc, SEQUENCE_FINISHED);
        break;

    case SEQUENCE_FINISHED:
        set_led_green(OFF);
        //decrease the wait counter, when it is not 0
        if (task_igc.local_counter)
            task_igc.local_counter--;
        else
            arbiter_set_sequence(&task_igc, SEQUENCE_ENTRY);
        break;

    default:
        break;
    }
};

/**
 **********************************************************
 * @brief Close IGC log, sign it and write to sd-card.
 **********************************************************
 * Also sets the log status entry.
 * 
 * Argument:    none
 * @return Returns 1 when the file is closed.
 * @details call-by-reference
 **********************************************************
 */
void igc_close(void)
{
    //Log is not yet closed
    IgcInfo.open = IGC_LOG_BUSY;

    //Allocate memory
    unsigned long* l_count      = arbiter_malloc(&task_igc,2);
    unsigned long* l_LineCount  = l_count + 1;
    
    //Perform the command action
    switch (arbiter_get_sequence(&task_igc))
    {
        case SEQUENCE_ENTRY:
            //Tell the md5 task to finalize the hashes
            for (unsigned char i = 0; i < IGC_HASH_NUMBER; i++)
                igc_call_task(MD5_CMD_FINALIZE, (unsigned long)&IgcInfo.md5[i], did_MD5);
            
            //Reset Hash counter
            *l_count     = 0;

            //Goto next sequence
            arbiter_set_sequence(&task_igc,IGC_SEQUENCE_GET_DIGEST);
            // arbiter_set_sequence(&task_igc, IGC_SEQUENCE_WRITE_FILE);
            break;

        case IGC_SEQUENCE_GET_DIGEST:
            //Save digest to hashbuffer
            md5_GetDigest(&IgcInfo.md5[*l_count], IgcInfo.hashbuffer);

            //Reset line counter
            *l_LineCount = 0;

            //Goto directly to next sequence
            arbiter_set_sequence(&task_igc, IGC_SEQUENCE_WRITE_DIGEST);

        case IGC_SEQUENCE_WRITE_DIGEST:
            //Create G-Record
            igc_NewRecord('G');
            //Write one junk of the digest
            for (unsigned char character = 0; character < IGC_DIGEST_CHARPERLINE; character++)
                IgcInfo.linebuffer[character + 1] = IgcInfo.hashbuffer[character + *l_LineCount * IGC_DIGEST_CHARPERLINE];

            //Terminate string with LF and zero afterwards
            IgcInfo.linebuffer[IGC_DIGEST_CHARPERLINE + 1] = '\n';
            IgcInfo.linebuffer[IGC_DIGEST_CHARPERLINE + 2] = 0x00;

            //Tell sdio task to write the new string
            igc_call_task(SDIO_CMD_STRING2FILE, (unsigned long)IgcInfo.linebuffer, did_SDIO);

            //Increase line counter
            *l_LineCount += 1;

            //check whether all hashes are written
            if (*l_LineCount == (32/IGC_DIGEST_CHARPERLINE))
            {
                //Current hash is written, goto next hash
                *l_count += 1;

                //Check whether all hashes are written
                if (*l_count == IGC_HASH_NUMBER)
                    arbiter_set_sequence(&task_igc, IGC_SEQUENCE_WRITE_FILE);
                else
                    arbiter_set_sequence(&task_igc, IGC_SEQUENCE_GET_DIGEST);
            }
            break;

        case IGC_SEQUENCE_WRITE_FILE:
            //Write file to sd-card
            igc_call_task(SDIO_CMD_FCLOSE, 0, did_SDIO);

            //Goto next sequence
            arbiter_set_sequence(&task_igc, SEQUENCE_FINISHED);
            break;

        case SEQUENCE_FINISHED:
            //Send infobox
            txcmd_igc.did = did_IGC;
            txcmd_igc.cmd = cmd_gui_set_std_message;
            txcmd_igc.data = data_info_logging_stopped;
            txcmd_igc.timestamp = TIM5->CNT;
            ipc_queue_push(&txcmd_igc, sizeof(T_command), did_GUI);

            //Log is closed
            IgcInfo.open = IGC_LOG_FINISHED;

            //Exit command
            arbiter_return(&task_igc,1);
            break;
    }

};

/**
 * @brief Check if current character is a valid igc character and therefore included in the g-record.
 * @param character The character which should be checked.
 * @return Whether the character is a valid character.
 */
unsigned char igc_IsValidCharacter(unsigned char character)
{
    unsigned char valid = 0;

    //check for character range, the character 0x7E is excluded because it is a reserved character
    if ((character < 0x7E) && (character >= 0x20))
        valid = 1;

    //check for reserved characters
    if (character == '$' || character == '*' || character == '!' || character == '\\' || character == '^')
        valid = 0;

    return valid;
};

/**
 * @brief Check whether current record should be included in g-record or not.
 * @param in Pointer to the current record.
 * @return Whether record should be included in hash.
 */
unsigned char igc_IncludeInGrecord(char *in)
{
    unsigned char valid = false;

    switch (in[0])
    {
    case 'L':
        if (sys_strcmp(IGC_MANUF_ID, &in[1]))
            // only include L records made by XCS
            valid = true;
        break;

    case 'G':
        break;

    case 'H':
        if ((in[1] != 'O') && (in[1] != 'P'))
            valid = true;
        break;

    default:
        valid = true;
    }

    return valid;
};

/**
 * @brief Call a other task via the ipc queue.
 * @param cmd The command which should be called
 * @param data The data for the command.
 * @param did_target The did of the target task.
 */
void igc_call_task(unsigned char cmd, unsigned long data, unsigned char did_target)
{
    //Set the command and data for the target task
    txcmd_igc.did = did_IGC;
    txcmd_igc.cmd = cmd;
    txcmd_igc.data = data;

    //Push the command
    ipc_queue_push(&txcmd_igc, sizeof(T_command), did_target);

    //Set wait counter to wait for called task to finish
    task_igc.halt_task += did_target;
};

/**
 * @brief Commit char to hash
 * @param character The character to be committed
 */
void igc_CommitCharacter(unsigned char character)
{
    //Check if character is valid
    if (igc_IsValidCharacter(character))
    {
        //Commit character to the hashbuffer
        IgcInfo.hashbuffer[IgcInfo.hashpointer++] = character;
    }
};

/**
 * @brief Commit line of log to hash.
 * String has to be terminated with a LF (0x0A).
 * @param line The pointer to the current record string to be committed
 */
void igc_CommitLine(char *line)
{
    //Counter for string
    unsigned char ch_count = 0;

	//Check if current record should be included in hash
	if(igc_IncludeInGrecord(line))
	{
		while(line[ch_count] != '\n')
		{
			//Check for end of string
			if(line[ch_count] == 0)
				break;
			//Commit the characters of the string
			igc_CommitCharacter(line[ch_count]);
			//Next character
			ch_count++;
		}
        //Terminate hash buffer
        IgcInfo.hashbuffer[IgcInfo.hashpointer++] = 0x00;

        //Tell md5 task to append the hash
        for (unsigned char i = 0; i < IGC_HASH_NUMBER; i++)
            igc_call_task(MD5_CMD_APPEND_STRING, (unsigned long)&IgcInfo.md5[i], did_MD5);
    }
};

/**
 * @brief Write line to file and commit to hash.
 * The LF at the end of the line is added automatically using the linepointer.
 */
void igc_WriteLine(void)
{
    /*
	 * Write LF at end of line and thus include it in the linecommit, although we know LF is an invalid character.
	 * Just to be sure in case the FAI or XCSOAR decide that the LF is a valid character.
	 */
    IgcInfo.linebuffer[IgcInfo.linepointer++] = '\n';

    //Terminate line string
    IgcInfo.linebuffer[IgcInfo.linepointer++] = 0x00;

    //Write string to fileIgc
    igc_call_task(SDIO_CMD_STRING2FILE, (unsigned long)IgcInfo.linebuffer, did_SDIO);

    //Commit line
    igc_CommitLine(IgcInfo.linebuffer);
};

/**
 * @brief Add new record line to log.
 * The old line has to be finished with a LF.
 * @param type The type of the new record
 */
void igc_NewRecord(unsigned char type)
{
    IgcInfo.linebuffer[0] = type;
    IgcInfo.linepointer = 1;
    IgcInfo.hashpointer = 0;
};

/**
 * @brief Append string to linebuffer and current record.
 * @param string The pointer to current string to be appended.
 */
void igc_AppendString(char *string)
{
    //Check if linebuffer is full
    if (IgcInfo.linepointer < 76)
    {
        while (*string != 0)
        {
            IgcInfo.linebuffer[IgcInfo.linepointer] = *string;
            string++;
            IgcInfo.linepointer++;
        }
    }
    else
    {
        //linebuffer is full, just reset pointer => this will lead to datacorruption
        IgcInfo.linepointer = 0;
    }
};

/**
 * @brief Append a number with a specific number of digits to the current record.
 * The sys_num2str function is used to save RAM and increase speed.
 * @param number The value to be appended
 * @param digits The number of significant digits
 */
void igc_AppendNumber(unsigned long number, unsigned char digits)
{
    //Check if the digits fit in the line buffer
    if (IgcInfo.linepointer < (76 - digits))
    {
        //Compute the string content
        sys_num2str(&IgcInfo.linebuffer[IgcInfo.linepointer], number, digits);
        IgcInfo.linepointer += digits;
    }
};

/**
 * @brief Write F-record. This records contains the current count of satellites used and their id.
 * @todo Add id support
 */
void igc_FRecord(void)
{
    igc_NewRecord('F');
    //Write UTC time
    igc_AppendNumber(get_hours_utc(), 2);
    igc_AppendNumber(get_minutes_utc(), 2);
    igc_AppendNumber(get_seconds_utc(), 2);
    for (unsigned char count = 0; count < GpsData->n_sat; count++)
        igc_AppendNumber(count, 2);
    igc_WriteLine();
};

/**
 * @brief Write B-record. This records the gps fixes.
 */
void igc_BRecord(void)
{
    unsigned long TempInt = 0;
    float temp = 0;

    igc_NewRecord('B');
    //Write UTC time
    igc_AppendNumber(get_hours_utc(), 2);
    igc_AppendNumber(get_minutes_utc(), 2);
    igc_AppendNumber(get_seconds_utc(), 2);

    //Write latitude
    //DONE Compute coordinates in minutes and seconds
    if (GpsData->lat >= 0)
    {
        //degrees
        temp = GpsData->lat;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 2);

        //minutes
        temp = (temp - TempInt) * 60000;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 5);

        igc_AppendString("N"); //DONE How do i know whether S or N? -> through the sign of the float
    }
    else
    {
        //degrees
        temp = GpsData->lat;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 2);

        //minutes
        temp = (temp - TempInt) * 60000;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 5);

        igc_AppendString("S");
    }

    //Write longitude
    if (GpsData->lon >= 0)
    {
        //degrees
        temp = GpsData->lon;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 3);

        //minutes
        temp = (temp - TempInt) * 60000;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 5);

        igc_AppendString("E"); //DONE How do i know whether W or E? -> through the sign of the float
    }
    else
    {

        //degrees
        temp = GpsData->lon;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 3);

        //minutes
        temp = (temp - TempInt) * 60000;
        TempInt = (unsigned long)temp;
        igc_AppendNumber(TempInt, 5);

        igc_AppendString("W");
    }

    //Write fix validity
    if (GpsData->fix == 3)
        igc_AppendString("A"); //3D fix
    else
        igc_AppendString("V"); //2D fix or no GPS data

    //Write pressure altitude
    igc_AppendNumber((unsigned long)BaroData->height, 5);

    //Write GNSS altitude
    igc_AppendNumber((unsigned long)GpsData->height, 5); //DONE Get height above ellipsoid

    //Write fix accuracy
    igc_AppendNumber((unsigned long)GpsData->hAcc, 3); //DONE Get real accuracy

    //Write line
    igc_WriteLine();
};

/**
 * @brief Return the current state of the IGC Logging.
 * This is done, because only for the state it seems not to be necessary
 * to register the 2.8kB of the whole IGC struct in the ipc memory.
 * Maybe it is also good, that the state is read-only this way...
 * @return The current state of teh IGC log.
 */
unsigned char igc_get_state(void)
{
    return IgcInfo.open;
};
