/**
  ******************************************************************************
  * @file    		flash_drive_resize.c
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		Dec 2017
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
  * @brief  Changes a file data size in the volume.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  newDataSize: The new file data size.
  * @retval Returns 0 on success, else flash drive error code.
	*					
  */
FLASH_DRIVE_STATUS FlashDrive_ChangeFileSize(uint16_t volumeIndex, char *filename, uint32_t newDataSize)
{
	FLASH_DRIVE_FILE file;
	uint32_t headerLocation;
	int16_t sizeChange;
	uint32_t volumeHeaderAddress, volumeLastAddress, newDataAddress;
	FLASH_DRIVE_STATUS status;
	

	//	get the file header
	if (0 != (status = FlashDrive_GetFile(volumeIndex, filename, &file, &headerLocation)))
		return status;
	sizeChange = newDataSize - file.dataSize;
	
	//	if file size is not changing, just return
	if (0 == sizeChange)
		return FDEC_OK;
	
	//	else if file is shrinking
	if (0 > sizeChange)
	{
		//	update file header
		file.dataSize = newDataSize;
		file.dataChecksum = FlashDrive_ComputeDataCRC16((uint8_t *)file.dataLocation, newDataSize);
		file.checksum = FlashDrive_ComputeHeaderCRC16(&file);
		if (0 != Matrix.appInterface->flashWrite(volumeIndex, headerLocation, &file, sizeof(FLASH_DRIVE_FILE)))
			return FDEC_FLASH_WRITE_ERROR;
		return FDEC_OK;
	}
	
	//	else if file is growing

	//	try to make space in the volume
	if (0 != (status = FlashDrive_TryMakeSpace(volumeIndex, newDataSize - file.dataSize)))
			return status;

	//	get the file header again to make sure was not corrupted during compaction
	if (0 != (status = FlashDrive_GetFile(volumeIndex, filename, &file, &headerLocation)))
		return status;
	
	//	find last file header
	volumeHeaderAddress = Matrix.appInterface->flashVolumes[volumeIndex].baseAddress;
	volumeLastAddress = volumeHeaderAddress + Matrix.appInterface->flashVolumes[volumeIndex].size;
	while (volumeHeaderAddress < volumeLastAddress)
	{
		//	read file header
		if (0 != Matrix.appInterface->flashRead(volumeIndex, volumeHeaderAddress, (uint8_t *)&file,
			sizeof(FLASH_DRIVE_FILE)))
				return FDEC_FLASH_READ_ERROR;

		//	if header not written, then return error
		if (FLASH_DRIVE_FILE_KEY_UNUSED == file.key)
			break;
		
		//	bump address
		volumeHeaderAddress += sizeof(FLASH_DRIVE_FILE);
	}
	if (volumeHeaderAddress >= volumeLastAddress)
		return FDEC_UNKNOWN_ERROR;
	
	//	run through headers downward
	volumeLastAddress = Matrix.appInterface->flashVolumes[volumeIndex].baseAddress;
	while (volumeHeaderAddress > volumeLastAddress)
	{
		//	bump address
		volumeHeaderAddress -= sizeof(FLASH_DRIVE_FILE);

		//	read file header
		if (0 != Matrix.appInterface->flashRead(volumeIndex, volumeHeaderAddress, (uint8_t *)&file,
			sizeof(FLASH_DRIVE_FILE)))
				return FDEC_FLASH_READ_ERROR;

		//	if an active file
		if (FLASH_DRIVE_FILE_KEY_ACTIVE == file.key)
		{
			//	move file data downward
			newDataAddress = (file.dataLocation - sizeChange) & 0xfffffffc;
			if (0 != (status = FlashDrive_MoveFileData(volumeIndex,	(uint8_t *)newDataAddress,
				(uint8_t *)(file.dataLocation), file.dataSize)))
				return status;

			//	if not the file being changed, then update file's data location
			if (0 != strncmp(filename, file.name, 12))
			{
				//	update file's data location
				file.dataLocation = newDataAddress;
				file.checksum = FlashDrive_ComputeHeaderCRC16(&file);
				if (0 != Matrix.appInterface->flashWrite(volumeIndex, volumeHeaderAddress, &file,
					sizeof(FLASH_DRIVE_FILE)))
					return FDEC_FLASH_WRITE_ERROR;
			}
			else  //  the file being changed
			{
				//	erase copied data at end of file
				//if (0 != Matrix.appInterface->flashErase(volumeIndex, newDataAddress + file.dataSize,
				//		file.dataLocation - newDataAddress))
				//	return FDEC_FLASH_ERASE_ERROR;
				
				//	update file's data location and data size, and return
				file.dataLocation = newDataAddress;
				file.dataSize = newDataSize;
				file.dataChecksum = FlashDrive_ComputeDataCRC16((uint8_t *)file.dataLocation, file.dataSize);
				file.checksum = FlashDrive_ComputeHeaderCRC16(&file);
				if (0 != Matrix.appInterface->flashWrite(volumeIndex, volumeHeaderAddress, &file,
					sizeof(FLASH_DRIVE_FILE)))
					return FDEC_FLASH_WRITE_ERROR;
				return FDEC_OK;
			}
		}
	}
	
	//	should not reach here
	return FDEC_UNKNOWN_ERROR;
}

