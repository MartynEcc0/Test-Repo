/**
  ******************************************************************************
  * @file    		matrix_crc.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		The crc calculation used in the Matrix messages.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  * OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#include <stdbool.h>
#include "matrix_config.h"
#include "matrix_crc.h"


/**
  * @brief  Computes a Matrix message CRC.
	* @param  bytes: A pointer to one or more data bytes.
	* @param  numBytes: The number of data bytes to include in the crc.
  * @retval Returns the computed CRC.
  */
uint16_t Matrix_ComputeCRC16(uint8_t *bytes, uint32_t numBytes)
{
	uint16_t crc;
	
	//	validate inputs
	if ((NULL == bytes) || (0 == numBytes))
		return 0;
	
	crc = MATRIX_MESSAGE_CRC_INIT_VALUE;
	while (numBytes--)
		Matrix_AddByteToCRC16(*bytes++, &crc);
	return crc;
}

/**
  * @brief  Adds a byte to a Matrix message CRC.
	* @param  byte: A byte to add to the CRC.
	* @param  crc: A pointer to a crc accumulator.
  * @retval None.
  */
void Matrix_AddByteToCRC16(uint8_t byte, uint16_t *crc)
{
	uint16_t bit;

	//	validate input
	if (NULL == crc)
		return;
	
	bit = 8;
	while (bit--)
	{
		*crc = ((byte ^ (uint8_t)*crc) & 1) ?
			((*crc >> 1) ^ (uint16_t)MATRIX_MESSAGE_CRC_POLY_VALUE) : (*crc >> 1);
		byte >>= 1;
	}
}

/**
  * @brief  Determines if a Matrix message byte stream is valid.
	* @param  bytes: A pointer the the message bytes.
	* @param  numBytes: The number of bytes in the message, including any checksum.
  * @retval None.
  */
bool Matrix_IsMessageChecksumValid(uint8_t *bytes, uint32_t numBytes)
{
	uint16_t messageCrc; 
	
	//	validate inputs
	if ((NULL == bytes) || (0 == numBytes))
		return false;
	
	/*
	//	one-frame messages do not include a checksum
	if (numBytes <= CAN_FRAME_MAX_NUM_BYTES)
		return true;
	*/
	
	//	compare checksums
	messageCrc = bytes[numBytes - 2];
	messageCrc = (messageCrc << 8) | bytes[numBytes - 1];
	return (messageCrc == Matrix_ComputeCRC16(bytes, numBytes - 2));
}
