/**
  ******************************************************************************
  * @file    		matrix_receiver.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Receives Matrix messages on the CAN bus.
	*
	*							Incoming CAN frames arrive asynchronously via CAN API callback
	*							that places them in the back region of a stream buffer.
	*
	*							The app thread periodically disables the callback interrupt,
	*							moves the	received frames to the front region of the buffer,
	*							and then re-enables the	interrupt.  The frames are examined,
	*							and complete messages are decompressed and sent to router.
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
#include "matrix_ftp_client.h"
#include "matrix_ftp_server.h"
#include "matrix_event_index.h"
#include "matrix_token_sequencer.h"
#include "matrix_receiver.h"


//	private methods
extern void Matrix_PrivateReceiveCanToken(TOKEN *token);
extern void Matrix_DelayStatusUpdate15mS(void);
static void ProcessMessagesInStream(uint16_t numNewFrames);
static void RemoveUnprocessedFrames(void);
static void SortNewFrames(uint16_t numNewFrames);


/**
  * @brief  The Matrix receiver data object.
  */
MATRIX_RECEIVER MatrixReceiver;


/**
  * @brief  Resets the Matrix receiver.
  * @param  None.
  * @retval None.
  */
void MatrixReceiver_Reset(void)
{
	//	clear the stream
	memset(MatrixReceiver.streamBuffer, 0,
		CAN_RX_STREAM_BUFFER_FRONT_SIZE * sizeof(MATRIX_RX_CAN_FRAME));

	//	clear the rxBuffer
	memset(MatrixReceiver.rxBuffer, 0,
		CAN_RX_STREAM_BUFFER_BACK_SIZE * sizeof(MATRIX_RX_CAN_FRAME));

	//	the the stream indices and block
	MatrixReceiver.rxBufferWriteIndex = 0;
	MatrixReceiver.rxBufferReadIndex = 0;

	//	clear the sender address filter
	MatrixReceiver.senderAddressFilter = 0;
	
	//	start the sender address filter timer
	//	each time this timer expires it automatically re-starts
	MatrixReceiver.senderAddressFilterTimeout =
		Matrix.systemTime + MATRIX_MAX_SENDER_ADDRESS_FILTER_TIME_MS;
}

/**
  * @brief  Clocks the receiver.
  * @param  None.
  * @retval None.
  */
void MatrixReceiver_Clock(void)
{
	uint16_t numNewFrames;
	MATRIX_RX_CAN_FRAME *pFrontBuffer, *pFrontBufferEnd;
	
	
	//	check the sender address filter timeout
	if (IsMatrixTimerExpired(MatrixReceiver.senderAddressFilterTimeout))
	{
		//	set next timeout and clear address filter
		MatrixReceiver.senderAddressFilterTimeout =
			Matrix.systemTime + MATRIX_MAX_SENDER_ADDRESS_FILTER_TIME_MS;
		MatrixReceiver.senderAddressFilter = 0;
	}

	//	validate uint rxBufferReadIndex as less than the back buffer size
	if (MatrixReceiver.rxBufferReadIndex >= CAN_RX_STREAM_BUFFER_BACK_SIZE)
		MatrixReceiver.rxBufferReadIndex = 0;
		
	//	check for received frames
	numNewFrames = MatrixReceiver.rxBufferWriteIndex - MatrixReceiver.rxBufferReadIndex;
	while (numNewFrames > CAN_RX_STREAM_BUFFER_BACK_SIZE)
		numNewFrames += CAN_RX_STREAM_BUFFER_BACK_SIZE;
	
	//	if new frames to process
	if (0 < numNewFrames)
	{
		//	shift bytes in front buffer to make room
		memmove(MatrixReceiver.streamBuffer, MatrixReceiver.streamBuffer + numNewFrames,
			(CAN_RX_STREAM_BUFFER_FRONT_SIZE - numNewFrames) * sizeof(MATRIX_RX_CAN_FRAME));

		//	copy received bytes into front buffer
		pFrontBufferEnd = MatrixReceiver.streamBuffer + CAN_RX_STREAM_BUFFER_FRONT_SIZE; 
		pFrontBuffer =  pFrontBufferEnd - numNewFrames;
		while (pFrontBuffer < pFrontBufferEnd)
		{
			*pFrontBuffer++ =	MatrixReceiver.rxBuffer[MatrixReceiver.rxBufferReadIndex];
			if (++MatrixReceiver.rxBufferReadIndex >= CAN_RX_STREAM_BUFFER_BACK_SIZE)
				MatrixReceiver.rxBufferReadIndex = 0;
		}
		
		//	process the messages in the stream
		ProcessMessagesInStream(numNewFrames);
	}
}

/**
  * @brief  Set the receiver sender address filter.
	*					The sender address filter is normally zero to receive message from all senders,
	*					and is reset to zero periodically.
  * @param  senderAddressFilter.
  * @retval None.
  */
void MatrixReceiver_SetSenderAddressFilter(uint8_t senderAddressFilter)
{
	//	set the address
	MatrixReceiver.senderAddressFilter = senderAddressFilter;
	
	//	start the sender address filter timer
	//	each time this timer expires it automatically re-starts
	MatrixReceiver.senderAddressFilterTimeout =
		Matrix.systemTime + MATRIX_MAX_SENDER_ADDRESS_FILTER_TIME_MS;
}



//	private methods...........................................................

uint8_t breakPoint, *pBreakPoint;

/**
  * @brief  Processes the messages in the stream.
	* @param  numNewFrames: the number of new frames received.
  * @retval None.
  */
static void ProcessMessagesInStream(uint16_t numNewFrames)
{
	MATRIX_RX_CAN_FRAME *messageFrame, *nextMessageFrame, *frame, *lastFrame;
	TOKEN token;
	uint8_t *frameData;
	uint16_t frameIndex, numMessageBytes, numMessageFrames, key;
	bool isCompleteMessage;
    bool isCommand;
	
	//	remove unprocessed frames from partial messages
	RemoveUnprocessedFrames();
	
	//	sort the new frames
	SortNewFrames(numNewFrames);

	//	starting with oldest frames, search for complete messages
	messageFrame = MatrixReceiver.streamBuffer;
	lastFrame = MatrixReceiver.streamBuffer + CAN_RX_STREAM_BUFFER_FRONT_SIZE;
	while (messageFrame < lastFrame)
	{
		//	advance to used frame
		if (MR_FRAME_FLAG_NONE == messageFrame->frameFlags)
		{
			++messageFrame;
			continue;
		}
		
		//	get next message in stream (incomplete or complete)
		numMessageFrames = 0;
		numMessageBytes = 0;
		frameIndex = messageFrame->frameIndex;
		nextMessageFrame = messageFrame;
		isCompleteMessage = false;
		while (nextMessageFrame < lastFrame)
		{
			//	if not next frame in sequence, then done
			if ((frameIndex != nextMessageFrame->frameIndex) ||
				(nextMessageFrame->senderAddress != messageFrame->senderAddress))
				break;

			//	if a single-frame message, then done
			if (MR_FRAME_FLAG_SINGLE == nextMessageFrame->frameFlags)
			{
					numMessageFrames = 1;
					numMessageBytes = nextMessageFrame->dataSize;
					++nextMessageFrame;
					isCompleteMessage = true;
					break;
			}
			
			//	accumulate frame count and data size
			++numMessageFrames;
			numMessageBytes += nextMessageFrame->dataSize;

			//	if last frame in message, then done
			if (MR_FRAME_FLAG_LAST == nextMessageFrame->frameFlags)
			{
				++nextMessageFrame;
				isCompleteMessage = (numMessageFrames > 1);
				break;
			}
			
			//  bump the frame index and the message frame
			++nextMessageFrame;
			frameIndex = (frameIndex + 1) & MATRIX_CAN_ID_FRAME_INDEX_MASK;
		}
		
		//	if have a complete message
		if (isCompleteMessage)
		{
			//	concatenate message bytes
			frameData = messageFrame->data; 
			frame = messageFrame;
			while (++frame < nextMessageFrame)
				memmove((frameData += CAN_FRAME_MAX_NUM_BYTES), frame->data, CAN_FRAME_MAX_NUM_BYTES);
			
			//	if message checksum is valid
			if ((numMessageFrames == 1) || Matrix_IsMessageChecksumValid(messageFrame->data, numMessageBytes))
			{
				//	remove checksum from message length
				if (CAN_FRAME_MAX_NUM_BYTES < numMessageBytes)
					numMessageBytes -= MATRIX_MESSAGE_CRC_SIZE;
				
				//	point to frame data
				frameData = messageFrame->data; 

				//	if message has at least the event index and a token key
				if (3 <= numMessageBytes)
				{
					//	get key without prefix
					key = ((uint16_t)(frameData[1] & ~KeyPrefix_Mask) << 8) | frameData[2];
					
					//	if a token sequencer sync output
					if (KeyPrefix_PatternSync == (frameData[1] & KeyPrefix_Mask))
					{
						token.address = messageFrame->senderAddress;
						token.key = KeyTokenSequencerSync;
						token.value = key;
						Matrix_PrivateReceiveCanToken(&token);
					}
					//	else if ftp response, then send to ftp client
					else if (Key_IsFtpResponse(key))
					{
						MatrixFTPClient_ServerResponseIn((uint8_t)messageFrame->senderAddress, key,
							frameData + 3, numMessageBytes - 3);
					}
					//	else if ftp request, then send to ftp server
					else if (Key_IsFtpRequest(key))
					{
						MatrixFTPServer_ClientRequestIn((uint8_t)messageFrame->senderAddress, key,
							frameData + 3, numMessageBytes - 3);
					}
					//	else a binary or analog repeat, or standard key of 0~4 bytes
					else
					{
						//	capture event index
						Matrix_NewEventIndex(messageFrame->data[0]);
						
						//	if receiving event, push status update
						if (messageFrame->isEvent)
							Matrix_DelayStatusUpdate15mS();
						
                        // check if the token is a command by verifying that it has a key prefix of 0x00 and the expected number
                        // of bytes for the token (3 comes from 1 for event index and 2 for key)
                        isCommand = (0 == (frameData[1] & KeyPrefix_Mask)) && (numMessageBytes == (3 + Key_ValueSize(key)));

						//	if an event or command or message event order has not expired
						if (messageFrame->isEvent || isCommand || !Matrix_IsEventIndexExpired(messageFrame->data[0]))
						{
							++frameData;
							--numMessageBytes;
							MatrixCodec_Decompress(&frameData, numMessageBytes,
								(uint8_t)messageFrame->senderAddress,	&Matrix_PrivateReceiveCanToken);
						}
					}
				}
			}
			else
			{
				breakPoint = 0;
			}
				
			//	erase message in message stream
			memmove(MatrixReceiver.streamBuffer + numMessageFrames, MatrixReceiver.streamBuffer,
				(uintptr_t)messageFrame - (uintptr_t)MatrixReceiver.streamBuffer);
			memset(MatrixReceiver.streamBuffer, 0, numMessageFrames * sizeof(MATRIX_RX_CAN_FRAME));
		}
		
		//	next message
		messageFrame = nextMessageFrame;
	}
}

/**
  * @brief  Removes unprocessed frames that have been in the buffer for longer than 500mS.
  *         Note that this is an exception because processed frames are already removed.
	* @param  None.
  * @retval None.
  */
static void RemoveUnprocessedFrames(void)
{
	MATRIX_RX_CAN_FRAME *pFrame;

	
	//	for all active frames
	pFrame = MatrixReceiver.streamBuffer + CAN_RX_STREAM_BUFFER_FRONT_SIZE;
	while (--pFrame >= MatrixReceiver.streamBuffer)
	{
		if ((pFrame->frameFlags != MR_FRAME_FLAG_NONE)
			&& (((Matrix.systemTime - pFrame->timeStamp) & 0x0fff) > MATRIX_RECEIVED_FRAME_TIMEOUT_MS))
		{
			if (pFrame > MatrixReceiver.streamBuffer)
				memmove(MatrixReceiver.streamBuffer + 1, MatrixReceiver.streamBuffer,
					((uintptr_t)pFrame - (uintptr_t)MatrixReceiver.streamBuffer));
			MatrixReceiver.streamBuffer[0].frameFlags = MR_FRAME_FLAG_NONE;
		}
	}
}

/**
  * @brief  Sorts the new frames into the proper location within the stream back buffer.
	* @param  numNewFrames: The number of new frames at the start of the back buffer.
  * @retval None.
  */
static void SortNewFrames(uint16_t numNewFrames)
{
	MATRIX_RX_CAN_FRAME *newFrame, *compareFrame, *lastFrame, tempFrame;
	int16_t n;
	bool matchFound;
	
	//	for all new frames
	lastFrame = MatrixReceiver.streamBuffer + CAN_RX_STREAM_BUFFER_FRONT_SIZE;
	newFrame = lastFrame - (numNewFrames + 1);
	while (++newFrame < lastFrame)
	{
		//	try to find most recent frame from same source
		compareFrame = newFrame;
		while ((--compareFrame >= MatrixReceiver.streamBuffer)
			&& (compareFrame->frameFlags != MR_FRAME_FLAG_NONE))
		{
			//	if have one or more frames from same source
			if (compareFrame->senderAddress == newFrame->senderAddress)
			{
				//	find correct position within frames
				n = 15;
				matchFound = false;
				++compareFrame;
				while ((--compareFrame >= MatrixReceiver.streamBuffer)
					&& (compareFrame->frameFlags != MR_FRAME_FLAG_NONE)
					&& (compareFrame->senderAddress == newFrame->senderAddress)
					&& (--n >= 0))
				{
					//	if frame found with same index as new frame, then done
					if (compareFrame->frameIndex == newFrame->frameIndex)
					{
						matchFound = true;
						break;
					}
					
					//	else if frame found with previous index to new frame, then done
					else if ((((newFrame->frameIndex - compareFrame->frameIndex) & MATRIX_CAN_ID_FRAME_INDEX_MASK)
						< ((MATRIX_CAN_ID_FRAME_INDEX_MASK + 1) / 2)))
							break;
				}
				
				//	if same frame index found, then replace previous frame with new one
				if (matchFound)
				{
					*compareFrame = *newFrame;
					if (newFrame > MatrixReceiver.streamBuffer)
						memmove(MatrixReceiver.streamBuffer + 1, MatrixReceiver.streamBuffer,
							(uintptr_t)newFrame - (uintptr_t)MatrixReceiver.streamBuffer);
					MatrixReceiver.streamBuffer[0].frameFlags = MR_FRAME_FLAG_NONE;
				}
				
				//	else if frame has new location, then move into place
				if (++compareFrame < newFrame)
				{
					tempFrame = *newFrame;
					memmove(compareFrame + 1, compareFrame, (uintptr_t)newFrame - (uintptr_t)compareFrame);
					*compareFrame = tempFrame;
				}
				
				//	done
				break;
			}
		}
	}		
}

/**
  * @brief  Receives a CAN frame from the bus.
	* @param  id: The CAN frame ID field.
	* @param  data: The CAN frame DATA field.
	* @param  dataSize: The number of bytes in data.
    * @param  systemTime: the current epoch time in milliseconds
  * @retval None.
  */
void Matrix_ReceiveCanFrame(uint32_t id, uint8_t* data, uint8_t dataSize, uint32_t systemTime)
{
	MATRIX_RX_CAN_FRAME *frame;
    ECCONET_CAN_FRAME_ID *p_enetId = (ECCONET_CAN_FRAME_ID *)&id;
	ECCONET_CAN_FRAME_ID enetId = *p_enetId;
	
	//	frame type filter
	//
	//	if not an ECCONet message, then return
	if ((enetId.frameType < MATRIX_MESSAGE_FRAME_TYPE_SINGLE)
		|| (enetId.frameType > MATRIX_MESSAGE_FRAME_TYPE_LAST))
		return;

	//	source address filter
	//
	//	if a multi-frame message frame, and only listening to multi-frame messages from one sender,
	//	and sender address does not match filter,	then return  (typically used for ftp)
	if ((enetId.frameType != MATRIX_MESSAGE_FRAME_TYPE_SINGLE)
		&& (0 != MatrixReceiver.senderAddressFilter)
		&& (MatrixReceiver.senderAddressFilter != enetId.sourceAddress))
		return;
	
	//	destination address filter
	//
	//	if not a broadcast message or a message to this device, then return
	if ((enetId.destinationAddress != MATRIX_CAN_BROADCAST_ADDRESS)
		&& (enetId.destinationAddress != Matrix_GetCanAddress()))
		return;

	//	validate uint rxBufferWriteIndex as less than the back buffer size
	if (MatrixReceiver.rxBufferWriteIndex >= CAN_RX_STREAM_BUFFER_BACK_SIZE)
		MatrixReceiver.rxBufferWriteIndex = 0;
	
	//	write frame to buffer
	frame = &MatrixReceiver.rxBuffer[MatrixReceiver.rxBufferWriteIndex];
	frame->timeStamp = systemTime;
	frame->frameIndex = enetId.frameIndex;
	frame->frameFlags = enetId.frameType - MATRIX_MESSAGE_FRAME_TYPE_SINGLE + 1;
	frame->senderAddress = enetId.sourceAddress;
	frame->isEvent = enetId.isEvent;
	frame->dataSize = MIN(dataSize, CAN_FRAME_MAX_NUM_BYTES);
	if (frame->dataSize)
		memcpy(frame->data, data, frame->dataSize);
	
	//	patch for VBT
	//	note that if the VBT gets updated, this patch becomes redundant
	if ((enetId.frameType == MATRIX_MESSAGE_FRAME_TYPE_SINGLE)
		&& (enetId.sourceAddress == MATRIX_VEHICLE_BUS_ADDRESS))
		frame->isEvent = true;

	//	bump write index
	if (++MatrixReceiver.rxBufferWriteIndex >= CAN_RX_STREAM_BUFFER_BACK_SIZE)
		MatrixReceiver.rxBufferWriteIndex = 0;
}

