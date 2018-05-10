/*
 * BMS.h
 *  Created for TI BQ25700A
 *  Created on: 10.05.2018
 *      Author: Sebastian
 */

#ifndef BMS_H_
#define BMS_H_
//*********** Includes **************
#include "stm32f4xx.h"
#include "ipc.h"
#include "did.h"

//*********** Defines **************
/*
 * Register Addresses
 */
#define CHARGE_OPTION_0_addr		0x12	// R/W
#define CHARGE_OPTION_1_addr		0x30	// R/W
#define CHARGE_OPTION_2_addr		0x31	// R/W
#define CHARGE_OPTION_3_addr		0x32	// R/W
#define CHARGE_CURRENT_addr			0x14	// R/W
#define MAX_CHARGE_VOLTAGE_addr		0x15	// R/W
#define PROCHOT_OPTION_0_addr		0x33	// R/W
#define PROCHOT_OPTION_1_addr		0x34	// R/W
#define ADC_OPTION_addr				0x35	// R/W
#define CHARGER_STATUS_addr			0x20	// R
#define PROCHOT_STATUS_addr			0x21	// R
#define INPUT_LIMIT_DPM_addr		0x22	// R	Used input current limit value
#define ADC_VBUS_addr				0x23	// R	ADC results of VBUS and PSYS
#define ADC_BAT_CURRENT_addr		0x24	// R	ADC	results of battery currents
#define ADC_INPUT_CURRENT_addr		0x25	// R	ADC results of input current and CMPNI
#define ADC_SYS_VOLTAGE_addr		0x26	// R	ADC results of VSYS and VBAT
#define OTG_VOLTAGE_addr			0x3B	// R/W
#define OTG_CURRENT_addr			0x3C	// R/W
#define INPUT_VOLTAGE_addr			0x3D	// R/W
#define MIN_SYS_VOLTAGE_addr		0x3E	// R/W
#define INPUT_LIMIT_HOST_addr		0x3F	// R/W	Set input current limit
#define MANUFACTURER_ID_addr		0xFE	// R/W
#define DEVICE_ID_addr				0xFF	// R/W

/*
 * Option Bits
 */
//CHARGE_OPTION_0
#define EN_LWPWR					(1<<15)	// Enable low power mode
#define IDPM_AUTO_DISABLE			(1<<12)
#define OTG_ON_CHRGOK				(1<<11)	// ADD OTG to CHRG_OK
#define EN_OOA						(1<<10) // Enable Out-of-Audio
#define PWM_FREQ					(1<<9)	// Set Switching frequency
#define EN_LEARN					(1<<5)
#define IADPT_GAIN					(1<<4)	// IADPT amplifier ratio
#define IBAT_GAIN					(1<<3)	// IBAT amplifier ratio
#define EN_LDO						(1<<2)
#define EN_IDPM						(1<<1)	// Enable IDPM regulation loop
#define CHRG_INHIBIT				(1<<0)

//CHARGE_OPTION_1
#define EN_IBAT						(1<<15)	// Enable IBAT output buffer
#define EN_PSYS						(1<<12)	// Enable PSYS sensing circuit and output buffer
#define RSNS_RAC					(1<<11)	// Set input sense resistor RAC
#define RSNS_RSR					(1<<10)	// set charge sense resistor RSR
#define PSYS_RATIO					(1<<9)	// Ratio of PSYS output current vs. total input battery power
#define CMP_REF						(1<<8)	// Set independent comparator internal reference
#define CMP_POL						(1<<7)	// Set independent comparator polarity
#define FORCE_LATCHOFF				(1<<3)	// Force power path off
#define EN_SHIP_DCHRG				(1<<1)	// Discharge SRN for shipping mode
#define AUTO_WAKEUP_EN				(1<<0)

//CHARGE_OPTION_2
#define EN_PKPWR_IDPM				(1<<13)	// Enable peak power mode triggered by input current overshoot
#define EN_PKPWR_VSYS				(1<<12) // Enable peak power mode triggered by system voltage undershoot
#define PKPWR_OVLD_STAT				(1<<11) // Indicator at the device is in overloading cycle
#define PKPWR_RELAX_STAT			(1<<10) // Indicator at the device is in relaxation cycle
#define EN_EXTILIM					(1<<7)	// Enable ILIM_HIZ pin to set input current limit
#define EN_ICHG_IDCHG				(1<<6) 	// Set function of IBAT pin
#define Q2_OCP						(1<<5)	// Q2 OCP threshold by sensing Q2 VDS
#define ACX_OCP						(1<<4)	// Input current OCP threshold by sensing ACP-ACN
#define EN_ACOC						(1<<3)	// Enable input overcurrent protection by sensing the voltage across ACP-ACN
#define ACOC_VTH					(1<<2)	// Set Mosfet OCP threshold as percentage of IDPM
#define EN_BATOC					(1<<1)	// Enable battery discharge overcurrent protection by sensing the voltage across SRN-SRP
#define BATOC_VTH					(1<<0)	// Set battery discharge overcurrent threshold

//CHARGE_OPTION_3
#define EN_HIZ						(1<<15)	// Device HI-Z mode enable
#define RESET_REG					(1<<14)	// Reset registers
#define RESET_VINDPM				(1<<13)	// Reset VINDPM threshold
#define EN_OTG						(1<<12)	// Enable device OTG mode when EN_OTG pin is HIGH
#define EN_ICO_MODE					(1<<11)	// Enable ICO algorithm
#define BATFETOFF_HIZ				(1<<1)	// Control BATFET during HI-Z mode
#define PSYS_OTG_IDCHG				(1<<0)	// PSYS function during OTG mode

//TODO Add Prochot option bits

//ADC_OPTION
#define ADC_CONV					(1<<15)	// Coonversion mode of ADC
#define ADC_START					(1<<14)	// Start ADC conversion
#define ADC_FULLSCALE				(1<<13)	// ADC input voltage range
#define EN_ADC_CMPIN				(1<<7)
#define EN_ADC_VBUS					(1<<6)
#define EN_ADC_PSYS					(1<<5)
#define EN_ADC_IIN					(1<<4)
#define EN_ADC_IDCHG				(1<<3)
#define EN_ADC_ICHG					(1<<2)
#define EN_ADC_VSYS					(1<<1)
#define EN_ADC_VBAT					(1<<0)

//CHARGER_STATUS
#define AC_STAT						(1<<15)	// Input source status, same as CHRG_OK bit
#define ICO_DONE					(1<<14)	// ICO routine successfully executed
#define IN_VINDPM					(1<<12)
#define IN_IINDPM					(1<<11)
#define IN_FCHRG					(1<<10)	// Fast charge mode
#define	IN_PCHRG					(1<<9)	// Pre-charge mode
#define IN_OTG						(1<<8)	// OTG mode
#define FAULT_ACOV					(1<<7)
#define FAULT_BATOC					(1<<6)
#define FAULT_ACOC					(1<<5)
#define SYSOVP_STAT					(1<<4)	// SYSOVP status and clear
#define FAULT_LATCHOFF				(1<<2)
#define FAULT_OTG_OVP				(1<<1)
#define FAULT_OTG_UCP				(1<<0)

//TODO Add Prochot status register bits

//CHARGE_CURRENT
#define ADD_CHRG_CURRENT_4096		(1<<12)	// Add 4096 mA to charger current
#define ADD_CHRG_CURRENT_2048		(1<<11)	// Add 2048 mA to charger current
#define ADD_CHRG_CURRENT_1024		(1<<10)	// Add 1024 mA to charger current
#define ADD_CHRG_CURRENT_512		(1<<9)	// Add 512 mA to charger current
#define ADD_CHRG_CURRENT_256		(1<<8)	// Add 256 mA to charger current
#define ADD_CHRG_CURRENT_128		(1<<7)	// Add 128 mA to charger current
#define ADD_CHRG_CURRENT_64			(1<<6)	// Add 64 mA to charger current

//TODO ADD bits for MAX_CHARGE_VOLTAGE, Min_SYS_VOLTAGE, INPUT_LIMIT_HOST. The register values are determined by external circuitry

//TODO Add bits for INPUT_VOLTAGE

//OTG_VOLTAGE
#define OTG_VOLTAGE_ADD_8192		(1<<13)	// Add 8192mV to OTG Voltage
#define OTG_VOLTAGE_ADD_4096		(1<<12)	// Add 4096mV to OTG Voltage
#define OTG_VOLTAGE_ADD_2048		(1<<11)	// Add 2048mV to OTG Voltage
#define OTG_VOLTAGE_ADD_1024		(1<<10)	// Add 1024mV to OTG Voltage
#define OTG_VOLTAGE_ADD_512			(1<<9)	// Add 512mV to OTG Voltage
#define OTG_VOLTAGE_ADD_256			(1<<8)	// Add 256mV to OTG Voltage
#define OTG_VOLTAGE_ADD_128			(1<<7)	// Add 128mV to OTG Voltage
#define OTG_VOLTAGE_ADD_64			(1<<6)	// Add 64mV to OTG Voltage

//OTG_CURRENT
#define OTG_CURENT_ADD_3200			(1<<14)	// ADD 3200 mA to OTG current
#define OTG_CURENT_ADD_1600			(1<<13)	// ADD 1600 mA to OTG current
#define OTG_CURENT_ADD_800			(1<<12)	// ADD 800 mA to OTG current
#define OTG_CURENT_ADD_400			(1<<11)	// ADD 400 mA to OTG current
#define OTG_CURENT_ADD_200			(1<<10)	// ADD 200 mA to OTG current
#define OTG_CURENT_ADD_100			(1<<9)	// ADD 100 mA to OTG current
#define OTG_CURENT_ADD_50			(1<<8)	// ADD 50 mA to OTG current

/*
 * Option Bit Positions
 */
//CHARGE_OPTION_0
#define WDTMR_ADJ_pos				13		// Watchdog timer adjust

//CHARGE_OPTION_1
#define EN_PROCHOT_LPWR_pos			13
#define CMP_DEG_pos					4		// Set independent comparator deglitch time

//CHARGE_OPTION_2
#define PKPWR_TOVL_DEG_pos			14		// Input overload time in peak power mode
#define PKPWR_TMAX_pos				8		// Peak power mode relax and overload cycle time



#endif /* BMS_H_ */
