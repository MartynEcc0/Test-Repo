/**
  ******************************************************************************
  * @file    		transmitter.c
  * @copyright  © 2018 ECCO Safety Group.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		Feb 2018
  * @brief   		A lightweight Matrix transmitter for the bootloader.
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

#ifndef __TRANSMITTER_H
#define __TRANSMITTER_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_tokens.h"
#include "ecconet.h"


/**
  * @brief  The transmitter data object.
  */
typedef struct
{
	//	the message frame
	ENET_CAN_FRAME frame;
	
	//	the message data
	uint8_t buffer[262] __attribute__((aligned(4)));
	
	//	the message data pointer
	uint8_t *pData;
	
} TRANSMITTER_OBJECT;
extern TRANSMITTER_OBJECT Transmitter;


/**
  * @brief  Resets and configures the transmitter.
  * @param  None.
  * @retval None.
  */
extern void Transmitter_Reset(void);

/**
  * @brief  Resets the transmitter for a new message.
	* @param  destinationAddress: The destination address, or zero for broadcast.
  * @retval None.
  */
extern void Transmitter_StartMessage(uint8_t destinationAddress);

/**
  * @brief  Adds a value to the transmit buffer in big-endian format.
	* @param  value: The value to add.
  * @param  valueSize: The value size in bytes.
  * @retval None.
  */
extern void Transmitter_AddValueBigEndian(uint32_t value, uint16_t valueSize);

/**
  * @brief  Adds a string to the transmit buffer.
	* @param  string: The string to send.
	* @param  length: The string length, not including any zero termination.
  * @retval None.
  */
extern void Transmitter_AddString(const char *string, uint16_t length);

/**
  * @brief  Adds data to the transmit buffer.
	* @param  data: The data to send.
	* @param  dataSize: The data size.
  * @retval None.
  */
extern void Transmitter_AddData(void *data, uint16_t dataSize);

/**
  * @brief  Sends the transmitter buffer over the CAN bus.
	* @param  data: The data to send.
	* @param  dataSize: The data size.
	* @param  address: The destination address.
  * @retval None.
  */
extern void Transmitter_Finish(void);


//	messages.................................................................

/**
  * @brief  Sends data over the CAN bus.
	* @param  token: The token to send.
  *         size: The token value size in bytes.
  * @retval None.
  */
extern void Transmitter_SendToken(TOKEN *token, uint16_t size);

/**
  * @brief  Sends info file info or read reply.
  * @param  destinationAddress: The destination address.
  * @param  isInfo: True to respond to an info request.
  * @retval None.
  */
extern void Transmitter_SendInfoFileReply(uint8_t destinationAddress, bool isInfo);

/**
  * @brief  Sends info file segment reply.
  * @param  destinationAddress: The destination address.
  * @retval None.
  */
extern void Transmitter_SendInfoFileSegmentReply(uint8_t destinationAddress);



#endif  //  __TRANSMITTER_H
