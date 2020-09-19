/**
 ******************************************************************************
 * @file    md5.h
 * @author  SO
 * @version V2.0
 * @date    19-September-2020
 * @brief   Calculates the md5-hash. Uses the IPC command interface.
 ******************************************************************************
 */

#ifndef MD5_H_
#define MD5_H_
//*********** Includes **************
#include "oVario_Framework.h"

//*********** defines for task **************
//Commands
#define MD5_CMD_APPEND_CHAR         1
#define MD5_CMD_APPEND_STRING       2
#define MD5_CMD_FINALIZE            3

//Sequences for commands

//*********** defines for hash **************


//*********** Functions **************
void            md5_task            (void);
void            md5_check_commands            (void);
void            md5_register_ipc    (void);
void            md5_initialize      (MD5_T* hash, unsigned long* key);
unsigned long   md5_leftrotate      (unsigned long x, unsigned long c);
void            md5_append_char     (void);
void            md5_append_string   (void);
void            md5_process512      (MD5_T* hash);
void            md5_WriteLength     (MD5_T* hash);
void            md5_finalize        (MD5_T* hash);
void            md5_GetDigest       (MD5_T* hash, char* digest);


#endif /* MD5_H_ */
