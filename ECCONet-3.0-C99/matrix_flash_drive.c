/**
  ******************************************************************************
  * @file    		flash_drive.c
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		File system using FLASH storage and 8.3 file names.
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
#include "matrix_flash_drive.h"
#include "matrix_lib_interface.h"


/**
  * @brief  Reads a file from the given flash drive volume.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  buffer: A data buffer to hold the data to read.
	* @param  bufferSize: The data buffer size in bytes.
	* @param  out_timestamp: Pointer to a 32-bit value to hold the file timestamp.
  * @retval Returns 0 on success, else -1.
  */
FLASH_DRIVE_STATUS FlashDrive_ReadFile(uint16_t volumeIndex, char *filename,
	void *buffer, uint32_t bufferSize, uint32_t *out_timestamp)
{
	FLASH_DRIVE_FILE file;
	FLASH_DRIVE_STATUS status;
	
	//	validate inputs
	if ((NULL == buffer) || (0 >= bufferSize))
		return FDEC_INPUT_NOT_VALID;

	//	get the file header
	//	validates app support, file name and volume
	status = FlashDrive_GetFile(volumeIndex, filename, &file, NULL);
	if (0 != status)
		return status;
	
	//	if buffer is too small to hold file, return
	if (bufferSize < file.dataSize)
		return FDEC_READ_BUFFER_TOO_SMALL;

	//	read data
	if (0 != Matrix.appInterface->flashRead(volumeIndex, file.dataLocation, buffer, file.dataSize))
		return FDEC_FLASH_READ_ERROR;
	
	//	set timestamp
	*out_timestamp = file.timestamp;
	
	//	check the data checksum
	if (file.dataChecksum != FlashDrive_ComputeDataCRC16(buffer, file.dataSize))
		return FDEC_FILE_DATA_CORRUPTED;
	return FDEC_OK;
}

/**
  * @brief  Writes a file to the given flash drive volume if there is room.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: A null-terminated 8.3 file name.
	* @param  data: A pointer to the data to be written.
	* @param  dataSize: The number of data bytes to be written.
	* @param  timestamp: The file timestamp.
  * @retval Returns 0 on success, else -1.
  */
FLASH_DRIVE_STATUS FlashDrive_WriteFile(uint16_t volumeIndex, char *filename,
	void *data, uint32_t dataSize, uint32_t timestamp)
{
	MATRIX_FILE_METADATA file;
	FLASH_DRIVE_STATUS status;
	
	//	validate inputs
	if ((NULL == data) || (0 >= dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	build file metadata
	memset(&file, 0, sizeof(MATRIX_FILE_METADATA));
	strcpy(file.name, filename);
	file.volumeIndex = volumeIndex;
	file.dataLocation = (uint32_t)data;
	file.dataSize = dataSize;
	file.timestamp = timestamp;

	//	compute the data checksum
	file.dataChecksum =	FlashDrive_ComputeDataCRC16(data, dataSize);

	//	write file header and allocate the file data
	if (0 != (status = FlashDrive_WriteFileHeader(&file)))
		return status;
	
	//	write the file data
	return FlashDrive_WriteData(volumeIndex, filename, data, dataSize, 0);
}

/**
  * @brief  Tags a file for deletion.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name to erase.
  * @retval Returns 0 if file found and erased, else -1.
  */
FLASH_DRIVE_STATUS FlashDrive_EraseFile(uint16_t volumeIndex, char *filename)
{
	uint16_t key;
	uint32_t headerAddress;
	FLASH_DRIVE_FILE file;
	FLASH_DRIVE_STATUS status;

	//	get the file header address
	//	validates app support, file name and volume
	status = FlashDrive_GetFile(volumeIndex, filename, &file, &headerAddress);
	if (0 != status)
		return status;

	//	zero first byte of data (if file key lsB is zero, then no effect)
	if (file.dataSize)
	{
		key = 0;
		Matrix.appInterface->flashWrite(volumeIndex, file.dataLocation, &key, 1);
	}
	
	//	rewrite the file key
	key = FLASH_DRIVE_FILE_KEY_DELETED;
	if (0 != Matrix.appInterface->flashWrite(volumeIndex, headerAddress, &key,
		sizeof(((FLASH_DRIVE_FILE*)0)->key)))
		return FDEC_FLASH_WRITE_ERROR;
	return FDEC_OK;
}

