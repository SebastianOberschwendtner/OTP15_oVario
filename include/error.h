/**
 ******************************************************************************
 * @file    error.h
 * @author  JK
 * @version V1.0
 * @date    10-March-2018
 * @brief   The global error variable and defines.
 ******************************************************************************
 */

#ifndef ERROR_H_
#define ERROR_H_


//*********** Errors **************
#define err_no_memory_left  (1<<0)
#define err_queue_overrun	(1<<1)
#define err_bms_fault		(1<<2)
#define err_coloumb_fault	(1<<3)
#define err_baro_fault		(1<<4)
#define err_sd_fault		(1<<5)

//Bit defines
#define SENSOR_ERROR_BMS		(1<<3)
#define SENSOR_ERROR_GAUGE		(1<<7)
#define SENSOR_ERROR			(1<<7)







#endif /* ERROR_H_ */
