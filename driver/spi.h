/*
 * spi.h
 *
 *  Created on: 27.02.2018
 *      Author: Sebastian
 */

#ifndef SPI_H_
#define SPI_H_
#include "oVario_Framework.h"

void init_spi(void);
void spi_send_char(unsigned char ch_data);

#endif /* SPI_H_ */
