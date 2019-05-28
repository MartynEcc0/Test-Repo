/**
  ******************************************************************************
  * @file    		matrix_transmitter.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Transmits Matrix messages over the CAN bus.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  * OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#ifndef __MATRIX_TRANSMITTER_H
#define __MATRIX_TRANSMITTER_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_tokens.h"
#include "matrix_config.h"


//	A two-frame byte fifo for look-ahead function.
#define CAN_TX_STREAM_FIFO_SIZE (2 * CAN_FRAME_MAX_NUM_BYTES)

/**
  * @brief  The Matrix transmitter CAN frame.
  */
typedef struct
{
	//	id field
	//	the top 4 bits are used for data size
	uint32_t id;

	//	the frame data
	uint8_t data[CAN_FRAME_MAX_NUM_BYTES];
	
} MATRIX_TX_CAN_FRAME;



/**
  * @brief  The Matrix transmitter data object.
  */
typedef struct
{
	//	message id-address field
	ECCONET_CAN_FRAME_ID idAddress;
	
	//	a two-frame byte fifo for look-ahead function
	uint8_t fifo[CAN_TX_STREAM_FIFO_SIZE];
	
	//	message bytes fifo index
	uint32_t fifoIndex;
	
	//	message sent bytes counter
	uint16_t numBytesSent;
	
	//	message bytes crc accumulator
	uint16_t crc;
	
	//	cyclic frame index
	uint16_t frameIndex;
	
	//	outgoing stream buffer
	MATRIX_TX_CAN_FRAME streamBuffer[CAN_TX_STREAM_BUFFER_SIZE];
	
	//	stream buffer write index
	uint16_t streamBufferWriteIndex;
	
	//	stream buffer read index
	uint16_t streamBufferReadIndex;

	
} MATRIX_TRANSMITTER;
//extern MATRIX_TRANSMITTER MatrixTransmitter;


/**
  * @brief  Resets the Matrix transmitter.
  * @param  None.
  * @retval None.
  */
extern void MatrixTransmitter_Reset(void);

/**
  * @brief  Clocks the Matrix transmitter.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
extern void MatrixTransmitter_Clock(void);

/**
  * @brief  Resets the transmitter for a new normal message type.
	* @param  destinationAddress: The destination address, or zero for broadcast.
  * @retval None.
  */
extern void MatrixTransmitter_StartMessage(uint8_t destinationAddress);

/**
  * @brief  Resets the transmitter and starts a new message for the given type.
	* @param  destinationAddress: The destination address, or zero for broadcast.
	* @param  key: A token key used to determine the message type.  Use KeyNull for normal messages.
  * @retval None.
  */
extern void MatrixTransmitter_StartMessageWithAddressAndKey(uint8_t destinationAddress, uint16_t key);

/**
  * @brief  Adds a byte to the transmit fifo and accumulates the crc.
	*					If the fifo is then full, sends a CAN frame.
	* @param  byte: The byte to add to the fifo.
  * @retval None.
  */
extern void MatrixTransmitter_AddByte(uint8_t byte);

/**
  * @brief  Adds a 16-bit value to the transmit fifo and accumulates the crc.
	*					If the fifo is then full, sends a CAN frame.
	* @param  value: The value to add to the fifo.
  * @retval None.
  */
extern void MatrixTransmitter_AddInt16(uint16_t value);

/**
  * @brief  Adds an 32-bit value to the transmit fifo and accumulates the crc.
	*					If the fifo is then full, sends a CAN frame.
	* @param  value: The value to add to the fifo.
  * @retval None.
  */
extern void MatrixTransmitter_AddInt32(uint32_t value);

/**
  * @brief  Adds a string up to 256 characters to the transmit fifo and accumulates the crc.
	*					CAUTION: Make sure string is null-terminated!
	*					If the fifo is then full, sends a CAN frame.
	* @param  str: The string to add to the fifo.
  * @retval None.
  */
extern void MatrixTransmitter_AddString(char *str);

/**
  * @brief  Adds a token to the transmit fifo.
	* @param  tokens: A pointer to a token to add.
  * @retval None.
  */
extern void MatrixTransmitter_AddToken(TOKEN *token);

/**
  * @brief  Sends any message bytes remaining in the transmit fifo.
	* @param  None.
  * @retval Returns zero on success, else -1.
  */
extern int MatrixTransmitter_FinishMessage(void);



#endif  //  __MATRIX_TRANSMITTER_H
