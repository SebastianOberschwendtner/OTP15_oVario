/*
 * igc.h
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
 */

#ifndef IGC_H_
#define IGC_H_
//*********** Includes **************
#include "oVario_Framework.h"
#include "Variables.h"
#include "sdio.h"
#include "ipc.h"
#include "did.h"

//*********** Defines **************
#define IGC_CODE		"XCS"	//The igc code has to mimic the XCSOAR header

//*********** Functions **************
unsigned char igc_IsValidCharacter(unsigned char character);
unsigned char igc_IncludeInGrecord(char* in);
void igc_create(void);


#endif /* IGC_H_ */
