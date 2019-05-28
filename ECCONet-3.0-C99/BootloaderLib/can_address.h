/**
  ******************************************************************************
  * @file    		can_address.h
  * @copyright  © 2017-2018 ECCO Safety Group.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		Feb 2018
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

#ifndef __CAN_ADDRESS_H
#define __CAN_ADDRESS_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_tokens.h"
#include "bootloader_interface.h"




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
	BOOTLOADER_CAN_ADDRESS_STRUCT addressMode;
	
	
} CAN_ADDRESS_OBJECT;
extern CAN_ADDRESS_OBJECT CanAddress;



/**
  * @brief  Resets and configures the Matrix CAN address mechanism.
  * @param  None.
  * @retval None.
  */
extern void CanAddress_Reset(void);

/**
  * @brief  Clocks the Matrix CAN address mechanism.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
extern void CanAddress_Clock(void);

/**
  * @brief  Checks incoming tokens and senders for CAN address mechanism.
	* @param  token: A message token.
  * @retval None.
  */
extern void CanAddress_TokenIn(TOKEN *token);


#endif  //  __CAN_ADDRESS_H
