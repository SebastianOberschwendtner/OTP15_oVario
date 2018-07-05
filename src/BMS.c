/*
 * BMS.c
 *	Created for TI BQ25700A
 *  Created on: 10.05.2018
 *      Author: Sebastian
 */

#include "BMS.h"
#include "Variables.h"

BMS_T* pBMS;

extern unsigned long error_var;

/*
 * init BMS
 * The BQ25700A requires LSB first at communication
 */
void init_BMS(void)
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

	//***********************Setup the coulomb counter**********************************************
	//save the desired manufacturer status init register value
	//FIXME At startup without the battery the i2c command waits for a response. Use timeouts?

	unsigned int i_config = IGNORE_SD_EN | ACCHG_EN | ACDSG_EN;
	//check the configuration value on the flash
	i2c_send_int_register_LSB(i2c_addr_BMS_GAUGE,MAC_addr,MAC_STATUS_INIT_df_addr);
	if(i_config != i2c_read_int(i2c_addr_BMS_GAUGE,MAC_DATA_addr))
	{
		//If the configuration on the flash doesn't match, write the flash.
		//Changes take affect on next restart.
		i2c_send_int_register_LSB(i2c_addr_BMS_GAUGE,MAC_addr,MAC_STATUS_INIT_df_addr);
		i2c_send_int_register(i2c_addr_BMS_GAUGE,MAC_DATA_addr,i_config);
		//calculate checksum
		unsigned int i_checksum = (MAC_STATUS_INIT_df_addr>>8) + (MAC_STATUS_INIT_df_addr & 0xFF);
		i_checksum += (i_config>>8);
		i_checksum += (i_config & 0xFF);
		i_checksum = ~i_checksum;
		i2c_send_int_register(i2c_addr_BMS_GAUGE,MAC_SUM_addr,(i_checksum<<8)+0x06);

	}
	//wait for flash write to finish
	wait_systick(1);

	//save the desired configuration register value
	i_config = 0x00;
	//check the configuration value on the flash
	i2c_send_int_register_LSB(i2c_addr_BMS_GAUGE,MAC_addr,CONFIGURATION_A_df_addr);
	if(i_config != i2c_read_int(i2c_addr_BMS_GAUGE,MAC_DATA_addr))
	{
		//If the configuration on the flash doesn't match, write the flash.
		//Changes take affect on next restart.
		i2c_send_int_register_LSB(i2c_addr_BMS_GAUGE,MAC_addr,CONFIGURATION_A_df_addr);
		i2c_send_int_register(i2c_addr_BMS_GAUGE,MAC_DATA_addr,i_config);
		//calculate checksum
		unsigned int i_checksum = (CONFIGURATION_A_df_addr>>8) + (CONFIGURATION_A_df_addr & 0xFF);
		i_checksum += (i_config>>8);
		i_checksum += (i_config & 0xFF);
		i_checksum = ~i_checksum;
		i2c_send_int_register(i2c_addr_BMS_GAUGE,MAC_SUM_addr,(i_checksum<<8)+0x06);

	}
	//Check communication status
	if(i2c_get_error())
	{
		pBMS->com_err = (unsigned char)0xFFFE;
		error_var |= err_coloumb_fault;
		i2c_reset_error();
	}

	//wait for flash write to finish
	wait_systick(1);

	//***********************Setup the BMS**********************************************************
	/*
	 * Set Charge options:
	 * -disable low power mode
	 * -disable WDT
	 * -PWM 800 kHz
	 * -IADPT Gain 40
	 * -IDPM enable
	 * -Out-of-audio enable
	 */
	i2c_send_int_register_LSB(i2c_addr_BMS,CHARGE_OPTION_0_addr,
			(PWM_FREQ | IADPT_GAIN | IBAT_GAIN | EN_LDO | EN_IDPM | EN_OOA));
	/*
	 * Set ADC options:
	 * 	-One shot update
	 * 	-Enable VBAT, VBUS, I_IN, I_charge, I_Discharge
	 */
	i2c_send_int_register_LSB(i2c_addr_BMS,ADC_OPTION_addr,
			(EN_ADC_VBAT | EN_ADC_VBUS | EN_ADC_IIN | EN_ADC_ICHG | EN_ADC_IDCHG));

	/*
	 * Set max charge voltage
	 * Resolution is 16 mV with this formula:
	 * VMAX = Register * 16 mV/bit + 1024
	 *
	 * The 11-bit value is bitshifted by 4
	 */
	i2c_send_int_register_LSB(i2c_addr_BMS,MAX_CHARGE_VOLTAGE_addr,(((MAX_BATTERY_VOLTAGE-1024)/16)<<4));

	/*
	 * Set min sys voltage
	 * Resolution is 256 mV with this formula:
	 * VMIN = Register * 256 mV/bit + 1024
	 *
	 * The 6-bit value is bitshifted by 8
	 */
	//i2c_send_int_register_LSB(i2c_addr_BMS,MIN_SYS_VOLTAGE_addr,(((MIN_BATTERY_VOLTAGE-1024)/256)<<8));

	/*
	 * Set max input current
	 * Resolution is 50 mA with this formula:
	 * IMAX = Register * 50 mV/bit
	 *
	 * The 7-bit value is bitshifted by 8
	 */
	i2c_send_int_register_LSB(i2c_addr_BMS,INPUT_LIMIT_HOST_addr,((MAX_CURRENT/50)<<8));

	//Register memory
	pBMS = ipc_memory_register(49,did_BMS);

	//Set charging current
	pBMS->max_charge_current = 1000;

	//Set OTG parameters
	pBMS->otg_voltage = OTG_VOLTAGE;
	pBMS->otg_current = OTG_CURRENT;

	//Check communication status
	if(i2c_get_error())
	{
		pBMS->com_err = (unsigned char)0xFFFF;
		error_var |= err_bms_fault;
		i2c_reset_error();
	}
};

/*
 * Task
 */
void BMS_task(void)
{
	// Read Values
	BMS_get_adc();
	BMS_gauge_get_adc();
	BMS_get_status();

	// Handle Commands
	uint8_t ipc_no_bytes = ipc_get_queue_bytes(did_GUI);
	T_GUI_cmd Gui_ipc_cmd;

	while(ipc_no_bytes > 5)
	{
		ipc_queue_get(did_GUI,6,&Gui_ipc_cmd); 	// get new command
		ipc_no_bytes = ipc_get_queue_bytes(did_GUI);

		switch(Gui_ipc_cmd.cmd)					// switch for pad number
		{
		case GUI_cmd_OTG_ON:
			BMS_set_otg(ON);
			break;
		case GUI_cmd_OTG_OFF:
			BMS_set_otg(OFF);
			break;
		default:
			break;
		}
	}




}

/*
 * Solar Panel Charge Controller
 */
float U_error 	= 0;
float I_Charge 	= 0;
float I_Gain    = 100;

void BMS_SolarPanelController(void)
{
#define	n_cells 6.0f	// Number of Serial Solar Cells Used
#define U_MPP	0.6f	// Voltage of Maximum Power Point of one Cell

	// Solar Panel Controller (I Controller)
	U_error 	= (float)pBMS->input_voltage - n_cells * U_MPP;
	I_Charge 	+= I_Gain * U_error;
	BMS_set_charge_current((unsigned int)I_Charge);
}


/*
 * Start ADC conversion
 */

void BMS_adc_start(void)
{
	//If no communication error occurred
	if(!(pBMS->com_err & SENSOR_ERROR))
	{
		//Reset i2c error
		i2c_reset_error();

		//Send command
		i2c_send_int_register_LSB(i2c_addr_BMS,ADC_OPTION_addr,
				(ADC_START | EN_ADC_VBAT | EN_ADC_VBUS | EN_ADC_IIN | EN_ADC_ICHG | EN_ADC_IDCHG));
		pBMS->charging_state |= STATUS_ADC_REQUESTED;
		pBMS->charging_state &= ~(STATUS_ADC_FINISHED);

		//check communication error
		pBMS->com_err += i2c_get_error();
	}
	else
	{
		error_var |= err_bms_fault;
	}

};

/*
 * Read VBAT
 * To get a reading of the adc values this function has to be executed at least twice with a delay of at least approx. 50 ms!
 */
void BMS_get_adc(void)
{
	unsigned int i_temp = 0;

	//If no communication error occurred
	if(!(pBMS->com_err & SENSOR_ERROR))
	{
		//Reset i2c error
		i2c_reset_error();
		//read BMS status
		BMS_get_status();

		//start ADC when no conversion is started
		if((pBMS->charging_state & STATUS_ADC_FINISHED) && !(pBMS->charging_state & STATUS_ADC_REQUESTED))
			BMS_adc_start();
		//read ADC when conversion is finished
		if((pBMS->charging_state & STATUS_ADC_FINISHED) && (pBMS->charging_state & STATUS_ADC_REQUESTED))
		{
			pBMS->charging_state &= ~(STATUS_ADC_REQUESTED);
			/*
			 * Read the battery voltage, only the last 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * VBAT = ADC * 64 mV/bit + 2880 mV
			 */
			i_temp = i2c_read_int_LSB(i2c_addr_BMS,ADC_SYS_VOLTAGE_addr);
			pBMS->battery_voltage = (i_temp & 0b11111111)*64 + 2880;

			/*
			 * Read the bus voltage, only the first 8 bits are the value of the bus voltage
			 * The voltage reading follows this formula:
			 * VIN = ADC * 64 mV/bit + 3200 mV
			 */
			//Check whether input voltage is present
			if(pBMS->charging_state & STATUS_INPUT_PRESENT)
				pBMS->input_voltage = (i2c_read_int_LSB(i2c_addr_BMS,ADC_VBUS_addr) >> 8)*64 + 3200;
			else
				pBMS->input_voltage = 0;

			//temporary save the battery current adc value
			i_temp = i2c_read_int_LSB(i2c_addr_BMS,ADC_BAT_CURRENT_addr);

			/*
			 * Read the charge current, only the first 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * I_Charge = ADC * 64 mA/bit
			 */
			pBMS->charge_current = (i_temp >> 8)*64;

			/*
			 * Read the discharge current, only the last 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * I_Discharge = ADC * 256 mA/bit
			 */
			pBMS->discharge_current = (i_temp & 0b1111111)*256;

			/*
			 * Read the input current, only the first 8 bits are the value of the battery voltage
			 * The voltage reading follows this formula:
			 * I_Charge = ADC * 50 mA/bit
			 */
			i_temp = i2c_read_int_LSB(i2c_addr_BMS,ADC_INPUT_CURRENT_addr);
			pBMS->input_current = (i_temp >> 8)*50;
		}
		//check communication error
		pBMS->com_err += i2c_get_error();
	}
	else
	{
		error_var |= err_bms_fault;
	}
};

/*
 * Read the BMS status
 */
//TODO can the reset of the options bits be optimized?
void BMS_get_status(void)
{
	//If no communication error occurred
	if(!(pBMS->com_err & SENSOR_ERROR))
	{
		i2c_reset_error();
		unsigned i_temp = i2c_read_int_LSB(i2c_addr_BMS,CHARGER_STATUS_addr);

		//check the status bits from charger_status
		if(i_temp & AC_STAT)
			pBMS->charging_state |= STATUS_INPUT_PRESENT;
		else
			pBMS->charging_state &= ~(STATUS_INPUT_PRESENT);
		if(i_temp & IN_FCHRG)
			pBMS->charging_state |= STATUS_FAST_CHARGE;
		else
			pBMS->charging_state &= ~(STATUS_FAST_CHARGE);
		if(i_temp & IN_PCHRG)
			pBMS->charging_state |= STATUS_PRE_CHARGE;
		else
			pBMS->charging_state &= ~(STATUS_PRE_CHARGE);
		if(i_temp & IN_OTG)
			pBMS->charging_state |= STATUS_OTG_EN;
		else
			pBMS->charging_state &= ~(STATUS_OTG_EN);

		//check the adc status
		i_temp = i2c_read_int_LSB(i2c_addr_BMS,ADC_OPTION_addr);
		if(!(i_temp & ADC_START))
			pBMS->charging_state |= STATUS_ADC_FINISHED;
		else
			pBMS->charging_state &= ~(STATUS_ADC_FINISHED);

		//check the CHRG_OK pin
		if(GPIOA->IDR & GPIO_IDR_IDR_1)
			pBMS->charging_state |= STATUS_CHRG_OK;
		else
			pBMS->charging_state &= ~(STATUS_CHRG_OK);

		//check communication error
		pBMS->com_err += i2c_get_error();
	}
	else
	{
		error_var |= err_bms_fault;
	}
};

/*
 * Start battery charging.
 * Checks whether the battery is already charging or not.
 */
void BMS_charge_start(void)
{
	//If no communication error occurred
	if(!(pBMS->com_err & SENSOR_ERROR))
	{
		i2c_reset_error();

		//Check input source
		if(pBMS->charging_state & STATUS_CHRG_OK)
		{
			//Check if allready charging
			if(!(pBMS->charging_state & STATUS_FAST_CHARGE))
			{
				/*
				 * Set charge current in mA. Note that the actual resolution is only 64 mA/bit.
				 * Setting the charge current automatically starts the charge procedure.
				 */
				//Clamp to max charge current of 8128 mA
				if(pBMS->max_charge_current > 8128)
					pBMS->max_charge_current = 8128;

				i2c_send_int_register_LSB(i2c_addr_BMS,CHARGE_CURRENT_addr,((pBMS->max_charge_current/64)<<6));
			}
		}
		pBMS += i2c_get_error();
	}
	else
	{
		error_var |= err_bms_fault;
	}
};

/*
 * Set battery charging current.
 * Checks whether input is present or not.
 * This function doesn't save the set current!
 */
void BMS_set_charge_current(unsigned  int i_current)
{
	//If no communication error occurred
	if(!(pBMS->com_err & SENSOR_ERROR))
	{
		i2c_reset_error();

		//Check input source
		if(pBMS->charging_state & STATUS_CHRG_OK)
		{
			/*
			 * Set charge current in mA. Note that the actual resolution is only 64 mA/bit.
			 * Setting the charge current to 0 automatically terminates the charge.
			 */
			//Clamp to max charge current of 8128 mA
			if(i_current > 8128)
				i_current = 8128;

			i2c_send_int_register_LSB(i2c_addr_BMS,CHARGE_CURRENT_addr,((i_current/64)<<6));
			pBMS->max_charge_current = i_current;
		}
		pBMS += i2c_get_error();
	}
	else
	{
		error_var |= err_bms_fault;
	}
};

/*
 * Enables the OTG mode
 */
//TODO Implement an error if OTG is enabled during charging
void BMS_set_otg(unsigned char ch_state)
{
	//If no communication error occurred
	if(!(pBMS->com_err & SENSOR_ERROR))
	{
		i2c_reset_error();

		if(ch_state == ON)
		{
			//Check if input power is present and not in otg mode
			if(!(pBMS->charging_state & STATUS_CHRG_OK) && !(pBMS->charging_state & STATUS_OTG_EN))
			{
				i2c_send_int_register_LSB(i2c_addr_BMS,CHARGE_CURRENT_addr,0);		//Set charging current 0

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
				i2c_send_int_register_LSB(i2c_addr_BMS,OTG_VOLTAGE_addr,(((pBMS->otg_voltage-4480)/64)<<6));

				/*
				 * Set OTG current
				 * Resolution is 50 mA with this formula:
				 * IOTG = Register * 50 mA/bit
				 *
				 * The 7-bit value is bitshifted by 8
				 */
				//Limit the voltage
				if(pBMS->otg_current > 5000)
					pBMS->otg_current = 5000;
				i2c_send_int_register_LSB(i2c_addr_BMS,OTG_CURRENT_addr,((pBMS->otg_current/50)<<8));

				//Enable OTG
				i2c_send_int_register_LSB(i2c_addr_BMS,CHARGE_OPTION_3_addr,EN_OTG);
				GPIOA->BSRRL |= GPIO_BSRR_BS_0;										//Set EN_OTG high
			}
		}
		else
		{
			//disable OTG
			GPIOA->BSRRH |= GPIO_BSRR_BR_0;										//Set EN_OTG low
			i2c_send_int_register_LSB(i2c_addr_BMS,CHARGE_OPTION_3_addr,0);
		}
		pBMS->com_err += i2c_get_error();
	}
	else
	{
		error_var |= err_bms_fault;
	}
};

/*
 * Get the battery parameters from the coulomb counter
 */
//TODO discuss whether signed int for current should be used, problem: i2c data register is unsigned
void BMS_gauge_get_adc(void)
{

	//If no communication error occurred
	if(!(pBMS->com_err & SENSOR_ERROR))
	{
		i2c_reset_error();

		pBMS->battery_voltage = i2c_read_int_LSB(i2c_addr_BMS_GAUGE,VOLTAGE_addr);

		//Get battery current, has to be converted from unsigned to signed
		unsigned int i_temp = i2c_read_int_LSB(i2c_addr_BMS_GAUGE,CURRENT_addr);
		if(i_temp > 32767)
			pBMS->current = i_temp - 65535;
		else
			pBMS->current = i_temp;

		//Get accumulated charge, has to be converted from unsigned to signed, positive discharge, negative charge
		//TODO Add offline saving of discharged capacity.

		i_temp = i2c_read_int_LSB(i2c_addr_BMS_GAUGE,ACC_CHARGE_addr);
		if(i_temp > 32767)
			pBMS->discharged_capacity = i_temp - 65535;
		else
			pBMS->discharged_capacity = i_temp;

		pBMS->com_err += i2c_get_error();
	}
	else
	{
		error_var |= err_bms_fault;
	}
};
