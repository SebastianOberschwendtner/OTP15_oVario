/**
 ******************************************************************************
 * @file    datafusion.h
 * @author  JK
 * @version V1.1
 * @date    21-March-2018
 * @brief   Handles the datafusion between the GPS the barometer and the vario
 * 			task.
 ******************************************************************************
 */

#ifndef DATAFUSION_H_
#define DATAFUSION_H_

#include "oVario_Framework.h"

// ***** Defines *****
#define climbavtime 15

// ***** Functions *****
void    datafusion_register_ipc    (void);
void    datafusion_get_ipc         (void);
void    datafusion_init            (void);
void    datafusion_task            (void);


void    wind_grad                  (void);
void    wind_collect_samples       (void);
int8_t  wind_turn_dir            (float hdg_last, float hdg);

float   df_sqr                    (float x);
float   df_abs                    (float x);
float   df_min                    (float x, float y);



#endif /* DATAFUSION_H_ */
