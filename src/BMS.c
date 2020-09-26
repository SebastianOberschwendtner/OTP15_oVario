/**
 ******************************************************************************
 * @file    bms.c
 * @author  SO
 * @version V2.0
 * @date    19-September-2020
 * @brief   Talks to the BMS IC TI BQ25700A via I2C and manages the battery
 * 			charging and the OTG functionality.
 ******************************************************************************
 */
//****** Includes ******
#include "BMS.h"

//****** Variables ******
BMS_T* pBMS;
unsigned char bat_health = 100;
unsigned char gauge_command[3];
unsigned char gauge_buffer[32];

TASK_T task_bms;			// Task struct for ms6511-task
unsigned long *pl_args;		// Pointer to call commands with multiple arguments
T_command rxcmd_bms;		// Command struct to receive ipc commands
T_command txcmd_bms;		// Command struct to send ipc commands

//****** Inline Functions ******
/**
 * @brief Get the return value of the last finished ipc task which was called.
 * @return Return value of command.
 * @details inline function
 */
inline unsigned long bms_get_call_return(void)
{
    return rxcmd_bms.data;
};

/**
 * @brief Create the data payload for the I2C command.
 * 
 * The I2C sends all bytes of the command with MSB first, but the BMS
 * expects LSB first. 
 * Some commands set a register address and the an integer value. 
 * To convert this order for the I2C command the inline function swaps 
 * the bytes:
 * 
 * In:	00;REG;MSB;LSB;
 * Out: 00;MSB;LSB;REG;
 * 
 * @param reg The register address of the data
 * @param data The data which should be trasnmitted
 * @return The payload data which is swapped from "MSB to LSB".
 * @details inline function
 */
inline unsigned long bms_create_payload(unsigned char reg, unsigned long data)
{
	return (data<<8) | reg;
};

//****** Functions ******
/**
 * @brief Register everything relevant for IPC
 */
void bms_register_ipc(void)
{
	//Register everything relevant for IPC
	//Register memory
	pBMS = ipc_memory_register(sizeof(BMS_T),did_BMS);
	//Register command queue
	ipc_register_queue(5 * sizeof(T_command), did_BMS);
	ipc_register_queue(5 * sizeof(T_command), did_COUL);

	//Initialize task struct
    arbiter_clear_task(&task_bms);
    arbiter_set_command(&task_bms, BMS_CMD_INIT);

    //Initialize receive command struct
    rxcmd_bms.did           = did_BMS;
    rxcmd_bms.cmd           = 0;
    rxcmd_bms.data          = 0;
    rxcmd_bms.timestamp     = 0;
};

/**
 ***********************************************************
 * @brief TASK BMS
 ***********************************************************
 * 
 * 
 ***********************************************************
 * @details
 * Execution:	non-interruptable
 * Wait: 		Yes
 * Halt: 		Yes
 **********************************************************
 */
void bms_task(void)
{
    //When the task wants to wait
    if(task_bms.wait_counter)
        task_bms.wait_counter--; //Decrease the wait counter
    else    //Execute task when wait is over
    {
        //Perform command action when the task does not wait for other tasks to finish
        if (task_bms.halt_task == 0)
        {
            //Perform action according to active state
            switch (arbiter_get_command(&task_bms))
            {
            case CMD_IDLE:
				bms_idle();
				break;

			case BMS_CMD_INIT:
				bms_init();
				break;
			
			case BMS_CMD_INIT_COULOMB:
				coulomb_init();
				break;

			case BMS_CMD_GET_STATUS:
				bms_get_status();
				break;

			case BMS_CMD_GET_ADC:
				bms_get_adc();
				break;

			case BMS_CMD_SET_CHARGE_CURRENT:
				bms_set_charge_current();
				break;

			case BMS_CMD_SET_OTG:
				bms_set_otg();
				break;

			case BMS_CMD_READ_INT_FLASH:
				coulomb_read_int_flash();
				break;

			case BMS_CMD_WRITE_INT_FLASH:
				coulomb_write_int_flash();
				break;

			case BMS_CMD_GET_ADC_COULOMB:
				coulomb_get_adc();
				break;

			default:
				break;
			}
		}
		else
			bms_check_semaphores(); //Task is waiting for semaphores
	}

	///@todo Check for errors here
};

/**
 * @brief Check the semaphores in the ipc queue.
 * 
 * This command is called before the command action. Since the BMS is non-
 * interruptable, this command checks only for semaphores.
 * 
 * @details Other commands are checked in the idle command.
 */
void bms_check_semaphores(void)
{
	//Variable to remember when a new command is received
	unsigned char new_command = 0;

    //Check semaphores for BMS
    if (ipc_get_queue_bytes(did_BMS) >= sizeof(T_command))      // look for new command in keypad queue
	{
		new_command = 1;
		ipc_queue_get(did_BMS, sizeof(T_command), &rxcmd_bms);  // get new command
	}

	//Check semaphores for Coulomb Counter
    if (ipc_get_queue_bytes(did_COUL) >= sizeof(T_command))      // look for new command in keypad queue
	{
		new_command = 1;
		ipc_queue_get(did_COUL, sizeof(T_command), &rxcmd_bms); // get new command
	}

	//When a new command was received
	if (new_command)
	{
		//Evaluate semaphores
		switch (rxcmd_bms.cmd)
		{
		case cmd_ipc_signal_finished:
			//The called task is finished, decrease halt counter
			task_bms.halt_task -= rxcmd_bms.did;
			break;

		case cmd_ipc_outta_time:
			//The called command timed out, so set the corresponding sensor inactive since it is not responding
			if (rxcmd_bms.data == did_BMS)
				pBMS->charging_state &= ~STATUS_BMS_ACTIVE; //The BMS sensors is faulty
			else if (rxcmd_bms.data == did_COUL)
				pBMS->charging_state &= ~STATUS_GAUGE_ACTIVE; //The coulomb counter is faulty

		default:
			break;
		}
	}
};

/**********************************************************
 * @brief Idle Command for BMS
 **********************************************************
 * 
 * This command checks for new commands in the queue.
 * 
 * Attention:
 * As of now, commands which are received when this task is
 * halted are lost! Commands can only be received when the
 * task is in idle state!
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details Should/Can not be called directly via the arbiter.
 **********************************************************/
void bms_idle(void)
{
	//the idle task assumes that all commands are finished, so reset all sequence states
	arbiter_reset_sequence(&task_bms);

	 //Check commands
    if (ipc_get_queue_bytes(did_BMS) >= sizeof(T_command)) // look for new command in keypad queue
    {
		ipc_queue_get(did_BMS, sizeof(T_command), &rxcmd_bms); // get new command

		//Call the command
		arbiter_callbyreference(&task_bms, rxcmd_bms.cmd, &rxcmd_bms.data);
	}
};

/**********************************************************
 * @brief Initialize the BMS System
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details call-by-reference
 **********************************************************/
void bms_init(void)
{
	//Allocate memory
	unsigned long* l_argument = arbiter_malloc(&task_bms,1);

	//Perform the command action
	switch(arbiter_get_sequence(&task_bms))
	{
		case SEQUENCE_ENTRY:
			bms_init_peripherals();
			//Set the initial communication status
			pBMS->charging_state |= (STATUS_GAUGE_ACTIVE | STATUS_BMS_ACTIVE | STATUS_BAT_PRESENT);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_OPTION_0);
			break;

		case BMS_SEQUENCE_SET_OPTION_0:
			/* Set Charge options:
			 * -disable low power mode
			 * -disable WDT
			 * -PWM 800 kHz
			 * -IADPT Gain 40
			 * -IDPM enable
			 * -Out-of-audio enable
			 */
			*l_argument = bms_create_payload(CHARGE_OPTION_0_addr, (PWM_FREQ | IADPT_GAIN | IBAT_GAIN));
			//Send the command via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_OPTION_1);
			break;

		case BMS_SEQUENCE_SET_OPTION_1:
			/*
	 		 * Set Charge Option 1 register
	 		 */
			*l_argument = bms_create_payload(CHARGE_OPTION_1_addr, AUTO_WAKEUP_EN);
			//Send the command via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_OPTION_ADC);
			break;

		case BMS_SEQUENCE_SET_OPTION_2:
			/*
	 		 * Set Charge Option 2 register
	 		 */
			*l_argument = bms_create_payload(CHARGE_OPTION_2_addr, 0);
			//Send the command via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_OPTION_ADC);
			break;

		case BMS_SEQUENCE_SET_OPTION_ADC:
			/*
	 		 * Set ADC options:
			 * 	-One shot update
			 * 	-Enable VBAT, VBUS, I_IN, I_charge, I_Discharge
	 		 */
			*l_argument = bms_create_payload(ADC_OPTION_addr, (EN_ADC_VBAT | EN_ADC_VBUS | EN_ADC_IIN | EN_ADC_ICHG | EN_ADC_IDCHG));
			//Send the command via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_MAX_VOLT);
			break;

		case BMS_SEQUENCE_SET_MAX_VOLT:
			/*
	 		 * Set max charge voltage
	 		 * Resolution is 16 mV with this formula:
	 		 * VMAX = Register * 16 mV/bit
	 		 *
	 		 * The 11-bit value is bitshifted by 4
	 		 */
			*l_argument = bms_create_payload(MAX_CHARGE_VOLTAGE_addr, (((MAX_BATTERY_VOLTAGE)/16)<<4));
			//Send the command via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_SYS_VOLT);
			break;

		case BMS_SEQUENCE_SET_SYS_VOLT:
			/*
	 		 * Set min sys voltage
			 * Resolution is 256 mV with this formula:
	 		 * VMIN = Register * 256 mV/bit
			 *
			 * The 6-bit value is bitshifted by 8
	 		 */
			*l_argument = bms_create_payload(MIN_SYS_VOLTAGE_addr, (((MIN_BATTERY_VOLTAGE)/256)<<8));
			//Send the command via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_INP_CURR);
			break;

		case BMS_SEQUENCE_SET_INP_CURR:
			/*
	 		 * Set max input current
			 * Resolution is 50 mA with this formula:
			 * IMAX = Register * 50 mV/bit
			 *
			 * The 7-bit value is bitshifted by 8
	 		 */
			*l_argument = bms_create_payload(INPUT_LIMIT_HOST_addr, ((MAX_CURRENT/50)<<8));
			//Send the command via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);
			break;

		case SEQUENCE_FINISHED:
			// Set charging current
			pBMS->max_charge_current = 800;

			//Set OTG parameters
			pBMS->otg_voltage = OTG_VOLTAGE;
			pBMS->otg_current = OTG_CURRENT;

			//Exit the command, after the coulomb counter is initialized
			if (arbiter_callbyreference(&task_bms, BMS_CMD_INIT_COULOMB, 0))
				arbiter_return(&task_bms, 1);
			break;

		default:
			break;
	}
};

/**********************************************************
 * @brief Initialize the Coulomb Counter
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details call-by-reference
 **********************************************************/
void coulomb_init(void)
{
	//Perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
		case SEQUENCE_ENTRY:
			//Allocate Arguments
			pl_args = arbiter_allocate_arguments(&task_bms, 1);

			//First read the flash to see whether the settings are up-to-date
			pl_args[0] = MAC_STATUS_INIT_df_addr;
			if (arbiter_callbyvalue(&task_bms, BMS_CMD_READ_INT_FLASH))
			{
				//Check whether the current settings match the desired settings
				/*Desired Settings:
				 * IGNORE_SD_EN:	Ignore the self-discharge of battery
				 * ACCHG_EN:		Integrate the charge current
				 * ACDSG_EN:		Integrate the discharge current
				 */
#define COULOMB_MAC_CONFIG (IGNORE_SD_EN | ACCHG_EN | ACDSG_EN) // MAC Settings for Coulomb Counter

				if (arbiter_get_return_value(&task_bms) == COULOMB_MAC_CONFIG)
					arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_CONFIG_A); //Settings did match, goto next setting
				else
					arbiter_set_sequence(&task_bms, BMS_SEQUENCE_SET_MAC); //Settings did not match, so update them
			}
			break;

		case BMS_SEQUENCE_SET_MAC:
			//Settings did not match
			//Allocate Arguments to update the settings
			pl_args = arbiter_allocate_arguments(&task_bms, 2);
			pl_args[0] = MAC_STATUS_INIT_df_addr;
			pl_args[1] = COULOMB_MAC_CONFIG;

			//Write settings and goto next setting afterwards
			if (arbiter_callbyvalue(&task_bms, BMS_CMD_WRITE_INT_FLASH))
				arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_CONFIG_A);
			break;

		case BMS_SEQUENCE_READ_CONFIG_A:
			//Allocate Arguments
			pl_args = arbiter_allocate_arguments(&task_bms, 1);

			//First read the flash to see whether the settings are up-to-date
			pl_args[0] = CONFIGURATION_A_df_addr;
			if (arbiter_callbyvalue(&task_bms, BMS_CMD_READ_INT_FLASH))
			{
				//Check whether the current settings match the desired settings
				if (arbiter_get_return_value(&task_bms) == 0)
					arbiter_return(&task_bms, 1); //Settings did match, finish command
				else
					arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED); //Settings did not match, so update them
			}
			break;

		case SEQUENCE_FINISHED:
			//Settings for CONFIG_A did not match
			//Allocate Arguments to update the settings
			pl_args = arbiter_allocate_arguments(&task_bms, 2);
			pl_args[0] = CONFIGURATION_A_df_addr;
			pl_args[1] = 0;

			//Write settings and exit command afterwards
			if (arbiter_callbyvalue(&task_bms, BMS_CMD_WRITE_INT_FLASH))
				arbiter_return(&task_bms, 1);
			break;
		
		default:
			break;
	}
};

/**********************************************************
 * @brief Read the status of the BMS System
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details call-by-reference
 **********************************************************/
void bms_get_status(void)
{
	//Allocate memory
	unsigned long* l_status = arbiter_malloc(&task_bms,1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
		case SEQUENCE_ENTRY:
			//Read the status of the bms via i2c
			bms_call_task(I2C_CMD_READ_INT, CHARGER_STATUS_addr, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_GET_STATUS);
			break;

		case BMS_SEQUENCE_GET_STATUS:
			//When i2c returned the data, convert it to LSB first
			*l_status = sys_swap_endian(bms_get_call_return(),2);

			//check the status bits from charger_status
			if (*l_status & AC_STAT)
				pBMS->charging_state |= STATUS_INPUT_PRESENT;
			else
				pBMS->charging_state &= ~(STATUS_INPUT_PRESENT);

			if (*l_status & IN_FCHRG)
				pBMS->charging_state |= STATUS_FAST_CHARGE;
			else
				pBMS->charging_state &= ~(STATUS_FAST_CHARGE);

			if (*l_status & IN_PCHRG)
				pBMS->charging_state |= STATUS_PRE_CHARGE;
			else
				pBMS->charging_state &= ~(STATUS_PRE_CHARGE);

			if (*l_status & IN_OTG)
				pBMS->charging_state |= STATUS_OTG_EN;
			else
				pBMS->charging_state &= ~(STATUS_OTG_EN);

			// Read the next status
			bms_call_task(I2C_CMD_READ_INT, ADC_OPTION_addr, did_I2C);

			//Goto next sequence
			arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);
			break;

		case SEQUENCE_FINISHED:
			//When i2c returned the data, convert it to LSB first
			*l_status = sys_swap_endian(bms_get_call_return(),2);

			//check the adc status
			if (!(*l_status & ADC_START))
				pBMS->charging_state |= STATUS_ADC_FINISHED;
			else
				pBMS->charging_state &= ~(STATUS_ADC_FINISHED);

			//check the CHRG_OK pin
			if (GPIOA->IDR & GPIO_IDR_IDR_1)
				pBMS->charging_state |= STATUS_CHRG_OK;
			else
				pBMS->charging_state &= ~(STATUS_CHRG_OK);

			//Exit the command
			arbiter_return(&task_bms,1);
			break;

		default:
			break;
	}
};

/**********************************************************
 * @brief Get the ADC values of the BMS System
 **********************************************************
 * 
 * Argument:	none
 * Return:		none
 * 
 * @details call-by-reference
 **********************************************************/
void bms_get_adc(void)
{
	//allocate memory
	unsigned long* l_argument = arbiter_malloc(&task_bms,1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
		case SEQUENCE_ENTRY:
			//Get the status of the bms
			if (arbiter_callbyreference(&task_bms,BMS_CMD_GET_STATUS,0))
				arbiter_set_sequence(&task_bms, BMS_SEQUENCE_START_ADC); //Goto next sequence
			break;

		case BMS_SEQUENCE_START_ADC:
			//Check whether the ADC is finished and get aquire new data
			if (pBMS->charging_state & STATUS_ADC_FINISHED)
			{
				//When no conversion is ongoing, start a new conversion
				if (!(pBMS->charging_state & STATUS_ADC_REQUESTED))
				{
					//Send command
					*l_argument = bms_create_payload(ADC_OPTION_addr, (ADC_START | EN_ADC_VBAT | EN_ADC_VBUS | EN_ADC_IIN | EN_ADC_ICHG | EN_ADC_IDCHG));
					bms_call_task(I2C_CMD_SEND_24BIT, *l_argument, did_I2C);

					//Set the ADC flags in the charging_state
					pBMS->charging_state |= STATUS_ADC_REQUESTED;
					pBMS->charging_state &= ~(STATUS_ADC_FINISHED);

					//Set wait to 60ms before reading the status again
					task_bms.wait_counter = MS2TASKTICK(60, LOOP_TIME_TASK_BMS);

					//Read the status again
					arbiter_set_sequence(&task_bms, SEQUENCE_ENTRY);
				}
				else
				{
					//Conversion is finished, read the results
					arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_SYS);
				}
			}
			else
				arbiter_set_sequence(&task_bms, SEQUENCE_ENTRY); //Read the status of the bms again
			break;

		case BMS_SEQUENCE_READ_SYS:
			//Reset the request state
			pBMS->charging_state &= ~(STATUS_ADC_REQUESTED);
			
			//Request the results of the ADC for the SYS voltage
			bms_call_task(I2C_CMD_READ_INT, ADC_SYS_VOLTAGE_addr, did_I2C);

			//Goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_VBUS);
			break;

		case BMS_SEQUENCE_READ_VBUS:
			//When i2c returned the data, convert it to LSB first
			*l_argument = sys_swap_endian(bms_get_call_return(),2);
			/*
			 * Read the battery voltage, only the last 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * VBAT = ADC * 64 mV/bit + 2880 mV
			 */
			pBMS->battery_voltage = (*l_argument & 0b11111111)*64 + 2880;

			//Request the results of the ADC for the VBUS voltage, when INPUT is present
			if(pBMS->charging_state & STATUS_INPUT_PRESENT)
			{
				//Request the result
				bms_call_task(I2C_CMD_READ_INT, ADC_VBUS_addr, did_I2C);
				//Goto next state
				arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_BAT);
			}
			else
			{
				//When no input is present, the voltage is zero
				pBMS->input_voltage = 0;
				//Request the BAT current and skip next sequence
				bms_call_task(I2C_CMD_READ_INT, ADC_BAT_CURRENT_addr, did_I2C);
				arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_INP);
			}
			break;

		case  BMS_SEQUENCE_READ_BAT:
			//When i2c returned the data, convert it to LSB first
			*l_argument = sys_swap_endian(bms_get_call_return(),2);
			/*
			 * Read the bus voltage, only the first 8 bits are the value of the bus voltage
			 * The voltage reading follows this formula:
			 * VIN = ADC * 64 mV/bit + 3200 mV
			 */
			pBMS->input_voltage = (*l_argument >> 8)*64 + 3200;

			//Request the BAT current
			bms_call_task(I2C_CMD_READ_INT, ADC_BAT_CURRENT_addr, did_I2C);

			//Goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_INP);
			break;

		case BMS_SEQUENCE_READ_INP:
			//When i2c returned the data, convert it to LSB first
			*l_argument = sys_swap_endian(bms_get_call_return(),2);

			/*
			 * Read the charge current, only the first 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * I_Charge = ADC * 64 mA/bit
			 */
			pBMS->charge_current = (*l_argument >> 8)*64;

			/*
			 * Read the discharge current, only the last 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * I_Discharge = ADC * 256 mA/bit
			 */
			pBMS->discharge_current = (*l_argument & 0b1111111)*256;

			//Request the BAT current
			bms_call_task(I2C_CMD_READ_INT, ADC_INPUT_CURRENT_addr, did_I2C);

			//Goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_READ_COLOUMB);
			break;

		case BMS_SEQUENCE_READ_COLOUMB:
			//When i2c returned the data, convert it to LSB first
			*l_argument = sys_swap_endian(bms_get_call_return(),2);
			/*
			 * Read the input current, only the first 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * I_Charge = ADC * 50 mA/bit
			 */
			pBMS->input_current = (*l_argument >> 8)*50;

			//Immediately goto next state
			arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);

		case SEQUENCE_FINISHED:
			//Read the ADC of teh coulomb counter
			if (arbiter_callbyreference(&task_bms, BMS_CMD_GET_ADC_COULOMB, 0))
			{
				//Command is finished
				arbiter_return(&task_bms, 1);
			}
			break;

		default:
			break;
	}
};

/**********************************************************
 * @brief Set the charging current of the BMS
 **********************************************************
 * This command can be used to start and stop the charging.
 * When the battery is not charging, setting a non-zero
 * charging current starts the charging.
 * When the battery is charging, setting the charging 
 * current to 0 stops the charging.
 * Returns 1 when the charge current could be set.
 * 
 * @param l_charge_current The new charging current
 * @return Whether the charging current was set or not.
 * 
 * @details call-by-value, nargs = 1
 **********************************************************/
void bms_set_charge_current(void)
{
	//Get arguments
	unsigned long *pl_charging_current = arbiter_get_argument(&task_bms);

	//Allocate memory
	unsigned long *pl_argument = arbiter_malloc(&task_bms, 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
	case SEQUENCE_ENTRY:
		//Clamp to max charge current of 8128 mA
		if (*pl_charging_current > 8128)
			*pl_charging_current = 8128;

		//Check input source and if in OTG mode
		if ((pBMS->charging_state & (STATUS_CHRG_OK | STATUS_BMS_ACTIVE | STATUS_BAT_PRESENT)) && !(pBMS->charging_state & STATUS_OTG_EN))
		{
			/*
			 * Set charge current in mA. Note that the actual resolution is only 64 mA/bit.
			 * Setting the charge current to 0 automatically terminates the charge.
			 */
			*pl_argument = bms_create_payload(CHARGE_CURRENT_addr, ((*pl_charging_current / 64) << 6));
			bms_call_task(I2C_CMD_SEND_24BIT, *pl_argument, did_I2C);
			//goto next sequence
			arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);
		}
		else
			arbiter_return(&task_bms,0); //Charge current cannot be set, exit command
		
		break;

	case SEQUENCE_FINISHED:
		//Remember new charge current
		pBMS->max_charge_current = *pl_charging_current;

		//Charge current was set, exit the command
		arbiter_return(&task_bms,1);
		break;

	default:
		break;
	}
};

/**********************************************************
 * @brief Set the state of the OTG
 **********************************************************
 * 
 * @param l_state_otg The new OTG state (ON or OFF)
 * @return The new state which was set.
 * 
 * @details call-by-value, nargs = 1
 **********************************************************/
void bms_set_otg(void)
{
	//get arguments
	unsigned long *pl_state_otg = arbiter_get_argument(&task_bms);

	//Allocate memory
	unsigned long *pl_argument = arbiter_malloc(&task_bms, 1);

	//perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
		case SEQUENCE_ENTRY:
			//Check whether to enable or disable the OTG mode
			if (*pl_state_otg == ON)
			{
				//Check if input power is present and not in otg mode
				if (!(pBMS->charging_state & STATUS_CHRG_OK) && !(pBMS->charging_state & STATUS_OTG_EN))
				{
					//Enable OTG, disable charging
					*pl_argument = bms_create_payload(CHARGE_CURRENT_addr, 0);
					bms_call_task(I2C_CMD_SEND_24BIT,*pl_argument,did_I2C);

					//goto next sequence
					arbiter_set_sequence(&task_bms, BMS_SEQUENCE_OTG_VOLTAGE);
				}
				else
					arbiter_return(&task_bms,0); //OTG cannot be enabled, exit the command				
			}
			else
				arbiter_set_sequence(&task_bms, BMS_SEQUENCE_OTG_SET_STATE); //OTG should be disabled -> directly send the new state
			break;

		case BMS_SEQUENCE_OTG_VOLTAGE:
			/*
			 * Set OTG voltage
			 * Resolution is 64 mV with this formula:
			 * VOTG = Register * 64 mV/bit + 4480 mV
			 *
			 * The 8-bit value is bitshifted by 6
			 */
			//Limit the voltage
			if(pBMS->otg_voltage > 15000)
				pBMS->otg_voltage = 15000;
			
			//Set the new voltage via I2c
			*pl_argument = bms_create_payload(OTG_VOLTAGE_addr,(((pBMS->otg_voltage-4480)/64)<<6));
			bms_call_task(I2C_CMD_SEND_24BIT, *pl_argument, did_I2C);

			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_OTG_CURRENT);
			break;
		
		case BMS_SEQUENCE_OTG_CURRENT:
			/*
			 * Set OTG current
			 * Resolution is 50 mA with this formula:
			 * IOTG = Register * 50 mA/bit
			 *
			 * The 7-bit value is bitshifted by 8
			 */
			//Limit the current
			if (pBMS->otg_current > 5000)
				pBMS->otg_current = 5000;

			//Set the current via I2C
			*pl_argument = bms_create_payload(OTG_CURRENT_addr, ((pBMS->otg_current/50)<<8));
			bms_call_task(I2C_CMD_SEND_24BIT, *pl_argument, did_I2C);

			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_OTG_SET_STATE);
			break;

		case BMS_SEQUENCE_OTG_SET_STATE:
			//Get the new state
			if (*pl_state_otg == ON)
				*pl_argument = bms_create_payload(CHARGE_OPTION_3_addr, EN_OTG);
			else
				*pl_argument = bms_create_payload(CHARGE_OPTION_3_addr, 0);
			
			//Send the new state via I2C
			bms_call_task(I2C_CMD_SEND_24BIT, *pl_argument, did_I2C);

			//goto next sequence
			arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);
			break;

		case SEQUENCE_FINISHED:
			//When communication is finsihed set the enable pin
			if (*pl_state_otg)
				GPIOA->BSRRL |= GPIO_BSRR_BS_0;
			else
				GPIOA->BSRRH |= GPIO_BSRR_BR_0;

			//Exit the command
			arbiter_return(&task_bms, *pl_state_otg);
			break;

		default:
			break;
	}
};

/**********************************************************
 * @brief Read an int from the flash of the coulomb counter
 **********************************************************
 * Although this functions only read an integer from the
 * specified address, it read 32 bits, the coulomb counter
 * only allows to READ this blocksize from the flash.
 * 
 * @param	i_register_address Defines the register address from which the data is read
 * @return 	The integer value which is returned by the coulomb counter.
 * 
 * @details call-by-value, nargs = 1
 **********************************************************/
void coulomb_read_int_flash(void)
{
	//Get arguments
	unsigned int *pi_register_address = arbiter_get_argument(&task_bms);

	//Allocate memory (array is also used as receive buffer: 32bytes + 1byte => 9 unsigned long)
	unsigned long *pl_command = arbiter_malloc(&task_bms, 10);
	unsigned char *pch_array = (unsigned char*)(pl_command + 1);

	//perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
	case SEQUENCE_ENTRY:
		//Set the command data to read the flash
		*pl_command = bms_create_payload(MAC_addr, (unsigned long)*pi_register_address);
		coulomb_call_task(I2C_CMD_SEND_24BIT, *pl_command, did_I2C);

		//immediately perform the read
		pch_array[0] = 32; //Set array length
		coulomb_call_task(I2C_CMD_READ_ARRAY, (unsigned long)pch_array, did_I2C);

		//Goto next sequence
		arbiter_set_sequence(&task_bms, BMS_SEQUENCE_GET_CRC);
		break;

	case BMS_SEQUENCE_GET_CRC:
		//Get the new CRC
		coulomb_call_task(I2C_CMD_READ_CHAR, MAC_SUM_addr, did_I2C);

		//goto next sequence
		arbiter_set_sequence(&task_bms, BMS_SEQUENCE_GET_LEN);
		break;
	
	case BMS_SEQUENCE_GET_LEN:
		//Read the returned CRC
		pBMS->crc = bms_get_call_return();

		//Get the length of the command
		coulomb_call_task(I2C_CMD_READ_CHAR, MAC_LEN_addr, did_I2C);

		//goto next sequence
		arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Read the returned Length of the command
		pBMS->len = bms_get_call_return();

		//Command finished and return the received value
		arbiter_return(&task_bms, (pch_array[1]<<8) | pch_array[2]);
		break;
	}
};

/**********************************************************
 * @brief Write an int to the flash of the coulomb counter
 **********************************************************
 * 
 * @param	i_register_address Defines the register address where the data should be written to
 * @param   i_data New data which should be written to register address
 * @return 	Whether the data was successfully sent.
 * 
 * @details call-by-value, nargs = 2
 **********************************************************/
void coulomb_write_int_flash(void)
{
	//Get arguments
	unsigned int *pi_register_address = arbiter_get_argument(&task_bms);
	unsigned int *pi_data = (arbiter_get_argument(&task_bms) + 4);

	//Allocate memory (array is also used as receive buffer: 32bytes + 1byte => 9 unsigned long)
	unsigned long *pl_command = arbiter_malloc(&task_bms, 10);
	unsigned char *pch_array = (unsigned char *)(pl_command + 1);

	//perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
	case SEQUENCE_ENTRY:
		//Set the command data to read the flash
		*pl_command = bms_create_payload(MAC_addr, (unsigned long)*pi_register_address);
		coulomb_call_task(I2C_CMD_SEND_24BIT, *pl_command, did_I2C);

		//immediately perform the read
		pch_array[0] = 32; //Set array length
		coulomb_call_task(I2C_CMD_READ_ARRAY, (unsigned long)pch_array, did_I2C);

		//Reset crc
		pBMS->crc = 0;

		//Goto next sequence
		arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);
		break;

	case SEQUENCE_FINISHED:
		//Set the new data in the received buffer
		pch_array[2] = (unsigned char)(*pi_data & 0xFF); //Set new MSB
		pch_array[1] = (unsigned char)(*pi_data >> 8);	 //SET new LSB
		pch_array[0] = 32;								 //Length of array

		//calculate the new checksum
		for (unsigned char count = 1; count <= 32; count++)
			pBMS->crc += pch_array[count];
		pBMS->crc += (*pi_register_address & 0xFF);
		pBMS->crc += (*pi_register_address >> 8);

		// write the command to set the new flash data and write the data
		*pl_command = bms_create_payload(MAC_addr, (unsigned long)*pi_register_address);
		coulomb_call_task(I2C_CMD_SEND_24BIT, *pl_command, did_I2C);
		coulomb_call_task(I2C_CMD_SEND_ARRAY, (unsigned long)pch_array, did_I2C);

		//set the new checksum and length to intiate the flash write
		coulomb_call_task(I2C_CMD_SEND_INT, ((~pBMS->crc) << 8) + 0x24, did_I2C);

		//Command is finished
		arbiter_return(&task_bms, 1);
		break;

	default:
		break;
	}
};

/**********************************************************
 * @brief Get the ADC values of the Coulomb Counter
 **********************************************************
 * 
 * Argument:	none
 * @return ADC values where read successfully.
 * 
 * @details call-by-reference
 **********************************************************/
void coulomb_get_adc(void)
{
	//Allocate memory
	unsigned long *pl_temp = arbiter_malloc(&task_bms, 1);

	//Perform the command action
	switch (arbiter_get_sequence(&task_bms))
	{
		case SEQUENCE_ENTRY:
			//Keep this state to do some entry stuff here in the future
			//immediately goto next state
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_GET_TEMP);
		
		case BMS_SEQUENCE_GET_TEMP:
			//Read the temperature of the coulomb counter
			coulomb_call_task(I2C_CMD_READ_INT, TEMPERATURE_addr, did_I2C);

			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_GET_VOLT);
			break;

		case BMS_SEQUENCE_GET_VOLT:
			//When i2c returned the data, convert it to LSB first
			pBMS->temperature = sys_swap_endian(bms_get_call_return(),2);

			//Read the battery voltage measured by the coulomb counter
			coulomb_call_task(I2C_CMD_READ_INT, VOLTAGE_addr, did_I2C);

			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_GET_CURRENT);
			break;

		case BMS_SEQUENCE_GET_CURRENT:
			//Read the returned value
			pBMS->battery_voltage = sys_swap_endian(bms_get_call_return(),2);

			//Read the battery current measured by the coulomb counter
			coulomb_call_task(I2C_CMD_READ_INT, CURRENT_addr, did_I2C);

			//goto next sequence
			arbiter_set_sequence(&task_bms, BMS_SEQUENCE_GET_CHARGE);
			break;

		case BMS_SEQUENCE_GET_CHARGE:
			//Get battery current, has to be converted from unsigned to signed
			*pl_temp = sys_swap_endian(bms_get_call_return(),2);
			if (*pl_temp > 32767)
				pBMS->current = *pl_temp - 65535;
			else
				pBMS->current = *pl_temp;

			//Read the accumulated charge measured by the coulomb counter
			coulomb_call_task(I2C_CMD_READ_INT, ACC_CHARGE_addr, did_I2C);

			//goto next sequence
			arbiter_set_sequence(&task_bms, SEQUENCE_FINISHED);
			break;
		
		case SEQUENCE_FINISHED:
			//Get accumulated charge, has to be converted from unsigned to signed, positive discharge, negative charge
			*pl_temp = sys_swap_endian(bms_get_call_return(),2);
			if (*pl_temp > 32767)
				pBMS->discharged_capacity = *pl_temp - 65535;
			else
				pBMS->discharged_capacity = *pl_temp;
			pBMS->discharged_capacity += pBMS->old_capacity;

			//Exit the command
			arbiter_return(&task_bms, 1);
			break;

		default:
			break;
	}
};

/**
 * @brief Initalize the peripherals needed for the bms
 */
void bms_init_peripherals(void)
{
	/*
	 * Set BMS specific IOs
	 * PA0	Output	PUSH_PULL	EN_OTG
	 * PA1	Input	PULL_UP		CHRG_OK
	 * PA2	Input	PULL_UP		PROCHOT
	 * PA3	Input	PULL_UP		ALERT_CC
	 */
	gpio_en(GPIO_A);
	GPIOA->MODER |= GPIO_MODER_MODER0_0;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR3_0 | GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR1_0;
};

/**
 * @brief Call a other task via the ipc queue.
 * @details Only forwards the call when the sensor is active. Otherwise it does nothing.
 * This means that, when the sensor is not active, the data in the bms struct will not be valid.
 */
void bms_call_task(unsigned char cmd, unsigned long data, unsigned char did_target)
{
	//Only when bms is active and responding
	if (pBMS->charging_state & STATUS_BMS_ACTIVE)
	{
		//Set the command and data for the target task
		txcmd_bms.did = did_BMS;
		txcmd_bms.cmd = cmd;
		txcmd_bms.data = data;

		//Push the command
		ipc_queue_push(&txcmd_bms, sizeof(T_command), did_target);

		//Set wait counter to wait for called task to finish
		task_bms.halt_task += did_target;
	}
};

/**
 * @brief Call a other task via the ipc queue.
 * @details Only forwards the call when the sensor is active. Otherwise it does nothing.
 * This means that, when the sensor is not active, the data in the bms struct will not be valid.
 */
void coulomb_call_task(unsigned char cmd, unsigned long data, unsigned char did_target)
{
	//Only when coulomb counter is active and responding
	if (pBMS->charging_state & STATUS_GAUGE_ACTIVE)
	{
		//Set the command and data for the target task
		txcmd_bms.did = did_COUL;
		txcmd_bms.cmd = cmd;
		txcmd_bms.data = data;

		//Push the command
		ipc_queue_push(&txcmd_bms, sizeof(T_command), did_target);

		//Set wait counter to wait for called task to finish
		task_bms.halt_task += did_target;
	}
};

// /*
//  * Task
//  */
// void BMS_task(void)
// {
// 	// Read Values
// 	BMS_get_adc();
// 	BMS_gauge_get_adc();
// 	BMS_get_status();
// 	BMS_check_battery();

// 	// set charge current when input source is present
// 	if (pBMS->charging_state & STATUS_INPUT_PRESENT)
// 		BMS_set_charge_current(800);

// 	// Handle Commands
// 	uint8_t ipc_no_bytes = ipc_get_queue_bytes(did_BMS);
// 	T_command IPC_cmd;

// 	while(ipc_no_bytes > 9)
// 	{
// 		ipc_queue_get(did_BMS,10,&IPC_cmd); 	// get new command
// 		ipc_no_bytes = ipc_get_queue_bytes(did_BMS);

// 		switch(IPC_cmd.cmd)					// switch for pad number
// 		{
// 		case cmd_BMS_OTG_ON:
// 			BMS_set_otg(ON);
// 			BMS_get_status();
// 			//Check whether otg was really enabled
// 			if(!(pBMS->charging_state & STATUS_OTG_EN))
// 			{
// 				//Send infobox
// 				IPC_cmd.did 		= did_BMS;
// 				IPC_cmd.cmd			= cmd_gui_set_std_message;
// 				IPC_cmd.data 		= data_info_otg_on_failure;
// 				IPC_cmd.timestamp 	= TIM5->CNT;
// 				ipc_queue_push(&IPC_cmd, 10, did_GUI);
// 			}
// 			break;
// 		case cmd_BMS_OTG_OFF:
// 			BMS_set_otg(OFF);
// 			break;
// 		case cmd_BMS_ResetCapacity:
// 			pBMS->old_capacity = 0;
// 			pBMS->discharged_capacity = 0;
// 		default:
// 			break;
// 		}
// 	}
// }

// /*
//  * Solar Panel Charge Controller
//  */
// float U_error 	= 0;
// float I_Charge 	= 0;
// float I_Gain    = 100;

// void BMS_SolarPanelController(void)
// {
// #define	n_cells 6.0f	// Number of Serial Solar Cells Used
// #define U_MPP	0.6f	// Voltage of Maximum Power Point of one Cell

// 	// Solar Panel Controller (I Controller)
// 	U_error 	= (float)pBMS->input_voltage - n_cells * U_MPP;
// 	I_Charge 	+= I_Gain * U_error;
// 	BMS_set_charge_current((unsigned int)I_Charge);
// }

// /*
//  * check whether battery is present or not
//  */
// void BMS_check_battery(void)
// {

// 	// When already in charging mode, check whether a battery is present via the actual chargin current
// 	if (pBMS->charging_state & STATUS_FAST_CHARGE)
// 	{
// 		// When the charging current is bigger than 10 mA the battery should be present, otherwise decrease the
// 		// battery health
// 		if((pBMS->current < 10) && (pBMS->current > -10))
// 			bat_health--;
// 	}
// 	// When no battery should be present, but the Battery Gauge can measure a voltage, reset the battery presence
// 	// When not in fast charge mode. In charge mode the battery voltage is present regardless whether there is
// 	// a battery present.
// 	else
// 	{
// 		if((pBMS->charging_state & STATUS_GAUGE_ACTIVE)
// 				&&(pBMS->battery_voltage>2880)
// 				&&!(pBMS->charging_state & STATUS_BAT_PRESENT))
// 			bat_health++;
// 	}

// 	// check the bat health whether the battery is present or not. The variable is used as a counter, because the
// 	// voltage response of the battery removal takes some time.
// 	if(bat_health > 115)
// 	{
// 		pBMS->charging_state |= STATUS_BAT_PRESENT;
// 		bat_health = 100;
// 	}
// 	else if (bat_health < 98)
// 	{
// 		// when the charging current is to small, disable the charging
// 		BMS_set_charge_current(0);
// 		pBMS->charging_state &= ~STATUS_BAT_PRESENT;
// 		bat_health = 100;
// 	}
// }