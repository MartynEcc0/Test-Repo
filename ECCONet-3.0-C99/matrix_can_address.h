/**
  ******************************************************************************
  * @file    		matrix_self_address.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		The Matrix mechanism for self-assigning CAN address.
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

#ifndef __MATRIX_SELF_ADDRESS_H
#define __MATRIX_SELF_ADDRESS_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_lib_interface.h"
#include "matrix_tokens.h"
#include "matrix_transmitter.h"
#include "matrix_receiver.h"




/**
  * @brief  The Matrix can address data object.
  */
typedef struct
{
	//	self-address guid index
	uint16_t xorIndex;
	
	//	self-address address offset
	uint16_t addressOffset;
	
	//	self-address timer
	uint32_t requestTime;
	
	//	proposed address
	uint8_t proposedAddress;
	
	//	the device address and static flag
	MATRIX_CAN_ADDRESS_FILE_OBJECT file;
	
} MATRIX_CAN_ADDRESS_OBJECT;
extern MATRIX_CAN_ADDRESS_OBJECT MatrixCanAddress;



/**
  * @brief  Resets and configures the Matrix CAN address mechanism.
  * @param  None.
  * @retval None.
  */
extern void MatrixCanAddress_Reset(void);

/**
  * @brief  Clocks the Matrix CAN address mechanism.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
extern void MatrixCanAddress_Clock(void);

/**
  * @brief  Checks incoming tokens and senders for CAN address mechanism.
	* @param  token: A message token.
  * @retval None.
  */
extern void MatrixCanAddress_CanTokenIn(TOKEN token);


#endif  //  __MATRIX_SELF_ADDRESS_H
