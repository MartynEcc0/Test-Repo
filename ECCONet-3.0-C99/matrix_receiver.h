/**
  ******************************************************************************
  * @file    		matrix_receiver.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Receives Matrix messages on the CAN bus.
	*
	*							CAN frames arrive asynchronously via callback interrupt that
	*							places them in the front portion of a stream buffer.
	*
	*							The app thread periodically disables the callback interrupt,
	*							moves the	received frames to back of the buffer, and then
	*							re-enables the	interrupt. The frames are then examined, and
	*							any	complete messages are decompressed and sent to the router.
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

#ifndef __MATRIX_RECEIVER_H
#define __MATRIX_RECEIVER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_config.h"

//	Total stream buffer size
#define CAN_RX_STREAM_BUFFER_SIZE	\
	(CAN_RX_STREAM_BUFFER_FRONT_SIZE + CAN_RX_STREAM_BUFFER_BACK_SIZE)



/**
  * @brief  The Matrix receiver mutex modes.
  */
typedef enum
{
	ReceiverMutexModeFree,
	ReceiverMutexModeRx,
	ReceiverMutexModeProcess,
	
} ReceiverMutexModes;

/**
  * @brief  The Matrix receiver frame flag enumeration.
  */
	/* previous
	MR_FRAME_FLAG_BODY,
	MR_FRAME_FLAG_LAST,
	MR_FRAME_FLAG_SINGLE,
	*/
typedef enum
{
	MR_FRAME_FLAG_NONE,
	MR_FRAME_FLAG_SINGLE,
	MR_FRAME_FLAG_BODY,
	MR_FRAME_FLAG_LAST,
	
} MR_FRAME_FLAGS;

	
	

/**
  * @brief  The Matrix receiver CAN frame.
  */
typedef struct __attribute__((__packed__))
{
	//	the sender address
	uint32_t senderAddress;

	//	the sender frame index and last frame flag
	uint32_t frameIndex;
	
	//	frame flags
	uint32_t frameFlags;

	//	event flag
	uint32_t isEvent;

	//	the number of frame data bytes
	uint32_t dataSize;
	
	//	the frame timestamp
	uint32_t timeStamp;

	//	the frame data
	uint8_t data[CAN_FRAME_MAX_NUM_BYTES];
	
} MATRIX_RX_CAN_FRAME;


/**
  * @brief  The Matrix receiver data object.
  */
typedef struct
{
	//	stream buffer
	MATRIX_RX_CAN_FRAME streamBuffer[CAN_RX_STREAM_BUFFER_FRONT_SIZE];
	
	//	CAN interrupt stream buffer
	MATRIX_RX_CAN_FRAME rxBuffer[CAN_RX_STREAM_BUFFER_BACK_SIZE];
	
	//	the incoming frame write index
	volatile uint16_t rxBufferWriteIndex;
	
	//	the incoming frame read index
	volatile uint16_t rxBufferReadIndex;
	
	//	the address filter timeout
	uint32_t senderAddressFilterTimeout;
	
	//	the sender address filter
	//	this is normally zero to receive messages from all senders
	uint8_t senderAddressFilter;
	
	//	number of frames read 0
	uint8_t numFramesRead0;

	//	number of frames read 1
	uint8_t numFramesRead1;

	
} MATRIX_RECEIVER;
//extern MATRIX_RECEIVER MatrixReceiver;


/**
  * @brief  Resets the Matrix receiver.
  * @param  None.
  * @retval None.
  */
extern void MatrixReceiver_Reset(void);

/**
  * @brief  Clocks the receiver.
  * @param  None.
  * @retval None.
  */
extern void MatrixReceiver_Clock(void);

/**
  * @brief  Set the receiver sender address filter.
	*					The sender address filter is normally zero to receive message from all senders,
	*					and is reset to zero periodically.
  * @param  None.
  * @retval None.
  */
extern void MatrixReceiver_SetSenderAddressFilter(uint8_t senderAddressFilter);

#endif  //  __MATRIX_RECEIVER_H



#if 0
/**
  * @brief  The CAN Rx callback.  Made public for testing.
	* @param  msg: The received message.
  * @retval None.
  */
extern void MatrixReceiver_RxCallback(CAN_MSG_OBJ *msg);
#endif

