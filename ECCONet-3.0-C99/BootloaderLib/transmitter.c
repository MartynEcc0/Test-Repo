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

#include <string.h>
#include "bootloader_interface.h"
#include "bootloader.h"
#include "encryption.h"
#include "transmitter.h"



/**
  * @brief  The transmitter data object.
  */
TRANSMITTER_OBJECT Transmitter;



/**
  * @brief  Resets and configures the transmitter.
  * @param  None.
  * @retval None.
  */
void Transmitter_Reset(void)
{
	//	reset the frame index
	Transmitter.frame.idBits.frameIndex = 0;
}

/**
  * @brief  Resets the transmitter for a new message.
	* @param  destinationAddress: The destination address, or zero for broadcast.
  * @retval None.
  */
void Transmitter_StartMessage(uint8_t destinationAddress)
{
	//	set the id
	Transmitter.frame.idBits.destinationAddress = destinationAddress;
	Transmitter.frame.idBits.sourceAddress = Bootloader_GetCanAddress();
	Transmitter.buffer[0] = 0;
	Transmitter.pData = &Transmitter.buffer[1];
}

/**
  * @brief  Adds a value to the transmit buffer in big-endian format.
	* @param  value: The value to add.
  * @param  valueSize: The value size in bytes.
  * @retval None.
  */
void Transmitter_AddValueBigEndian(uint32_t value, uint16_t valueSize)
{
	while (valueSize--)
		*Transmitter.pData++ = value >> (8 * valueSize);
}  

/**
  * @brief  Adds data to the transmit buffer.
	* @param  data: The data to send.
	* @param  dataSize: The data size.
  * @retval None.
  */
void Transmitter_AddData(void *data, uint16_t dataSize)
{
	memcpy(Transmitter.pData, data, dataSize);
	Transmitter.pData += dataSize;
}

/**
  * @brief  Adds a string to the transmit buffer.
	* @param  string: The string to send.
	* @param  length: The string length, not including any zero termination.
  * @retval None.
  */
void Transmitter_AddString(const char *string, uint16_t length)
{
	memcpy(Transmitter.pData, string, length);
	Transmitter.pData += length;
	*Transmitter.pData++ = 0;
}

/**
  * @brief  Sends the transmitter buffer over the CAN bus.
	* @param  data: The data to send.
	* @param  dataSize: The data size.
	* @param  address: The destination address.
  * @retval None.
  */
void Transmitter_Finish(void)
{
	uint16_t crc, totalSize, bytesToSend;
	bool multiFrame;
	
	//	if no interface then just return
	if (NULL == Bootloader.appInterface->sendCanFrame)
		return;

	//	total size
	totalSize = (uintptr_t)Transmitter.pData - (uintptr_t)Transmitter.buffer;
	
	//	add the crc
	multiFrame = (totalSize > 8);
	if (multiFrame)
	{
		crc = Encryption_ComputeCRC16(Transmitter.buffer, totalSize);
		Transmitter_AddValueBigEndian(crc, 2);
		totalSize += 2;
	}
	
	//	transmit the frames
	Transmitter.pData = Transmitter.buffer;
	while (totalSize)
	{
		//	send the next frame
		bytesToSend = MIN(totalSize, 8);
		totalSize -= bytesToSend;
		Transmitter.frame.idBits.frameType = !multiFrame ? ENET_MESSAGE_FRAME_TYPE_SINGLE :
			(totalSize ? ENET_MESSAGE_FRAME_TYPE_BODY : ENET_MESSAGE_FRAME_TYPE_LAST);
		Transmitter.frame.dataSize = bytesToSend;
		memcpy(Transmitter.frame.data, Transmitter.pData, bytesToSend);
		Bootloader.appInterface->sendCanFrame(&Transmitter.frame);
		
		//	bump data pointer and frame index
		Transmitter.pData += bytesToSend;
		++Transmitter.frame.idBits.frameIndex;
	}
}


//	messages.................................................................

/**
  * @brief  Sends data over the CAN bus.
	* @param  token: The token to send.
  *         size: The token value size in bytes.
  * @retval None.
  */
void Transmitter_SendToken(TOKEN *token, uint16_t size)
{
	//	send message
	Transmitter_StartMessage(token->address);
	Transmitter_AddValueBigEndian(token->key, 2);
	Transmitter_AddValueBigEndian(token->value, size);
	Transmitter_Finish();
}

/**
  * @brief  Sends info file info or read reply.
  * @param  destinationAddress: The destination address.
  * @param  isInfo: True to respond to an info request.
  * @retval None.
  */
void Transmitter_SendInfoFileReply(uint8_t destinationAddress, bool isInfo)
{
	uint16_t i, crc;

	//	if no interface with product info structure, then just return
	if (NULL == Bootloader.appInterface->productInfoStruct)
		return;
	
	//	get crc of product info file
	crc = Encryption_ComputeCRC16((uint8_t *)Bootloader.appInterface->productInfoStruct, 92);
	
	//	send message
	Transmitter_StartMessage(destinationAddress);
	Transmitter_AddValueBigEndian(isInfo ? KeyResponseFileInfo : KeyResponseFileReadStart, 2);
	Transmitter_AddString(ENET_PRODUCT_INFO_FILE_NAME, ENET_PRODUCT_INFO_FILE_NAME_SIZE);
	Transmitter_AddValueBigEndian(92, 4);
	Transmitter_AddValueBigEndian(crc, 2);
	Transmitter_AddValueBigEndian(0, 4);  // timestamp
	
	//	if is an info response, then send guid
	if (isInfo)
	{
		for (i = 0; i < 4; ++i)
			Transmitter_AddValueBigEndian(Encryption.deviceGuid[i], 4);
	}
	Transmitter_Finish();
}	

/**
  * @brief  Sends info file segment reply.
  * @param  destinationAddress: The destination address.
  * @retval None.
  */
void Transmitter_SendInfoFileSegmentReply(uint8_t destinationAddress)
{
	//	if no interface then just return
	if (NULL == Bootloader.appInterface->productInfoStruct)
		return;
	
	//	send message
	Transmitter_StartMessage(destinationAddress);
	Transmitter_AddValueBigEndian(KeyResponseFileReadSegment, 2);
	Transmitter_AddValueBigEndian(0, 2);  //	segment index
	Transmitter_AddData((void *)Bootloader.appInterface->productInfoStruct, 92);
	Transmitter_Finish();
}	

/**
  * @brief  Sends info file segment reply.
  * @param  destinationAddress: The destination address.
  * @retval None.
  */
void Transmitter_SendWriteFixedSegmentReply(uint8_t destinationAddress, uint8_t code)
{
	//	send message
	Transmitter_StartMessage(destinationAddress);
	Transmitter_AddValueBigEndian(KeyResponseFileWriteFixedSegment, 2);
	Transmitter_AddData(&code, 1);
	Transmitter_Finish();
}	


