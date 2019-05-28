/**
  ******************************************************************************
  * @file    		matrix_ftp_requests.h
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

#ifndef __MATRIX_FTP_SERVER_H
#define __MATRIX_FTP_SERVER_H

#include <stdio.h>
#include <stdint.h>
#include "matrix_tokens.h"
#include "matrix_config.h"
#include "matrix_flash_drive.h"
#include "matrix_lib_interface.h"


/**
  * @brief  The Matrix ftp client object.
  */
typedef struct
{
	//	the client CAN address
	uint16_t address;
	
	//	the current client request
	uint16_t request;
	
	//	the request timeout
	uint32_t requestTimeout;
	
} MFTP_CLIENT;


/**
  * @brief  The Matrix ftp server object.
  */
typedef struct
{
	//	the client
	MFTP_CLIENT client;
	
	//	the file
	MATRIX_FILE_METADATA file;
	
	//	this server's access code
	uint32_t accessCode;

} MATRIX_FTP_SERVER_OBJECT;
//	private extern MATRIX_FTP_SERVER_OBJECT MatrixFTPServer;


/**
  * @brief  Resets the Matrix ftp server.
  * @param  None.
  * @retval None.
  */
void MatrixFTPServer_Reset(void);

/**
  * @brief  Clocks the Matrix ftp server.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
void MatrixFTPServer_Clock(void);

/**
  * @brief  Handles incoming client request tokens.
	* @param  senderAddress: The CAN address of the sender.
	* @param  key: The token key.
	* @param  body: A pointer to the message body.
	* @param  bodySize: The message body size in bytes.
  * @retval None.
  */
void MatrixFTPServer_ClientRequestIn(uint16_t senderAddress, uint16_t requestKey,
	uint8_t *body, uint32_t bodySize);


#endif  //  __MATRIX_FTP_SERVER_H

