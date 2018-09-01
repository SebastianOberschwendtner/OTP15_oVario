/*
 * logging.h
 *
 *  Created on: 01.09.2018
 *      Author: Sebastian
 */

#ifndef LOGGING_H_
#define LOGGING_H_

//*********** Includes **************
#include "stm32f4xx.h"
#include "oVario_Framework.h"
#include "did.h"
#include "ipc.h"
#include "error.h"
#include "Variables.h"

//*********** Defines **************
//filename defines
#define LOG_NAME			"LOG     "
#define LOG_EXTENSION		"JOV"
#define LOG_NUMBER_START	3
#define LOG_NUMBER_LENGTH	5

//*********** Functions **************
void log_create(void);
void log_finish(void);

#endif /* LOGGING_H_ */
