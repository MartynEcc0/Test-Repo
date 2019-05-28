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

#ifndef __MATRIX_CRC_H
#define __MATRIX_CRC_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>



/**
  * @brief  Computes a Matrix message CRC.
	* @param  data: A pointer to the data.
	* @param  size: The data size.
  * @retval Returns the computed CRC.
  */
extern uint16_t Matrix_ComputeCRC16(uint8_t *bytes, uint32_t numBytes);

/**
  * @brief  Adds a byte to a Matrix message CRC.
	* @param  byte: A byte to add to the CRC.
	* @param  crc: A pointer to a crc accumulator.
  * @retval None.
  */
extern void Matrix_AddByteToCRC16(uint8_t byte, uint16_t *crc);

/**
  * @brief  Determines if a Matrix message byte stream is valid.
	* @param  bytes: A pointer the the message bytes.
	* @param  numBytes: The number of bytes in the message, including any checksum.
  * @retval None.
  */
extern bool Matrix_IsMessageChecksumValid(uint8_t *bytes, uint32_t numBytes);




#endif  //  __MATRIX_CRC_H
