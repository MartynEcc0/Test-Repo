/**
  ******************************************************************************
  * @file    		matrix_file_transfer.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Matrix FTP server for the CAN bus.
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
#include <stdbool.h>

//	matrix
#include "matrix.h"
#include "matrix_config.h"
#include "matrix_crc.h"
#include "matrix_transmitter.h"
#include "matrix_receiver.h"
#include "matrix_ftp_client.h"
#include "matrix_ftp_server.h"


//	external private matrix methods
extern MATRIX_FTP_CLIENT_OBJECT MatrixFTPClient;
extern int Matrix_PrivateSendCanToken(TOKEN *token);

//	internal private methods
uint32_t GenerateServerAccessCode(uint32_t guid[4]);
static bool ValidateServerAccessCode(uint8_t *code);
static void RefuseRequest(uint16_t responseKey);

//	request for file info or to read file
static void HandleFileInfoReadStartRequest(uint16_t senderAddress,
	uint8_t *body, uint32_t bodySize, uint16_t responseKey);
static void HandleFileReadSegmentRequest(uint8_t *body, uint32_t bodySize);

//	request to write file
static void HandleFileWriteStartRequest(uint16_t senderAddress,
	uint8_t *body, uint32_t bodySize);
static void HandleFileWriteSegmentRequest(uint8_t *body, uint32_t bodySize);

//	request to erase file
static void HandleFileEraseRequest(uint8_t *body, uint32_t bodySize);


/**
  * @brief  The Matrix file transfer data object.
  */
MATRIX_FTP_SERVER_OBJECT MatrixFTPServer;


/**
  * @brief  Resets the Matrix CAN file transfer mechanism.
  * @param  None.
  * @retval None.
  */
void MatrixFTPServer_Reset(void)
{
	uint32_t guid[4];

	//	reset the client request
	MatrixFTPServer.client.request = KeyNull;
	
	//	set the server access code
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->get128BitGuid))
		Matrix.appInterface->get128BitGuid(guid);
	else
		memset(guid, 0, sizeof(uint32_t) * 4);
	MatrixFTPServer.accessCode = GenerateServerAccessCode(guid);
	
	//	start the request timer
	//	each time this timer expires it automatically re-starts
	MatrixFTPServer.client.requestTimeout =
		Matrix.systemTime + MATRIX_MAX_FILE_REQUEST_RESPONSE_TIME_MS;
}

/**
  * @brief  Clocks the Matrix CAN file transfer mechanism.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
void MatrixFTPServer_Clock(void)
{
	//	check for transfer timeout
	if (IsMatrixTimerExpired(MatrixFTPServer.client.requestTimeout))
	{
		//	reset the timeout
		MatrixFTPServer.client.requestTimeout =
			Matrix.systemTime + MATRIX_MAX_FILE_REQUEST_RESPONSE_TIME_MS;
		
		//	clear any client request
		MatrixFTPServer.client.request = KeyNull;
	}
}

/**
  * @brief  Handles incoming client request tokens.
	* @param  senderAddress: The CAN address of the sender.
	* @param  key: The token key.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval None.
  */
void MatrixFTPServer_ClientRequestIn(uint16_t senderAddress, uint16_t requestKey,
	uint8_t *body, uint32_t bodySize)
{
	TOKEN token;
	
	//	if this device is currently acting as a client,
	//	then reject this request
	if (KeyNull != MatrixFTPClient.server.expectedResponse)
		return;
	
	//	while transfer is in progress, reject requests to start a new transfer
	//	and reject requests from a different client
	if ((KeyNull != MatrixFTPServer.client.request) &&
		((requestKey == KeyRequestFileReadStart) || (requestKey == KeyRequestFileWriteStart) ||
		(senderAddress != MatrixFTPServer.client.address)))
	{
		token.key = KeyResponseFtpServerBusy;
		token.value = 0;
		token.address = senderAddress;
		Matrix_PrivateSendCanToken(&token); 
		return;
	}
	
	//	save client request and address, and retstart transaction timer
	MatrixFTPServer.client.request = requestKey;
	MatrixFTPServer.client.address = senderAddress;
	MatrixFTPServer.client.requestTimeout =
    Matrix.systemTime + MATRIX_MAX_FILE_REQUEST_RESPONSE_TIME_MS;
	
	//	set the receiver sender address filter
	MatrixReceiver_SetSenderAddressFilter(MatrixFTPServer.client.address);

	//	handle request
	switch (requestKey)
	{
		case KeyRequestFileIndexedInfo:
		case KeyRequestFileInfo:
		case KeyRequestFileReadStart:
			HandleFileInfoReadStartRequest(senderAddress, body, bodySize, requestKey);
			break;
		
		case KeyRequestFileReadSegment:
			HandleFileReadSegmentRequest(body, bodySize);
			break;

		case KeyRequestFileWriteStart:
			HandleFileWriteStartRequest(senderAddress, body, bodySize);
			break;
		
		case KeyRequestFileWriteSegment:
			HandleFileWriteSegmentRequest(body, bodySize);
			break;
		
		case KeyRequestFileDelete:
			HandleFileEraseRequest(body, bodySize);
			break;
		
		case KeyRequestFileTransferComplete:
			//	clear the request and receiver address filter
			MatrixFTPServer.client.request = KeyNull;
			MatrixReceiver_SetSenderAddressFilter(0);
			break;

		default:
				break;
	}
}



//	private methods.....................................................................

/**
	* @brief  Generates the server access code.
	* @param  guid: An array of for uint32_t that are the 128-bit device guid.
  * @retval The server access code.
  */
uint32_t GenerateServerAccessCode(uint32_t guid[4])
{
	uint32_t value;
	
	//	run hash and return code
	value = guid[0] ^ guid[3];
	value >>= (int32_t)((guid[0] >> 3) & 3);
	value ^= guid[2];
	value ^= MATRIX_SERVER_ACCESS_POLY;
	value ^= guid[1];
	return value;
}

/**
	* @brief  Helper method validates the given server access code.
	* @param  code: A pointer to a byte array that contains code.
  * @retval Returns true if access code is valid.
  */
static bool ValidateServerAccessCode(uint8_t *code)
{
	uint16_t i;
	uint32_t accessCode;

	//	validate input
	if (NULL != code)
	{
		//	get the code
		i = 4;
		accessCode = 0;
		while (i--)
		{
			accessCode <<= 8;
			accessCode |= *code++;
		}
			
		//	return value indicating whether the code is valid
		return (accessCode == MatrixFTPServer.accessCode);
	}
	return false;
}

/**
	* @brief  Helper method does the following:
	*					- Clears the current request
	*					- Notifies the client of reason request was refused
	*
	* @param  responseKey: The response key provided to the client.
	* @param  filename: If not null, the filename is sent with the response.
  * @retval None.
  */
static void RefuseRequest(uint16_t responseKey)
{
	TOKEN token;
	
	//	clear the request and receiver address filter
	MatrixFTPServer.client.request = KeyNull;
	MatrixReceiver_SetSenderAddressFilter(0);

	//	send the response
	token.key = responseKey;
	token.value = 0;
	token.address = MatrixFTPServer.client.address;
	Matrix_PrivateSendCanToken(&token);
}

/**
  * @brief  Handles a request for file info or to read file.
	* @param  senderAddress: The CAN address of the sender.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
	* @param  responseKey: The response token key.
  * @retval Returns 0 if file is found, else -1.
  */
static void HandleFileInfoReadStartRequest(uint16_t senderAddress,
	uint8_t *body, uint32_t bodySize,	uint16_t requestKey)
{
	uint16_t i, filenameLen;
	uint32_t guid[4], fileIndex;
	bool sendingGuid = false;

	
	//	clear the file params
	memset(&MatrixFTPServer.file, 0, sizeof(MATRIX_FILE_METADATA));
	
	//	if an indexed file request
	if (KeyRequestFileIndexedInfo == requestKey)
	{
		//	client must provide required fields
		if ((2 + 4 + 4) > bodySize)
		{
			RefuseRequest(KeyResponseFtpClientError);
			return;
		}
		
		//	get the volume index
		i = 2;
		MatrixFTPServer.file.volumeIndex = 0;
		while (i--)
		{
			MatrixFTPServer.file.volumeIndex <<= 8;
			MatrixFTPServer.file.volumeIndex |= *body++;
		}

		//	get the file index
		i = 4;
		fileIndex = 0;
		while (i--)
		{
			fileIndex <<= 8;
			fileIndex |= *body++;
		}
		
		//	client must provide valid server access key
		if (!ValidateServerAccessCode(body))
		{
			RefuseRequest(KeyResponseFtpClientError);
			return;
		}
		
		//	try to get the indexed file header
		if (0 != FlashDrive_GetIndexedFileMetadata(fileIndex, &MatrixFTPServer.file))
		{
			RefuseRequest(KeyResponseFileNotFound);
			return;
		}
	}
	
	//	else a named file info or read start request
	else
	{
		//	client must provide valid file name
		filenameLen = FlashDrive_ValidateFileName((char *)body);
		if (0 == filenameLen)
		{
			RefuseRequest(KeyResponseFtpClientError);
			return;
		}

		//	copy file name
		strcpy(MatrixFTPServer.file.name, (char *)body);
		body += (filenameLen + 1);

		//	get sending guid status 
		sendingGuid = ((KeyRequestFileInfo == requestKey)
			&& (0 == strcmp(MatrixFTPServer.file.name, MATRIX_PRODUCT_INFO_FILE_NAME)));

		//	if not sending guid, then client must provide valid server access key
		if (!sendingGuid && !ValidateServerAccessCode(body))
		{
			RefuseRequest(KeyResponseFtpClientError);
			return;
		}
		
		//	try to get the volume to which the file belongs
		MatrixFTPServer.file.volumeIndex = 0;
		if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->fileNameToVolumeIndex))
			MatrixFTPServer.file.volumeIndex = Matrix.appInterface->fileNameToVolumeIndex(MatrixFTPServer.file.name);
		
		//	if the application handles the file
		if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->ftpServerReadHandler)
			&& (0 == Matrix.appInterface->ftpServerReadHandler(senderAddress, &MatrixFTPServer.file)))
		{
			//	set the data checksum
			MatrixFTPServer.file.dataChecksum = Matrix_ComputeCRC16(
				(uint8_t *)MatrixFTPServer.file.dataLocation, MatrixFTPServer.file.dataSize);
		}
		
		//	else if file found on flash drive volume
		else if (0 == FlashDrive_GetFileMetadata(&MatrixFTPServer.file))
		{
			//	nothing to do here
		}
		
		//	else file not available but sending guid
		else if (sendingGuid)
		{
			//	set a blank product info file
			MatrixFTPServer.file.dataSize = 1;
		}
		else
		{
			//	refuse request with file not found response
			RefuseRequest(KeyResponseFileNotFound);
			return;
		}				
	}
	
	//	initialize the transmitter
	MatrixTransmitter_StartMessage(MatrixFTPServer.client.address);

	//	send the response key
	if (KeyRequestFileIndexedInfo == requestKey)
		MatrixTransmitter_AddInt16(KeyResponseFileIndexedInfo);
	else if (KeyRequestFileInfo == requestKey)
		MatrixTransmitter_AddInt16(KeyResponseFileInfo);
	else
		MatrixTransmitter_AddInt16(KeyResponseFileReadStart);

	//	send the file name
	MatrixTransmitter_AddString(MatrixFTPServer.file.name);

	//	send the data size
	MatrixTransmitter_AddInt32(MatrixFTPServer.file.dataSize);
	
	//	send the data crc checksum
	MatrixTransmitter_AddInt16(MatrixFTPServer.file.dataChecksum);
	
	//	send the timestamp
	MatrixTransmitter_AddInt32(MatrixFTPServer.file.timestamp);
	
	//	if product info file info request, send the guid
	if (sendingGuid)
	{
		//	get the device guid
		if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->get128BitGuid))
			Matrix.appInterface->get128BitGuid(guid);
		else
			memset(guid, 0, sizeof(uint32_t) * 4);
		for (i = 0; i < 4; ++i)
			MatrixTransmitter_AddInt32(guid[i]);
	}
	
	//	send remaining part of message in fifo
	MatrixTransmitter_FinishMessage();
}

/**
  * @brief  Handles a file read segment request.
	* @param  senderAddress: The requestor CAN address.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval Returns 0 on success, else -1.
  */
static void HandleFileReadSegmentRequest(uint8_t *body, uint32_t bodySize)
{
	uint8_t buffer[16];
	uint16_t i, segmentIndex;
	uint32_t dataLocation, lastDataLocation;

	
	//	if file params not set, then refuse request
	if (0 == MatrixFTPServer.file.dataSize)
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	client must provide body with correct size
	if ((NULL == body) || ((2 + 4) > bodySize))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	get segment index
	i = 2;
	segmentIndex = 0;
	while (i--)
	{
		segmentIndex <<= 8;
		segmentIndex |= *body++;
	}
	
	//	client must provide valid server access code
	if (!ValidateServerAccessCode(body))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	initialize the transmitter
	MatrixTransmitter_StartMessage(MatrixFTPServer.client.address);

	//	send the response key
	MatrixTransmitter_AddInt16(KeyResponseFileReadSegment);

	//	send the segment index
	MatrixTransmitter_AddInt16(segmentIndex);

	//	send the data
	dataLocation = MatrixFTPServer.file.dataLocation + 
		(segmentIndex * MATRIX_MAX_FILE_SEGMENT_LENGTH);
	lastDataLocation = MIN((dataLocation + MATRIX_MAX_FILE_SEGMENT_LENGTH),
		(MatrixFTPServer.file.dataLocation + MatrixFTPServer.file.dataSize));
	while (dataLocation < lastDataLocation)
	{
		//	get bytes to read
		i = MIN(16, ((uint32_t)lastDataLocation - (uint32_t)dataLocation));
		if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->flashRead))
			Matrix.appInterface->flashRead(MatrixFTPServer.file.volumeIndex, dataLocation, buffer, i);
		for (segmentIndex = 0; segmentIndex < i; ++segmentIndex)
			MatrixTransmitter_AddByte(buffer[segmentIndex]);
		dataLocation += i;
	}
	
	//	send remaining part of message in fifo
	MatrixTransmitter_FinishMessage();
}

/**
  * @brief  Handles a file write start request.
	* @param  senderAddress: The requestor CAN address.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval Returns 0 on success, else -1.
  */
static void HandleFileWriteStartRequest(uint16_t senderAddress, uint8_t *body, uint32_t bodySize)
{
	uint16_t i, filenameLen;

	
	//	clear the file params
	memset(&MatrixFTPServer.file, 0, sizeof(MATRIX_FILE_METADATA));

	//	client must provide valid file name
	filenameLen = FlashDrive_ValidateFileName((char *)body);
	if (0 == filenameLen)
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}

	//	copy file name
	strcpy(MatrixFTPServer.file.name, (char *)body);

	//	client must provide body with correct size
	if ((NULL == body) || ((uint32_t)(filenameLen + (1 + 4 + 2 + 4 + 4)) > bodySize))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	get the file info
	body += (filenameLen + 1);
	i = 4;
	while (i--)
	{
		MatrixFTPServer.file.dataSize <<= 8;
		MatrixFTPServer.file.dataSize |= *body++;
	}
	i = 2;
	while (i--)
	{
		MatrixFTPServer.file.dataChecksum <<= 8;
		MatrixFTPServer.file.dataChecksum |= *body++;
	}
	i = 4;
	while (i--)
	{
		MatrixFTPServer.file.timestamp <<= 8;
		MatrixFTPServer.file.timestamp |= *body++;
	}
	
	//	client must provide valid server access code
	if (!ValidateServerAccessCode(body))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}

	//	try to get the volume to which the file belongs
	MatrixFTPServer.file.volumeIndex = 0;
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->fileNameToVolumeIndex))
		MatrixFTPServer.file.volumeIndex = Matrix.appInterface->fileNameToVolumeIndex(MatrixFTPServer.file.name);

	//	try to write the file header and allocate the data in flash
	if (0 != FlashDrive_WriteFileHeader(&MatrixFTPServer.file)
		|| (0 != FlashDrive_GetFileMetadata(&MatrixFTPServer.file)))
	{
		//	if cannot be written then refuse request with a disk full response
		RefuseRequest(KeyResponseFtpDiskFull);
		return;
	}
	
	//	start a message
	MatrixTransmitter_StartMessage(MatrixFTPServer.client.address);
	
	//	send the response key
	MatrixTransmitter_AddInt16(KeyResponseFileWriteStart);
	
	//	send the file name
	MatrixTransmitter_AddString(MatrixFTPServer.file.name);

	//	finish the message
	MatrixTransmitter_FinishMessage();
}

/**
  * @brief  Handles a file write segment request.
	* @param  senderAddress: The requestor CAN address.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval Returns 0 on success, else -1.
  */
static void HandleFileWriteSegmentRequest(uint8_t *body, uint32_t bodySize)
{
	uint16_t i, segmentIndex;
	uint32_t locationOffset;
	
	//	if file params not set, then refuse request
	if (0 == MatrixFTPServer.file.dataSize)
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	client must provide body with correct size
	if ((NULL == body) || (3 > bodySize))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	get the segment index and calculate location offset
	i = 2;
	segmentIndex = 0;
	while (i--)
	{
		segmentIndex <<= 8;
		segmentIndex |= *body++;
	}
	bodySize -= 2;
	locationOffset = segmentIndex * MATRIX_MAX_FILE_SEGMENT_LENGTH;
	
	//	client must provide valid server access code
	if (!ValidateServerAccessCode(body))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	body += 4;
	bodySize -= 4;
	
	//	write the file data
	if (0 != FlashDrive_WriteData(MatrixFTPServer.file.volumeIndex,
		MatrixFTPServer.file.name, body, bodySize, locationOffset))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	start a message
	MatrixTransmitter_StartMessage(MatrixFTPServer.client.address);

	//	send the response key
	MatrixTransmitter_AddInt16(KeyResponseFileWriteSegment);
	
	//	send the segment index
	MatrixTransmitter_AddInt16(segmentIndex);

	//	finish the message
	MatrixTransmitter_FinishMessage();
}

/**
  * @brief  Handles a file erase request.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval None.
  */
static void HandleFileEraseRequest(uint8_t *body, uint32_t bodySize)
{
	uint16_t filenameLen;
	
	//	clear the file params
	MatrixFTPServer.file.dataSize = 0;
	MatrixFTPServer.file.dataChecksum = 0;
	MatrixFTPServer.file.dataLocation = 0;
	MatrixFTPServer.file.volumeIndex = 0;
	memset(MatrixFTPServer.file.name, 0, MATRIX_FILE_NAME_LENGTH + 1);
	
	//	client must provide valid file name
	filenameLen = FlashDrive_ValidateFileName((char *)body);
	if (0 == filenameLen)
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	save pointer the request file name
	strcpy(MatrixFTPServer.file.name, (char *)body);
	body += (filenameLen + 1);
	
	//	client must provide body with correct size
	if ((NULL == body) || ((uint32_t)(filenameLen + (1 + 4)) > bodySize))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}
	
	//	client must provide valid server access code
	if (!ValidateServerAccessCode(body))
	{
		RefuseRequest(KeyResponseFtpClientError);
		return;
	}

	//	try to get the volume to which the file belongs
	MatrixFTPServer.file.volumeIndex = 0;
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->fileNameToVolumeIndex))
		MatrixFTPServer.file.volumeIndex = Matrix.appInterface->fileNameToVolumeIndex(MatrixFTPServer.file.name);

	//	try to erase the file
	if (0 != FlashDrive_EraseFile(MatrixFTPServer.file.volumeIndex, MatrixFTPServer.file.name))
	{
		RefuseRequest(KeyResponseFileNotFound);
		return;
	}
	
	//	start a message
	MatrixTransmitter_StartMessage(MatrixFTPServer.client.address);
	
	//	send the response key
	MatrixTransmitter_AddInt16(KeyResponseFileDelete);
	
	//	send the file name
	MatrixTransmitter_AddString(MatrixFTPServer.file.name);

	//	finish the message
	MatrixTransmitter_FinishMessage();
}
