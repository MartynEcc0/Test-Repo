/**
  ******************************************************************************
  * @file    		matrix_ftp_public.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		Dec 2017
  * @brief   		Public methods for the FTP interface.
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

#ifndef __MATRIX_FTP_PUBLIC_H
#define __MATRIX_FTP_PUBLIC_H

#include <stdint.h>
#include <stdbool.h>




/**
  * @brief  The ftp client transfer complete callback structure.
  * @retval None.
  */	
typedef struct
{
	//	response key
	uint16_t responseKey;
	
	//	server info
	uint8_t serverAddress;
	uint32_t serverAccessCode;
	uint32_t serverGuid[4];
	
	//	file info
	uint32_t fileDate;
	uint32_t fileDataSize;
	uint16_t fileDataChecksum;
	char filename[MATRIX_FILE_NAME_LENGTH + 1];
	
} FTP_CLIENT_CALLBACK_INFO;

/**
  * @brief  The ftp client transfer complete callback.
	* @param  callbackInfo: A pointer to a client callback info structure.
  * @retval None.
  */	
typedef void (*FTP_CLIENT_TRANSFER_COMPLETE_CALLBACK)(FTP_CLIENT_CALLBACK_INFO *callbackInfo);

/**
  * @brief  The ftp client transfer structure.
  * @retval None.
  */	
typedef struct
{
	//	the server network address
	uint8_t serverAddress;
	
	//	the server access code
	uint32_t serverAccessCode;
	
	//	a pointer to the file name
	char *filename;

	//	the volume index, only used for indexed file info
	uint16_t volumeIndex;
	
	//	the file index, only used for indexed file info
	uint32_t fileIndex;
	
	//	the file timestamp
	uint32_t fileTimestamp;
	
	//	a pointer to a data buffer
	//	for a file write, this buffer gives the file data
	//	for a file read, this buffer receives the file data
	void *buffer;
	
	//	the data buffer size as given by the caller
	uint32_t bufferSize;
	
	//	a method to call back when the transaction is complete
	//	this can be null
	FTP_CLIENT_TRANSFER_COMPLETE_CALLBACK callback;
	
	
} FTP_CLIENT_FILE_TRANSFER_PARAMS;


/**
  * @brief  Starts an indexed file info request from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
int MatrixFTPClient_GetIndexedFileInfo(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams);

/**
  * @brief  Starts a file info request from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
int MatrixFTPClient_GetFileInfo(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams);

/**
  * @brief  Starts a file read from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
extern int MatrixFTPClient_ReadFile(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams);
	
/**
  * @brief  Starts a file write to the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
extern int MatrixFTPClient_WriteFile(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams);

/**
  * @brief  Deletes a file from the given Matrix ftp server.
	* @param  transferParams: A pointer to a file transfer parameter structure.
  * @retval Returns 0 on success, else -1.
  */
extern int MatrixFTPClient_DeleteFile(FTP_CLIENT_FILE_TRANSFER_PARAMS *transferParams);



#endif  //  __MATRIX_LIB_INTERFACE_H
