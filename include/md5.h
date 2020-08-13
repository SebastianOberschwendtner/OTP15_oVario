/*
 * md5.h
 *
 *  Created on: 02.09.2018
 *      Author: Sebastian
 */

#ifndef MD5_H_
#define MD5_H_
//*********** Includes **************
#include "oVario_Framework.h"

//*********** defines for hash **************


//*********** Functions **************
void md5_initialize(MD5_T* hash, unsigned long* key);
unsigned long md5_leftrotate(unsigned long x, unsigned long c);
void md5_append_char(MD5_T* hash, unsigned char character);
void md5_process512(MD5_T* hash);
void md5_WriteLength(MD5_T* hash);
void md5_finalize(MD5_T* hash);
void md5_GetDigest(MD5_T* hash, char* digest);


#endif /* MD5_H_ */