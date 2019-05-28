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

#ifndef __MATRIX_FTP_CLIENT_H
#define __MATRIX_FTP_CLIENT_H

#include <stdio.h>
#include <stdint.h>
#include "matrix_flash_drive.h"
#include "matrix_tokens.h"
#include "matrix_config.h"


/**
  * @brief  The Matrix ftp server object.
  */
typedef struct
{
	//	the server file access code
	uint32_t accessCode;
	
	//	the server guid
	uint32_t guid[4];
	
	//	the server CAN address
	uint16_t address;
	
	//	the expected server response, or key null if idle
	uint16_t expectedResponse;
	
	//	the server timeout
	uint32_t responseTimeout;
	
} MFTP_SERVER;

/**
  * @brief  The Matrix ftp client file object.
  */
typedef struct
{
	//	the file date 
	uint32_t date;
	
	//	the file data size
	uint32_t dataSize;
	
	//	the file data CRC
	uint16_t dataChecksum;
	
	//	the number of bytes transferred
	int32_t segmentIndex;
	
	//	the file name
	char name[MATRIX_FILE_NAME_LENGTH + 1];
	
} MFTP_CLIENT_FILE;

/**
  * @brief  The Matrix ftp client requester object.
  */
typedef struct
{
	//	requester data buffer
	uint8_t *buffer;
	
	//	requester data buffer size
	int32_t bufferSize;
	
	//	requester transfer complete callback
	FTP_CLIENT_TRANSFER_COMPLETE_CALLBACK callback;
	
} MFTP_CLIENT_REQUESTER;

/**
  * @brief  The Matrix file transfer data object.
  */
typedef struct
{
	//	the transaction requester
	MFTP_CLIENT_REQUESTER requester;

	//	the server
	MFTP_SERVER server;
	
	//	the file information
	MFTP_CLIENT_FILE file;
	

} MATRIX_FTP_CLIENT_OBJECT;
//	private extern MATRIX_FTP_CLIENT_OBJECT MatrixFTPClient;


/**
  * @brief  Resets the Matrix ftp client.
  * @param  None.
  * @retval None.
  */
void MatrixFTPClient_Reset(void);

/**
  * @brief  Clocks the Matrix ftp client.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
void MatrixFTPClient_Clock(void);


	/**
  * @brief  Handles incoming server response tokens.
	* @param  senderAddress: The CAN address of the sender.
	* @param  responseKey: The token response key.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval None.
  */
extern void MatrixFTPClient_ServerResponseIn(uint16_t senderAddress, uint16_t responseKey,
	uint8_t *body, uint32_t bodySize);

	
#endif  //  __MATRIX_FTP_CLIENT_H

