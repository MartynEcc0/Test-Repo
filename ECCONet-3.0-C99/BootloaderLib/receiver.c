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

#include <string.h>
#include "ecconet.h"
#include "bootloader_interface.h"
#include "bootloader.h"
#include "can_address.h"
#include "encryption.h"
#include "transmitter.h"
#include "receiver.h"


//	private methods
static void ProcessMessage(void);
static void AddFrameToBuffer(ENET_CAN_FRAME *frame);


/**
  * @brief  The receiver data object.
  */
RECEIVER_OBJECT Receiver;


/**
  * @brief  Resets and configures the receiver.
  * @param  None.
  * @retval None.
  */
void Receiver_Reset(void)
{
	//	reset the receiver state
	Receiver.isReadingInfoFile = false;
	Receiver.pData = Receiver.buffer;
	Receiver.messageSize = 0;
}

/**
  * @brief  Clocks the receiver.
  * @param  None.
  * @retval None.
  */
void Receiver_Clock(void)
{
	//	if have new message sent to this device, then process it
	if (Receiver.messageSize)
		ProcessMessage();
}

/**
  * @brief  Receives a CAN frame from the bus.
	* @param  frame: A pointer to the CAN frame to receive.
  * @retval None.
  */
void Bootloader_ReceiveCanFrame(ENET_CAN_FRAME *frame)
{
	TOKEN token;
	
	//	if a broadcast message
	if (frame->idBits.destinationAddress == ENET_CAN_BROADCAST_ADDRESS)
	{
		//	create null token
		token.address = frame->idBits.sourceAddress;
		token.key = KeyNull;
			
		//	if a single-frame message
		if (frame->idBits.frameType == ENET_MESSAGE_FRAME_TYPE_SINGLE)
		{
			token.key = ((uint16_t)frame->data[1] << 8) | frame->data[2];
			token.value = frame->data[3];
		}
			
		//	update CAN address mechanism
		CanAddress_TokenIn(&token);
	}
	
	//	else if message sent just to this device
	else if (frame->idBits.destinationAddress == Bootloader_GetCanAddress())
	{
		//	set source address
		Receiver.sourceAddress = frame->idBits.sourceAddress;

		//	if receiver buffer is free
		if (!Receiver.messageSize)
		{
			//	if a single frame message
			if (frame->idBits.frameType == ENET_MESSAGE_FRAME_TYPE_SINGLE)
			{
				//	add frame to buffer and set message size
				memcpy(Receiver.buffer, frame->data, frame->dataSize);
				Receiver.messageSize = frame->dataSize;
			}

			//	else if a message body frame
			else if (frame->idBits.frameType == ENET_MESSAGE_FRAME_TYPE_BODY)
			{
				//	add frame to buffer
				AddFrameToBuffer(frame);
			}

			//	else a message last frame
			else if (frame->idBits.frameType == ENET_MESSAGE_FRAME_TYPE_LAST)
			{
				//	if receiving a multi-frame message
				if (((uintptr_t)Receiver.pData - (uintptr_t)Receiver.buffer) >= 8)
				{
					//	add frame to buffer and set message size
					AddFrameToBuffer(frame);
					Receiver.messageSize = (uintptr_t)Receiver.pData - (uintptr_t)Receiver.buffer;
				}		
			}
		}
	}
}

/**
  * @brief  Gets a value from the received data.
  * @param  valueSize: The value size.
  * @retval The value.
  */
uint32_t Receiver_GetValue(uint16_t valueSize)
{
	uint32_t value = 0;
	while (valueSize--)
		value |= ((uint32_t)*Receiver.pData++ << (8 * valueSize));
	return value;
}

/**
  * @brief  Checks the access code.
  * @param  None.
  * @retval True if valid access code.
  */
bool Receiver_CheckAccessCode(void)
{
	return (Receiver_GetValue(4) == Encryption_GetAccessCode());
}

/**
  * @brief  Compares a string to the data in the buffer, and then
  *         advances the buffer pointer past the zero termination.
  * @param  string: The string to compare.
  * @param  length: The string length, including the zero termination.
  * @retval The value.
  */
uint32_t Receiver_CompareString(const char *string, uint16_t length)
{
	int status = strncmp((char *)Receiver.pData, string, length);
	Receiver.pData += length;
	return status;
}


//	private methods.................................

/**
  * @brief  Adds frame to receiver buffer.
  * @param  frame: The frame to add.
  * @retval None.
  */
static void AddFrameToBuffer(ENET_CAN_FRAME *frame)
{
	uint16_t bufferSize;
	
	//	if room to add data, then add it
	bufferSize = (uintptr_t)Receiver.pData - (uintptr_t)Receiver.buffer;
	if ((bufferSize + frame->dataSize) <= RECEIVER_BUFFER_SIZE)
	{
		memcpy(Receiver.pData, frame->data, frame->dataSize);
		Receiver.pData += frame->dataSize;
	}
}

/**
  * @brief  Processes a received message.
  * @param  None.
  * @retval None.
  */
static void ProcessMessage(void)
{
	TOKEN token;
	uint16_t dataSize;
	uint32_t dataLocation;
	bool isInfo;

	//	bump back pData
	Receiver.pData -= 2;

	//	if message has checksum
	if ((Receiver.messageSize <= 8) ||
		(Encryption_ComputeCRC16(Receiver.buffer, Receiver.messageSize - 2) == Receiver_GetValue(2)))
	{
		//	get message key
		Receiver.pData = &Receiver.buffer[1];
		token.key = Receiver_GetValue(2);

		//	if request for file info or file read
		isInfo = (token.key == KeyRequestFileInfo);
		if (isInfo || (token.key == KeyRequestFileReadStart))
		{
			//	if have validate file name, and is file info request or read request with valid access code, then send file
			if ((0 == Receiver_CompareString(ENET_PRODUCT_INFO_FILE_NAME, ENET_PRODUCT_INFO_FILE_NAME_SIZE + 1))
				&& (isInfo || (Receiver_CheckAccessCode())))
			{
				Receiver.isReadingInfoFile = !isInfo;
				Transmitter_SendInfoFileReply(Receiver.sourceAddress, isInfo);
			}
		}
		
		//	else if request for reading file segment
		else if (token.key == KeyRequestFileReadSegment)
		{
			//	if is reading info file
			if (Receiver.isReadingInfoFile)
			{
				//	clear status
				Receiver.isReadingInfoFile = false;
				
				//	if segment is zero and valid access code, the send file
				if ((0 == Receiver_GetValue(2)) && (Receiver_CheckAccessCode()))
				{
					Transmitter_SendInfoFileSegmentReply(Receiver.sourceAddress);
				}
			}
		}
		
		//	else if request to write flash
		else if (token.key == KeyRequestFileWriteFixedSegment)
		{
			//	decrypt the inner data, less the event index, token, and message checksum
			Encryption_Encrypt(&Receiver.buffer[3], Receiver.messageSize - (1 + 2 + 2));
			
			//	result code
			token.value = BSC_OK;
			
			//	validate access code
			if (Receiver_GetValue(4) != Encryption_GetAccessCode())
				token.value = BSC_INVALID_ACCESS_CODE;
			
			//	validate model name
			else if ((NULL == Bootloader.appInterface->productInfoStruct)
				|| (0 != Receiver_CompareString(Bootloader.appInterface->productInfoStruct->modelName, 31)))
				token.value = BSC_INVALID_MODEL_NAME;
			
			//	validate area to flash
			else
			{
				//	get data location and size
				Receiver.pData = &Receiver.buffer[38];
				dataLocation = Receiver_GetValue(4);
				dataSize = Receiver_GetValue(2);

				if ((dataLocation < Bootloader.appInterface->appFlashAddress)
					|| ((dataLocation + dataSize) >
					(Bootloader.appInterface->appFlashAddress + Bootloader.appInterface->appFlashSize)))
					token.value = BSC_INVALID_FLASH_AREA;
					
				//	write flash
				else if ((NULL == Bootloader.appInterface->flashWrite)
					|| (!Bootloader.appInterface->flashWrite(dataLocation, &Receiver.buffer[44], dataSize)))
					token.value = BSC_FLASH_WRITE_ERROR;
			}
			
			//	send result
			token.address = Receiver.sourceAddress;
			token.key = KeyResponseFileWriteFixedSegment;
			Transmitter_SendToken(&token, 1);
		}
		
		//	else if KeyRequestSystemReboot
		else if (token.key == KeyRequestSystemReboot)
		{
			if ((Receiver_GetValue(4) == (Encryption_GetAccessCode() ^ TOKEN_VALUE_SYSTEM_REBOOT))
				&& (NULL != Bootloader.appInterface->reboot))
				Bootloader.appInterface->reboot();
		}
	}
	
	//	clear message size and address
	Receiver.messageSize = 0;
	Receiver.pData = Receiver.buffer;
}

