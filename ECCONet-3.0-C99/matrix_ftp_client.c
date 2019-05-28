/**
  ******************************************************************************
  * @file    		matrix_ftp_client.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Matrix file transfers over the CAN bus.
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
#include "matrix.h"
#include "matrix_crc.h"
#include "matrix_config.h"
#include "matrix_transmitter.h"
#include "matrix_receiver.h"
#include "matrix_ftp_server.h"
#include "matrix_ftp_client.h"


//	private methods
extern MATRIX_FTP_SERVER_OBJECT MatrixFTPServer;
extern int Matrix_PrivateSendCanToken(TOKEN *token);
extern uint32_t GenerateServerAccessCode(uint32_t guid[4]);
static void FinishRequest(uint16_t expectedResponse);
static void EndTransaction(uint16_t key);
static void PopulateCallbackInfo(FTP_CLIENT_CALLBACK_INFO *callbackInfo, uint16_t responseKey);

//	read file or file info
static void RequestReadSegment(void);
static void HandleFileInfoAndReadStartResponse(uint16_t response, uint8_t *body, uint32_t bodySize);
static void HandleFileReadSegmentResponse(uint8_t *body, uint32_t bodySize);

//	write file
static void RequestWriteSegment(void);
static void HandleFileWriteStartResponse(uint8_t *body, uint32_t bodySize);
static void HandleFileWriteSegmentResponse(uint8_t *body, uint32_t bodySize);

//	delete file
static void HandleFileDeleteResponse(uint8_t *body, uint32_t bodySize);


/**
  * @brief  The Matrix ftp client data object.
  */
MATRIX_FTP_CLIENT_OBJECT MatrixFTPClient;

/**
  * @brief  Resets the Matrix ftp client.
  * @param  None.
  * @retval None.
  */
void MatrixFTPClient_Reset(void)
{
	//	reset the file transfer state
	MatrixFTPClient.requester.callback = NULL;
	MatrixFTPClient.server.expectedResponse = KeyNull;

	//	start request timeout timer
	MatrixFTPClient.server.responseTimeout =
		Matrix.systemTime + MATRIX_MAX_FILE_REQUEST_RESPONSE_TIME_MS;
}

/**
  * @brief  Clocks the Matrix ftp client.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
void MatrixFTPClient_Clock(void)
{
	FTP_CLIENT_CALLBACK_INFO info;
	
	//	check for request timeout
	if (IsMatrixTimerExpired(MatrixFTPClient.server.responseTimeout))
	{
		//	setup next timeout
		MatrixFTPClient.server.responseTimeout =
			Matrix.systemTime + MATRIX_MAX_FILE_REQUEST_RESPONSE_TIME_MS;
		
		if (KeyNull != MatrixFTPClient.server.expectedResponse)
		{
			//	if callback then notify requester
			if (NULL != MatrixFTPClient.requester.callback)
			{
				PopulateCallbackInfo(&info, KeyResponseFtpTransactionTimedOut);
				MatrixFTPClient.requester.callback(&info);
			}

			//	clear callback and expected response
			MatrixFTPClient.requester.callback = NULL;
			MatrixFTPClient.server.expectedResponse = KeyNull;
		}
	}
}

/**
  * @brief  Handles incoming server responses.
	* @param  senderAddress: The CAN address of the sender.
	* @param  responseKey: The token response key.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval None.
  */
void MatrixFTPClient_ServerResponseIn(uint16_t senderAddress, uint16_t responseKey,
	uint8_t *body, uint32_t bodySize)
{
	//	if not expecting a response (in idle state)
	//	or response is from an unexpected server, then ignore
	if ((KeyNull == MatrixFTPClient.server.expectedResponse)
		|| (senderAddress != MatrixFTPClient.server.address))
		return;

	//	if response is not the expected response, then end transaction
	if (responseKey != MatrixFTPClient.server.expectedResponse)
	{
		EndTransaction(responseKey);
		return;
	}
	
	//	handle response
	switch (responseKey)
	{
		case KeyResponseFileIndexedInfo:
		case KeyResponseFileInfo:
		case KeyResponseFileReadStart:
			HandleFileInfoAndReadStartResponse(responseKey, body, bodySize);
			break;
			
		case KeyResponseFileReadSegment:
			HandleFileReadSegmentResponse(body, bodySize);
			break;
			
		case KeyResponseFileWriteStart:
			HandleFileWriteStartResponse(body, bodySize);
			break;
			
		case KeyResponseFileWriteSegment:
			HandleFileWriteSegmentResponse(body, bodySize);
			break;

		case KeyResponseFileDelete:
			HandleFileDeleteResponse(body, bodySize);
			break;

		//	any other response ends transaction
		default:
			EndTransaction(responseKey);
			break;
	}
}

/**
  * @brief  Starts an indexed file info request from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
int MatrixFTPClient_GetIndexedFileInfo(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams)
{
	//	if this device is currently acting as a server,
	//	then reject this request
	if (KeyNull != MatrixFTPServer.client.request)
		return -1;
	
	//	if tranfer already in progress, then return
	if (KeyNull != MatrixFTPClient.server.expectedResponse)
		return -1;
	
	//	validate inputs
	if ((NULL == transferParams) ||(0 == transferParams->serverAddress))	
		return -1;
	
	//	copy the requestor parameters
  MatrixFTPClient.server.address = transferParams->serverAddress;
  MatrixFTPClient.server.accessCode = transferParams->serverAccessCode;
	MatrixFTPClient.requester.buffer = NULL;
	MatrixFTPClient.requester.bufferSize = 0;
	MatrixFTPClient.requester.callback = transferParams->callback;
	MatrixFTPClient.file.name[0] = 0;
	
	//	initialize the transmitter
	MatrixTransmitter_StartMessage(transferParams->serverAddress);

	//	send the request key
	MatrixTransmitter_AddInt16(KeyRequestFileIndexedInfo);

	//	send the drive volume index
	MatrixTransmitter_AddInt16(transferParams->volumeIndex);

	//	send the file index
	MatrixTransmitter_AddInt32(transferParams->fileIndex);

	//	send the server access code
	MatrixTransmitter_AddInt32(transferParams->serverAccessCode);
	
	//	send the request
	FinishRequest(KeyResponseFileIndexedInfo);
	return 0;
}

/**
  * @brief  Starts a file info request from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
int MatrixFTPClient_GetFileInfo(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams)
{
	//	if this device is currently acting as a server,
	//	then reject this request
	if (KeyNull != MatrixFTPServer.client.request)
		return -1;
	
	//	if tranfer already in progress, then return
	if (KeyNull != MatrixFTPClient.server.expectedResponse)
		return -1;
	
	//	validate inputs
	if ((0 == transferParams->serverAddress) || (NULL == transferParams->filename)
		|| (NULL == transferParams->buffer) || (0 == transferParams->bufferSize))	
		return -1;
	
	//	validate the file name
	if (0 == FlashDrive_ValidateFileName(transferParams->filename))
		return -1;
	
	//	copy the requestor parameters
  MatrixFTPClient.server.address = transferParams->serverAddress;
  MatrixFTPClient.server.accessCode = transferParams->serverAccessCode;
	MatrixFTPClient.requester.buffer = NULL;
	MatrixFTPClient.requester.bufferSize = 0;
	MatrixFTPClient.requester.callback = transferParams->callback;
	strcpy(MatrixFTPClient.file.name, transferParams->filename);
	
	//	initialize the transmitter
	MatrixTransmitter_StartMessage(transferParams->serverAddress);

	//	send the request key
	MatrixTransmitter_AddInt16(KeyRequestFileInfo);

	//	send the file name
	MatrixTransmitter_AddString(transferParams->filename);

	//	send the server access code
	MatrixTransmitter_AddInt32(transferParams->serverAccessCode);
	
	//	send the request
	FinishRequest(KeyResponseFileInfo);
	return 0;
}

/**
  * @brief  Starts a file read from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
int MatrixFTPClient_ReadFile(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams)
{
	//	if this device is currently acting as a server,
	//	then reject this request
	if (KeyNull != MatrixFTPServer.client.request)
		return -1;
	
	//	if transfer already in progress, then return
	if (KeyNull != MatrixFTPClient.server.expectedResponse)
		return -1;

	//	validate inputs
	if ((0 == transferParams->serverAddress) || (NULL == transferParams->filename)
		|| (NULL == transferParams->buffer) || (0 == transferParams->bufferSize))	
		return -1;
	
	//	validate the file name
	if (0 == FlashDrive_ValidateFileName(transferParams->filename))
		return -1;
	
	//	copy the requestor parameters
  MatrixFTPClient.server.address = transferParams->serverAddress;
  MatrixFTPClient.server.accessCode = transferParams->serverAccessCode;
	MatrixFTPClient.requester.buffer = transferParams->buffer;
	MatrixFTPClient.requester.bufferSize = transferParams->bufferSize;
	MatrixFTPClient.requester.callback = transferParams->callback;
	strcpy(MatrixFTPClient.file.name, transferParams->filename);
	
	//	initialize the transmitter
	MatrixTransmitter_StartMessage(transferParams->serverAddress);

	//	send the request key
	MatrixTransmitter_AddInt16(KeyRequestFileReadStart);

	//	send the file name
	MatrixTransmitter_AddString(transferParams->filename);

	//	send the server access code
	MatrixTransmitter_AddInt32(transferParams->serverAccessCode);

	//	send the request
	FinishRequest(KeyResponseFileReadStart);
	return 0;
}

/**
  * @brief  Starts a file write to the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
int MatrixFTPClient_WriteFile(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams)
{
	//	if this device is currently acting as a server,
	//	then reject this request
	if (KeyNull != MatrixFTPServer.client.request)
		return -1;
	
	//	if transfer already in progress, then return
	if (KeyNull != MatrixFTPClient.server.expectedResponse)
		return -1;
	
	//	validate inputs
	if ((0 == transferParams->serverAddress) || (NULL == transferParams->filename)
		|| (NULL == transferParams->buffer) || (0 == transferParams->bufferSize))	
		return -1;
	
	//	validate the file name
	if (0 == FlashDrive_ValidateFileName(transferParams->filename))
		return -1;
	
	//	copy the requestor parameters
  MatrixFTPClient.server.address = transferParams->serverAddress;
  MatrixFTPClient.server.accessCode = transferParams->serverAccessCode;
	MatrixFTPClient.requester.buffer = transferParams->buffer;
	MatrixFTPClient.requester.bufferSize = transferParams->bufferSize;
	MatrixFTPClient.requester.callback = transferParams->callback;
	strcpy(MatrixFTPClient.file.name, transferParams->filename);
	MatrixFTPClient.file.dataSize = transferParams->bufferSize;

	//	initialize the transmitter
	MatrixTransmitter_StartMessage(transferParams->serverAddress);

	//	send the request key
	MatrixTransmitter_AddInt16(KeyRequestFileWriteStart);

	//	send the file name
	MatrixTransmitter_AddString(transferParams->filename);

	//	send the data size
	MatrixTransmitter_AddInt32(transferParams->bufferSize);

	//	send the data crc
	MatrixTransmitter_AddInt16(Matrix_ComputeCRC16(transferParams->buffer, transferParams->bufferSize));

	//	send the file timestamp
	MatrixTransmitter_AddInt32(transferParams->fileTimestamp);

	//	send the server access code
	MatrixTransmitter_AddInt32(transferParams->serverAccessCode);

	//	send the request
	FinishRequest(KeyResponseFileWriteStart);
	return 0;
}

/**
  * @brief  Deletes a file from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
int MatrixFTPClient_DeleteFile(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams)
{
	//	if this device is currently acting as a server,
	//	then reject this request
	if (KeyNull != MatrixFTPServer.client.request)
		return -1;
	
	//	if tranfer already in progress, then return
	if (KeyNull != MatrixFTPClient.server.expectedResponse)
		return -1;
	
	//	validate inputs
	if ((0 == transferParams->serverAddress) || (NULL == transferParams->filename)
		|| (NULL == transferParams->buffer) || (0 == transferParams->bufferSize))	
		return -1;
	
	//	validate the file name
	if (0 == FlashDrive_ValidateFileName(transferParams->filename))
		return -1;
	
	//	copy the requestor parameters
  MatrixFTPClient.server.address = transferParams->serverAddress;
  MatrixFTPClient.server.accessCode = transferParams->serverAccessCode;
	MatrixFTPClient.requester.buffer = NULL;
	MatrixFTPClient.requester.bufferSize = 0;
	MatrixFTPClient.requester.callback = transferParams->callback;
	strcpy(MatrixFTPClient.file.name, transferParams->filename);
	
	//	initialize the transmitter
	MatrixTransmitter_StartMessage(transferParams->serverAddress);

	//	send the request key
	MatrixTransmitter_AddInt16(KeyRequestFileDelete);

	//	send the file name
	MatrixTransmitter_AddString(transferParams->filename);

	//	send the server access code
	MatrixTransmitter_AddInt32(transferParams->serverAccessCode);
	
	//	send the request
	FinishRequest(KeyResponseFileDelete);
	return 0;
}


//	private methods........................................................

/**
	* @brief  Helper method does the following:
	*					- Sets the expected response
	*					- Starts the response timer
	*					- Sends request to server
	*
	* @param  expectedResponse: The response expected from the server.
  * @retval None.
  */
static void FinishRequest(uint16_t expectedResponse)
{
	//	send request
	if (0 == MatrixTransmitter_FinishMessage())
	{
    //  set the expected response and start the response timer
		MatrixFTPClient.server.expectedResponse = expectedResponse;
		MatrixFTPClient.server.responseTimeout =
			Matrix.systemTime + MATRIX_MAX_FILE_REQUEST_RESPONSE_TIME_MS;

		//	set the receiver sender address filter
		MatrixReceiver_SetSenderAddressFilter(MatrixFTPClient.server.address);
	}
	else
	{
		//	if failed to send message, then end transaction
		EndTransaction(KeyResponseFtpServerError);
	}
}

/**
	* @brief  Helper method populates callback info.
	* @param  callbackInfo: The callback info structure to populate.
	* @param  responseKey: The transaction response key provided to the client caller.
  * @retval None.
  */
static void PopulateCallbackInfo(FTP_CLIENT_CALLBACK_INFO *callbackInfo, uint16_t responseKey)
{
	//  response key
	callbackInfo->responseKey = responseKey;

	//  server info
	callbackInfo->serverAddress = MatrixFTPClient.server.address;
	callbackInfo->serverAccessCode = MatrixFTPClient.server.accessCode;
	callbackInfo->serverGuid[0] = MatrixFTPClient.server.guid[0];
	callbackInfo->serverGuid[1] = MatrixFTPClient.server.guid[1];
	callbackInfo->serverGuid[2] = MatrixFTPClient.server.guid[2];
	callbackInfo->serverGuid[3] = MatrixFTPClient.server.guid[3];

	//  file info
	strncpy(callbackInfo->filename, MatrixFTPClient.file.name, MATRIX_FILE_NAME_LENGTH);
	callbackInfo->filename[MATRIX_FILE_NAME_LENGTH] = 0;
	callbackInfo->fileDate = MatrixFTPClient.file.date;
	callbackInfo->fileDataSize = MatrixFTPClient.file.dataSize;
	callbackInfo->fileDataChecksum = MatrixFTPClient.file.dataChecksum;
}

/**
	* @brief  Helper method does the following:
	*					- Notifies callback of transaction final status
	*					- Clears the callback, expected response and response timer
	*					- Notifies the server that the transaction is complete
	*
	* @param  responseKey: The transaction response key provided to the client caller.
  * @retval None.
  */
static void EndTransaction(uint16_t responseKey)
{
	FTP_CLIENT_TRANSFER_COMPLETE_CALLBACK callback;
	FTP_CLIENT_CALLBACK_INFO info;
	TOKEN token;
	
	//	clear expected response and receiver sender address filter
	MatrixFTPClient.server.expectedResponse = KeyNull;
	MatrixReceiver_SetSenderAddressFilter(0);
	
	//	let the server know that transaction is complete
	token.address = MatrixFTPClient.server.address;
	token.key = KeyRequestFileTransferComplete;
	token.value = 0;
	Matrix_PrivateSendCanToken(&token); 
	
	//	if callback
	if (NULL != MatrixFTPClient.requester.callback)
	{
    //  clear the callback to accept another during callback
		callback = MatrixFTPClient.requester.callback;
		MatrixFTPClient.requester.callback = NULL;
		
		//	call the client back
		PopulateCallbackInfo(&info, responseKey);
		callback(&info);
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	READ FILE OR FILE INFO FROM SERVER

/**
  * @brief  Requests that the server read a file segment.
	* @param  None.
  * @retval None.
  */
static void RequestReadSegment(void)
{
	//	initialize the transmitter
	MatrixTransmitter_StartMessage(MatrixFTPClient.server.address);

	//	send the request key
	MatrixTransmitter_AddInt16(KeyRequestFileReadSegment);

	//	send the segment index
	MatrixTransmitter_AddInt16((uint32_t)MatrixFTPClient.file.segmentIndex);
	
	//	send the server access code
	MatrixTransmitter_AddInt32(MatrixFTPClient.server.accessCode);
	
	//	send request
	FinishRequest(KeyResponseFileReadSegment);
}

/**
  * @brief  Handles the server response for a file info or file read start request.
	* @param  response: The server response.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval None.
  */
static void HandleFileInfoAndReadStartResponse(uint16_t response,
	uint8_t *body, uint32_t bodySize)
{
	uint16_t filenameLen, i, m;


	//  server must provide valid file name
	filenameLen = FlashDrive_ValidateFileName((char *)body);
	if (0 == filenameLen)
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}

	//	if getting indexed file info, then set file name
	if (KeyResponseFileIndexedInfo == response)
	{
		strcpy(MatrixFTPClient.file.name, (char *)body);
	}
	else  //	else server response file name must match the request file name
	{
		if (0 != strcmp((char *)body, MatrixFTPClient.file.name))
		{
			EndTransaction(KeyResponseFtpServerError);
			return;
		}
	}

	//	server response body size must includes data size, data checksum and timestamp
	if (bodySize < (uint32_t)(filenameLen + (1 + 4 + 2 + 4)))
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}
	
	//	get the file data size and checksum
	body += (filenameLen + 1);
	i = 4;
	while (i--)
	{
		MatrixFTPClient.file.dataSize <<= 8;
		MatrixFTPClient.file.dataSize |= *body++;
	}
	i = 2;
	while (i--)
	{
		MatrixFTPClient.file.dataChecksum <<= 8;
		MatrixFTPClient.file.dataChecksum |= *body++;
	}
	i = 4;
	while (i--)
	{
		MatrixFTPClient.file.date <<= 8;
		MatrixFTPClient.file.date |= *body++;
	}
	
	//	if this was a product info file info request, and body has 16 additional guid bytes
	if ((KeyResponseFileInfo == response)
		&& (0 == strcmp(MatrixFTPClient.file.name, MATRIX_PRODUCT_INFO_FILE_NAME))
		&& (bodySize >= (uint32_t)(filenameLen + (1 + 4 + 2 + 4 + 16))))
	{
		//  get the guid and convert to server access code
		for (m = 0; m < 4; ++m)
		{
				i = 4;
				while (i--)
				{
						MatrixFTPClient.server.guid[m] <<= 8;
						MatrixFTPClient.server.guid[m] |= *body++;
				}
		}
		MatrixFTPClient.server.accessCode = GenerateServerAccessCode(MatrixFTPClient.server.guid);
	}
	
  //  a zero data size indicates that the file was not found
	if (0 == MatrixFTPClient.file.dataSize)
	{
		EndTransaction(KeyResponseFileNotFound);
		return;
	}
	
	//	if this was just a file info request, or no client buffer available, then done
	if ((KeyResponseFileInfo == response) 
		|| (NULL == MatrixFTPClient.requester.buffer) || (0 >= MatrixFTPClient.requester.bufferSize))
	{
		EndTransaction(KeyResponseFileInfoComplete);
		return;
	}

	//	clear number of bytes transferred and make first segment request
	MatrixFTPClient.file.segmentIndex = 0;
	RequestReadSegment();
}

/**
  * @brief  Handles a file read segment server response.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval Returns 0 on success, else -1.
  */
static void HandleFileReadSegmentResponse(uint8_t *body, uint32_t bodySize)
{
	uint16_t segmentIndex, key;
	int32_t dataIndex, numCopyBytes;
	
	//	server response must have a body of at least 3 bytes
	if ((NULL == body) || (3 > bodySize))
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}
	
	//	server response must match the requested segment index
	segmentIndex = *body++;
	segmentIndex = (segmentIndex << 8) | *body++;
	if (segmentIndex != MatrixFTPClient.file.segmentIndex)
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}
	
  //  get least of file bytes and requester buffer bytes remaining
  dataIndex = (int32_t)(MatrixFTPClient.file.segmentIndex * MATRIX_MAX_FILE_SEGMENT_LENGTH);
  numCopyBytes = (int32_t)MIN(bodySize - 2, MatrixFTPClient.file.dataSize - dataIndex);
  numCopyBytes = (int32_t)MIN(numCopyBytes, MatrixFTPClient.requester.bufferSize - dataIndex);	
	
	//	copy bytes to requested buffer
	if (0 < numCopyBytes)
		memcpy(MatrixFTPClient.requester.buffer, body, numCopyBytes);
	
	//	if whole file transferred or requestor buffer full, then done
	if ((0 >= (MatrixFTPClient.file.dataSize - dataIndex))
		|| (0 >= (MatrixFTPClient.requester.bufferSize - dataIndex)))
	{
		//	verify file data checksum
		if (dataIndex > MatrixFTPClient.requester.bufferSize)
			dataIndex = MatrixFTPClient.requester.bufferSize;
		key = (MatrixFTPClient.file.dataChecksum ==
			Matrix_ComputeCRC16(MatrixFTPClient.requester.buffer, (uint32_t)dataIndex)) ?
			KeyResponseFileReadComplete : KeyResponseFileChecksumError;

		//	complete transaction with checksum result
		EndTransaction(key);
	}	
	
	//  else more data to go, so request another segment
	else
	{
		++MatrixFTPClient.file.segmentIndex;
		RequestReadSegment();
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	WRITE FILE TO SERVER

/**
  * @brief  Requests that the server write a file segment.
	* @param  None.
  * @retval Returns 0 on success, else -1.
  */
static void RequestWriteSegment(void)
{
  int32_t dataIndex, numCopyBytes;
	
	//	get the bytes remaining to write
	dataIndex = (int32_t)(MatrixFTPClient.file.segmentIndex * MATRIX_MAX_FILE_SEGMENT_LENGTH);
	numCopyBytes = (int32_t)MIN(MatrixFTPClient.file.dataSize - dataIndex,
		MATRIX_MAX_FILE_SEGMENT_LENGTH);
	if (0 >= numCopyBytes)
	{
		EndTransaction(KeyResponseFileWriteComplete);
		return;
	}

	//	initialize the transmitter
	MatrixTransmitter_StartMessage(MatrixFTPClient.server.address);

	//	send the request key
	MatrixTransmitter_AddInt16(KeyRequestFileWriteSegment);

	//	send the segment index
	MatrixTransmitter_AddInt16(MatrixFTPClient.file.segmentIndex);
	
	//	send the access code
	MatrixTransmitter_AddInt32(MatrixFTPClient.server.accessCode);

  //	send the segment data
  while (0 != numCopyBytes--)
    MatrixTransmitter_AddByte(MatrixFTPClient.requester.buffer[dataIndex++]);
	
	//	send request
	FinishRequest(KeyResponseFileWriteSegment);
}

/**
  * @brief  Handles a file write start server response.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval Returns 0 on success, else -1.
  */
static void HandleFileWriteStartResponse(uint8_t *body, uint32_t bodySize)
{
	uint16_t filenameLen;

	//  server must provide valid file name
	filenameLen = FlashDrive_ValidateFileName((char *)body);
	if (0 == filenameLen)
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}

	//	server response file name must match the request file name
	if (0 != strcmp((char *)body, MatrixFTPClient.file.name))
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}
	
	//	clear number of bytes transferred and write the first segment
	MatrixFTPClient.file.segmentIndex = 0;
	RequestWriteSegment();
}

/**
  * @brief  Handles a file write segment server response.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval Returns 0 on success, else -1.
  */
static void HandleFileWriteSegmentResponse(uint8_t *body, uint32_t bodySize)
{
	uint16_t segmentIndex;
	
	//	server response must return a body of least 2 bytes for segment index
	if ((NULL == body) || (2 > bodySize))
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}
	
	//	server response must match the requested segment index
	segmentIndex = *body++;
	segmentIndex = (segmentIndex << 8) | *body++;
	if (segmentIndex != MatrixFTPClient.file.segmentIndex)
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}

	//	write the next segment
	++MatrixFTPClient.file.segmentIndex;
	RequestWriteSegment();
}

/**
  * @brief  Handles the server response for a file info or file read start request.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval None.
  */
static void HandleFileDeleteResponse(uint8_t *body, uint32_t bodySize)
{
	uint16_t filenameLen;

	//  server must provide valid file name
	filenameLen = FlashDrive_ValidateFileName((char *)body);
	if (0 == filenameLen)
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}

	//	server response file name must match the request file name
	if (0 != strcmp((char *)body, MatrixFTPClient.file.name))
	{
		EndTransaction(KeyResponseFtpServerError);
		return;
	}

	//	end transaction
	EndTransaction(KeyResponseFileDeleteComplete);
}
