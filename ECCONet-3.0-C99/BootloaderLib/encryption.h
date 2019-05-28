/**
  ******************************************************************************
  * @file    		encryption.h
  * @copyright  © 2018 ECCO Safety Group.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		Feb 2018
  * @brief   		Encryption and decryption methods.
	*							
  *             NOTICE:
  *
  *             THIS IS A SYMMETRIC KEY SCHEME AND IS NOT NEARLY AS SECURE AS
  *             A PUBLIC KEY SCHEME SUCH AS RSA OR AES.
  *    
  *             !!! THE METHODS AND CODES HEREIN ARE HIGHLY CONFIDENTIAL !!!
	*
	*
	******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  * OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#ifndef __ENCRYPTION_H
#define __ENCRYPTION_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
  * @brief  The encryption data object.
  */
typedef struct
{
	//	the device guid
	uint32_t deviceGuid[4];
	
} ENCRYPTION_OBJECT;
extern ENCRYPTION_OBJECT Encryption;


/**
	* @brief  Resets the encryption object.
	* @param  None.
  * @retval None.
  */
extern void Encryption_Reset(void);

/**
  * @brief  Computes am ECCONet message CRC.
	* @param  bytes: A pointer to one or more data bytes.
	* @param  numBytes: The number of data bytes to include in the crc.
  * @retval Returns the computed CRC.
  */
extern uint16_t Encryption_ComputeCRC16(uint8_t *bytes, uint32_t numBytes);

/**
  * @brief  Calculates CRC32/BZIP2 with poly=0x04C11DB7, no reflection,
  *					start=~0, output inverted (to match ELFfile.crc32)
	* @param  data: The data to perform the CRC on.
	* @param  length: The data length in bytes.
  * @retval Returns the CRC32 result.
  */
//extern uint32_t Bootloader_ComputeCRC32(uint8_t *data, uint32_t length);

/**
	* @brief  Generates the access code.
	* @param  guid: An array of for uint32_t that are the 128-bit device guid.
  * @retval The server access code.
  */
extern uint32_t Encryption_GetAccessCode(void);

/**
  * @brief  Encrypts or decrypts a set of data.
  * @param  data: A pointer to the data to encrypt.  Must be on 4-byte boundary.
  * @param  dataSize: The size of the data to encrypt in bytes.
  * @retval None.
  */
extern void Encryption_Encrypt(uint8_t *data, int32_t dataSize);



#endif  //  __ENCRYPTION_H
