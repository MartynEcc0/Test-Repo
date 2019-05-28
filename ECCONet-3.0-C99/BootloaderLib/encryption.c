/**
  ******************************************************************************
  * @file    		bootloader_encryption.h
  * @copyright  © 2018 ECCO Safety Group.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		Feb 2018
  * @brief   		Bootloader encryption and decryption methods.
	*							
  *             NOTICE:
  *
  *             THIS IS A SYMMETRIC KEY SCHEME AND IS NOT NEARLY AS SECURE AS
  *             A PUBLIC KEY SCHEME SUCH AS RSA OR AES.
  *    
  *             !!! THE METHODS AND CODES HEREIN ARE HIGHLY CONFIDENTIAL !!!
	*
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

#include <string.h>
#include "bootloader.h"
#include "bootloader_interface.h"
#include "encryption.h"



//	the CRC defines for Matrix messages
#define MATRIX_MESSAGE_CRC_INIT_VALUE						     0
#define MATRIX_MESSAGE_CRC_POLY_VALUE						0xA001

//	the server access poly
#define MATRIX_SERVER_ACCESS_POLY						0x5EB9417D


/**
  * @brief  The encryption data object.
  */
ENCRYPTION_OBJECT Encryption;

/**
	* @brief  Resets the encryption object.
	* @param  None.
  * @retval None.
  */
void Encryption_Reset(void)
{
	//	get the device guid
	if (NULL != Bootloader.appInterface->get128BitGuid)
		Bootloader.appInterface->get128BitGuid(Encryption.deviceGuid);
	else
		memcpy(Encryption.deviceGuid, &Encryption_ComputeCRC16, 16);
}

/**
  * @brief  Computes am ECCONet message CRC.
	* @param  bytes: A pointer to one or more data bytes.
	* @param  numBytes: The number of data bytes to include in the crc.
  * @retval Returns the computed CRC.
  */
uint16_t Encryption_ComputeCRC16(uint8_t *bytes, uint32_t numBytes)
{
	uint8_t byte;
	uint16_t crc, bit;
	
	//	validate inputs
	//if ((NULL == bytes) || (0 == numBytes))
	//	return 0;
	
	crc = MATRIX_MESSAGE_CRC_INIT_VALUE;
	while (numBytes--)
	{
		byte = *bytes++;
		bit = 8;
		while (bit--)
		{
			crc = ((byte ^ (uint8_t)crc) & 1) ?
				((crc >> 1) ^ (uint16_t)MATRIX_MESSAGE_CRC_POLY_VALUE) : (crc >> 1);
			byte >>= 1;
		}
	}
	return crc;
}

/**
  * @brief  Calculates CRC32/BZIP2 with poly=0x04C11DB7, no reflection,
  *					start=~0, output inverted (to match ELFfile.crc32)
	* @param  data: The data to perform the CRC on.
	* @param  length: The data length in bytes.
  * @retval Returns the CRC32 result.
  */
uint32_t Bootloader_ComputeCRC32(uint8_t *data, uint32_t length)
{
	uint32_t i;
	uint32_t crc = 0xffffffff;
	while (length--)
	{
		crc ^= ((uint32_t)(*data) << 24);
		data++;
		for (i = 8 ; i-- ; )
			crc = (crc & 0x80000000) ? (crc << 1) ^ 0x04C11DB7 : (crc << 1);
	}
	return ~crc;
}

/**
	* @brief  Generates the access code.
	* @param  guid: An array of for uint32_t that are the 128-bit device guid.
  * @retval The server access code.
  */
uint32_t Encryption_GetAccessCode(void)
{
	uint32_t value;

	//	run hash and return code
	value = Encryption.deviceGuid[0] ^ Encryption.deviceGuid[3];
	value >>= (int32_t)((Encryption.deviceGuid[0] >> 3) & 3);
	value ^= Encryption.deviceGuid[2];
	value ^= MATRIX_SERVER_ACCESS_POLY;
	value ^= Encryption.deviceGuid[1];
	return value;
}

/**
  * @brief  Encrypts or decrypts a set of data.
  * @param  data: A pointer to the data to encrypt.
  * @param  dataSize: The size of the data to encrypt in bytes.
  * @retval None.
  */
void Encryption_Encrypt(uint8_t *data, int32_t dataSize)
{
	uint16_t i;
	uint8_t convolutedKeys[16];
	
	//	get convoluted keys
	for (i = 0; i < 16; ++i)
		convolutedKeys[i] = (Encryption.deviceGuid[i >> 2] ^ 0x90208f7f) >> ((i >> 2) << 3);
	
	//	for all data bytes
	for (i = 0; i < dataSize; ++i)
		*data++ ^= convolutedKeys[(i ^ (convolutedKeys[(i >> 4) & 0x0f])) & 0x0f];
}












#ifdef ENCRYPTION_TEST

const char people[] = "Thank you to my good friends who made me this nice birthday cake!";
uint16_t test22;

void EncryptionTest(void)
{
	uint8_t data[65];
	
	uid = FlashApiSmall_ReadUniqueID();

	for (i = 0; i < 65; ++i)
		data[i] = people[i];
		
	test22 = 0;
	
	Encryption_Encrypt(data, 65, uid);
	
	test22 = 0;
	
	Encryption_Encrypt(data, 65, uid);
	
	test22 = 0;
}
#endif



