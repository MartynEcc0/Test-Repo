/**
  ******************************************************************************
  * @file    		receiver.c
  * @copyright  © 2018 ECCO Safety Group.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		Feb 2018
  * @brief   		A lightweight Matrix receiver for the bootloader.
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

#ifndef __RECEIVER_H
#define __RECEIVER_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_tokens.h"


#define RECEIVER_BUFFER_SIZE  302

/**
  * @brief  The receiver data object.
  */
typedef struct
{
	//	reading info file
	bool isReadingInfoFile;
	
	//	the message data
	uint8_t buffer[RECEIVER_BUFFER_SIZE] __attribute__((aligned(4)));
	
	//	the message data pointer
	uint8_t *pData;
	
	//	the message data size in the buffer
	uint16_t messageSize;

	//	the message source address
	uint16_t sourceAddress;


} RECEIVER_OBJECT;
extern RECEIVER_OBJECT Receiver;


/**
  * @brief  Resets and configures the receiver.
  * @param  None.
  * @retval None.
  */
extern void Receiver_Reset(void);

/**
  * @brief  Clocks the receiver.
  * @param  None.
  * @retval None.
  */
extern void Receiver_Clock(void);

/**
  * @brief  Receives a CAN frame from the bus.
	* @param  frame: A pointer to the CAN frame to receive.
  * @retval None.
  */
//extern void Bootloader_ReceiveCanFrame(ENET_CAN_FRAME *frame);

/**
  * @brief  Gets a value from the received data.
  * @param  valueSize: The value size.
  * @retval The value.
  */
extern uint32_t Receiver_GetValue(uint16_t valueSize);


#endif  //  __RECEIVER_H
