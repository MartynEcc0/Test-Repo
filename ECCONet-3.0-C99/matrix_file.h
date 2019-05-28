/**
  ******************************************************************************
  * @file    		matrix_file.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		The Matrix library file metadata object.
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

#ifndef __MATRIX_FILE_H
#define __MATRIX_FILE_H

#include <stdint.h>


//	file name length 8.3
#define MATRIX_FILE_NAME_LENGTH		12



/**
  * @brief  The Matrix file metadata object.
	*
	*					Note: The flash file system has a similar file header object,
	*					but with additional fields and a non-zero terminated filename.
  */
typedef struct
{
	//	the file data location
	//	this does not get used when calling FlashDrive_WriteFileHeader().
	uint32_t dataLocation;
	
	//	the file data size
	uint32_t dataSize;
	
	//	the file data CRC
	uint16_t dataChecksum;
	
	//	the file flash drive volume
	uint16_t volumeIndex;
	
	//	the file timestamp in seconds since 00:00:00 Jan 1, 2017 
	uint32_t timestamp;
	
	//	the file name for writes
	char name[MATRIX_FILE_NAME_LENGTH + 1];

} MATRIX_FILE_METADATA;



#endif  //  __MATRIX_FILE_H
