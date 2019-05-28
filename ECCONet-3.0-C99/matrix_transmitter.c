/**
  ******************************************************************************
  * @file    		matrix_transmitter.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Transmits Matrix messages over the CAN bus.
	*
	*							Buffering the compressed outgoing messages allows a look-ahead
	*							for the last message packet, and is efficient use of memory as
	*							compared to buffering raw tokens or whole CAN frames.
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
#include "matrix.h"
#include "matrix_crc.h"
#include "matrix_codec.h"
#include "matrix_event_index.h"
#include "matrix_transmitter.h"


//	private methods
static int SendFrame(void);



/**
  * @brief  The Matrix transmitter data object.
  */
MATRIX_TRANSMITTER MatrixTransmitter;

/**
  * @brief  Resets the Matrix transmitter.
  * @param  None.
  * @retval None.
  */
void MatrixTransmitter_Reset(void)
{
	//	Reset the frame index and stream buffer indices.
	//	The other members are set by the StartMessage method.
	MatrixTransmitter.frameIndex = 0;
	MatrixTransmitter.streamBufferWriteIndex = 0;
	MatrixTransmitter.streamBufferReadIndex = 0;
}

/**
  * @brief  Clocks the Matrix transmitter.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
void MatrixTransmitter_Clock(void)
{
	MATRIX_TX_CAN_FRAME *frame;
	uint8_t dataSize;
	uint32_t id;

	//	validate the app interface
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->sendCanFrame))
	{
		//	validate stream buffer indices
		if (MatrixTransmitter.streamBufferWriteIndex >= CAN_TX_STREAM_BUFFER_SIZE)
			MatrixTransmitter.streamBufferWriteIndex = 0;
		if (MatrixTransmitter.streamBufferReadIndex >= CAN_TX_STREAM_BUFFER_SIZE)
			MatrixTransmitter.streamBufferReadIndex = 0;

		//	if frames in the stream to send	
		if (MatrixTransmitter.streamBufferReadIndex !=	MatrixTransmitter.streamBufferWriteIndex)
		{
			frame = &MatrixTransmitter.streamBuffer[MatrixTransmitter.streamBufferReadIndex];
			id = (frame->id & 0x0FFFFFFF) | 0x10000000;
			dataSize = frame->id >> 28;
			if (0 == Matrix.appInterface->sendCanFrame(id, frame->data, dataSize))
				++MatrixTransmitter.streamBufferReadIndex;
		}
	}
}	

/**
  * @brief  Resets the transmitter and starts a new message.
	* @param  destinationAddress: The destination address, or zero for broadcast.
  * @retval None.
  */
void MatrixTransmitter_StartMessage(uint8_t destinationAddress)
{
	MatrixTransmitter_StartMessageWithAddressAndKey(destinationAddress, KeyNull);
}

/**
  * @brief  Resets the transmitter and starts a new message for the given type.
	* @param  destinationAddress: The destination address, or zero for broadcast.
	* @param  key: A token key used to determine the message type.  Use KeyNull for normal messages.
  * @retval None.
  */
void MatrixTransmitter_StartMessageWithAddressAndKey(uint8_t destinationAddress, uint16_t key)
{
    uint32_t zero = 0;
	//	reset the fifo
	MatrixTransmitter.fifoIndex = 0;
	MatrixTransmitter.crc = 0;
	MatrixTransmitter.numBytesSent = 0;
	//*(uint32_t *)&MatrixTransmitter.idAddress = 0;
    memcpy(&MatrixTransmitter.idAddress, &zero, sizeof(uint32_t)); 

	//	add the destination address
	MatrixTransmitter.idAddress.destinationAddress = destinationAddress;

	//	add this device source address
	MatrixTransmitter.idAddress.sourceAddress = Matrix_GetCanAddress();
	
	//	add the first frame id
	MatrixTransmitter.idAddress.frameType = MATRIX_MESSAGE_FRAME_TYPE_BODY;
	
	//	determine the message type for event index and flag
	//
	//	if an address negotiation message
	if ((key == KeyRequestAddress) || (key == KeyResponseAddressInUse))
	{
		//	add event index zero
		MatrixTransmitter_AddByte(0);
	}
	//
	//	else an input our output event
	else if ((KeyPrefix_InputStatus == Key_GetPrefix(key))
		|| (KeyPrefix_OutputStatus == Key_GetPrefix(key)))
	{
		//	set event flag and add the current event index
		MatrixTransmitter.idAddress.isEvent = true;
		MatrixTransmitter_AddByte(Matrix_GetEventIndex());
	}
	//
	//	else any other type of message
	else
	{
		//	add the current event index
		MatrixTransmitter_AddByte(Matrix_GetEventIndex());
	}		
}

/**
  * @brief  Adds a byte to the transmit fifo and accumulates the crc.
	*					If the fifo is then full, sends a CAN frame.
	* @param  byte: The byte to add to the fifo.
  * @retval None.
  */
void MatrixTransmitter_AddByte(uint8_t byte)
{
	//	accumulate crc
	Matrix_AddByteToCRC16(byte, &MatrixTransmitter.crc);
	
	//	add the byte to the fifo
	MatrixTransmitter.fifo[MatrixTransmitter.fifoIndex] = byte;
	
	//	if fifo is full, then send CAN frame
	if (CAN_TX_STREAM_FIFO_SIZE <= ++MatrixTransmitter.fifoIndex)
		SendFrame();
}

/**
  * @brief  Adds a 16-bit value to the transmit fifo and accumulates the crc.
	*					If the fifo is then full, sends a CAN frame.
	* @param  value: The value to add to the fifo.
  * @retval None.
  */
void MatrixTransmitter_AddInt16(uint16_t value)
{
	MatrixTransmitter_AddByte(value >> 8);
	MatrixTransmitter_AddByte(value);
}

/**
  * @brief  Adds a 32-bit value to the transmit fifo and accumulates the crc.
	*					If the fifo is then full, sends a CAN frame.
	* @param  value: The value to add to the fifo.
  * @retval None.
  */
void MatrixTransmitter_AddInt32(uint32_t value)
{
	MatrixTransmitter_AddByte(value >> 24);
	MatrixTransmitter_AddByte(value >> 16);
	MatrixTransmitter_AddByte(value >> 8);
	MatrixTransmitter_AddByte(value);
}

/**
  * @brief  Adds a token to the transmit fifo.
	* @param  tokens: A pointer to a token to add.
  * @retval None.
  */
void MatrixTransmitter_AddToken(TOKEN *token)
{
	uint16_t valueSize;

	MatrixTransmitter_AddByte(token->key >> 8);
	MatrixTransmitter_AddByte(token->key);
	valueSize = (KeyPrefix_PatternSync == Key_GetPrefix(token->key)) ? 1 : Key_ValueSize(token->key);
	while (valueSize--)
		MatrixTransmitter_AddByte(token->value >> (8 * valueSize));
}

/**
  * @brief  Adds a string up to 256 characters to the transmit fifo and accumulates the crc.
	*					CAUTION: Make sure string is null-terminated!
	*					If the fifo is then full, sends a CAN frame.
	* @param  str: The string to add to the fifo.
  * @retval None.
  */
void MatrixTransmitter_AddString(char *str)
{
	uint16_t i = 0;
	do
		MatrixTransmitter_AddByte(*str);
	while (*str++ && (++i <= 256));
}

/**
  * @brief  Sends any message bytes remaining in the transmit fifo.
	* @param  None.
  * @retval Returns zero on success, else -1.
  */
int MatrixTransmitter_FinishMessage(void)
{
	uint16_t numBytesToSend, crc;
	int status;
	bool isSingleFrame;
	
	//	determine if is single frame message
	isSingleFrame = (8 >= (MatrixTransmitter.numBytesSent + MatrixTransmitter.fifoIndex));
	
	//	if more than one frame to send, then add checksum
	if (!isSingleFrame)
	{
		//	get crc in local variable because AddByte updates the crc
		crc = MatrixTransmitter.crc;
		MatrixTransmitter_AddByte(crc >> 8);
		MatrixTransmitter_AddByte(crc);
	}
	
	//	while bytes in fifo
	status = 0;
	while (0 != MatrixTransmitter.fifoIndex)
	{
		//	get number of bytes to send
		numBytesToSend = (MatrixTransmitter.fifoIndex <=	CAN_FRAME_MAX_NUM_BYTES) ?
			MatrixTransmitter.fifoIndex : CAN_FRAME_MAX_NUM_BYTES;
		
		//	flag last frame
		if (MatrixTransmitter.fifoIndex == numBytesToSend)
		{
			if (isSingleFrame)
				MatrixTransmitter.idAddress.frameType = MATRIX_MESSAGE_FRAME_TYPE_SINGLE;
			else
				MatrixTransmitter.idAddress.frameType = MATRIX_MESSAGE_FRAME_TYPE_LAST;
		}

		//	send the frame
		status = SendFrame();
		if (0 != status)
			return status;
	}
	return status;
}


//	private methods...........................................................

/**
  * @brief  Takes up to 8 bytes from the fifo, creates the next indexed frame,
  *					and adds the frame to the transmit buffer.
	* @param  None.
  * @retval Returns 0 on success, else -1.
  */
static int SendFrame(void)
{
	MATRIX_TX_CAN_FRAME *frame;
	uint8_t dataSize;
	int status = 0;
    uint32_t idAddress;
	
	//	get number of bytes to send
	dataSize =
		(MatrixTransmitter.fifoIndex <= CAN_FRAME_MAX_NUM_BYTES) ?
		MatrixTransmitter.fifoIndex : CAN_FRAME_MAX_NUM_BYTES;
	if (0 == dataSize)
		return -1;
	
	//	validate the write index
	if (MatrixTransmitter.streamBufferWriteIndex >= CAN_TX_STREAM_BUFFER_SIZE)
		MatrixTransmitter.streamBufferWriteIndex = 0;

	//	put the frame in the transmit buffer
	//	if the write index overtakes the read index, then old message frames are discarded
	frame = &MatrixTransmitter.streamBuffer[MatrixTransmitter.streamBufferWriteIndex];
    memcpy(&idAddress, &MatrixTransmitter.idAddress, sizeof(uint32_t));
	frame->id = (idAddress & 0x0FFFFFFF) | (((uint32_t)dataSize) << 28)
		| MatrixTransmitter.frameIndex;
	memcpy(frame->data, MatrixTransmitter.fifo, dataSize);
	++MatrixTransmitter.streamBufferWriteIndex;
	
	//	update number of bytes sent and the frame index
	MatrixTransmitter.numBytesSent += dataSize;
	MatrixTransmitter.frameIndex = (MatrixTransmitter.frameIndex + 1) &
		MATRIX_CAN_ID_FRAME_INDEX_MASK;
	
	//	move index and shift any remaining bytes in fifo
	MatrixTransmitter.fifoIndex -= dataSize;
	if (0 != MatrixTransmitter.fifoIndex)
	{
		memmove(MatrixTransmitter.fifo,	MatrixTransmitter.fifo + dataSize,
			MatrixTransmitter.fifoIndex);
	}
	
	//	return status
	return status;
}
