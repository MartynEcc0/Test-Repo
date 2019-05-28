/**
  ******************************************************************************
  * @file    		flash_drive_compact.c
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
  * @brief  Tries to make space in a volume.
	* @param  volumeIndex: Index of flash drive volume.
	* @param  size: The required writable space.
  * @retval Returns 0 on success, else the flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_TryMakeSpace(uint16_t volumeIndex, uint32_t size)
{
	FLASH_DRIVE_VOLUME_STATS volumeStats;
	uint16_t tries;
	
	//	get volume statistics
	FlashDrive_GetVolumeStatistics(volumeIndex, &volumeStats, NULL);
	
	//	if volume intact and already have enough writable space, then return OK
	if (!volumeStats.IsCorrupted && (volumeStats.WriteableSpace >= size))
		return FDEC_OK;

	//	try up to three times to make room in volume
	for (tries = 0; tries < 3; ++tries)
	{
		//	compact the volume, and if successful then get the file header and volume statistics again
		if (0 == FlashDrive_CompactVolume(volumeIndex))
		{
			//	get volume statistics again
			FlashDrive_GetVolumeStatistics(volumeIndex, &volumeStats, NULL);

			//	if have writable enough space, then return OK
			if (!volumeStats.IsCorrupted && (volumeStats.WriteableSpace >= size))
				return FDEC_OK;
		}			
	}

	//	if volume is corrupted then return
	if (volumeStats.IsCorrupted)
		return FDEC_VOLUME_CORRUPTED;
	
	//	return status of whether volume has enough writable space
	if (volumeStats.WriteableSpace >= size)
		return FDEC_OK;
	return FDEC_NOT_ENOUGH_ROOM_IN_VOLUME;
}

/**
  * @brief  Compacts the files in a flash drive volume.
	* @param  volumeIndex: Index of flash drive volume to access.
  * @retval Returns 0 on success, else the flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_CompactVolume(uint16_t volumeIndex)
{
	FLASH_DRIVE_FILE header;
	uint16_t key;
	uint32_t previousDataAddress, lowestDataAddress;
	uint32_t volumeHeaderAddress, highestHeaderAddress, volumeLastAddress;
	FLASH_DRIVE_STATUS status;

	//	verify app support
	if ((NULL == Matrix.appInterface)
		|| (NULL == Matrix.appInterface->flashWrite)
		|| (NULL == Matrix.appInterface->flashRead)
		|| (NULL == Matrix.appInterface->flashErase))
		return FDEC_NO_APP_SUPPORT;
	
	//	validate volume index
	if (volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
	//	volume params
	volumeHeaderAddress = Matrix.appInterface->flashVolumes[volumeIndex].baseAddress;
	volumeLastAddress = volumeHeaderAddress + Matrix.appInterface->flashVolumes[volumeIndex].size;

	//	highest header and lowest data addresses
	highestHeaderAddress = volumeHeaderAddress;
	lowestDataAddress = volumeLastAddress;

	//---------------------------------------------------------------------------//
	//	Move all file headers to start of volume.                                //
	//---------------------------------------------------------------------------//
	
	//	while headers
	status = FDEC_OK;
	while (volumeHeaderAddress < volumeLastAddress)
	{
		//	read in header
		FlashDrive_ReadFileHeader(volumeIndex, volumeHeaderAddress, &header);
		
		//	if at end of headers
		if (FLASH_DRIVE_FILE_KEY_UNUSED == header.key)
			break;

		//	if file header not marked for deletion and not corrupt
		if ((header.key == FLASH_DRIVE_FILE_KEY_ACTIVE) &&
			(header.checksum == FlashDrive_ComputeHeaderCRC16(&header)))
		{
			//	capture lowest new data address and previous data address
			//	store data on 4-byte boundaries
			lowestDataAddress = (lowestDataAddress - header.dataSize) & 0xfffffffc;
			previousDataAddress = header.dataLocation;
			header.dataLocation = lowestDataAddress;
			
			//	if file needs to be re-written
			if ((volumeHeaderAddress != highestHeaderAddress)
				|| (previousDataAddress != header.dataLocation))
			{
				//	recalculate header checksum and write header
				header.checksum = FlashDrive_ComputeHeaderCRC16(&header);
				if (0 != Matrix.appInterface->flashWrite(volumeIndex, highestHeaderAddress,
					&header, sizeof(FLASH_DRIVE_FILE)))
					return FDEC_FLASH_WRITE_ERROR;
				
				//	move file, and if not moved successfully, then abort compaction
				if (0 != (status = FlashDrive_MoveFileData(0, (uint8_t *)lowestDataAddress,
					(uint8_t *)previousDataAddress, header.dataSize)))
					return status;
			}
			
			//	capture highest header address
			highestHeaderAddress += sizeof(FLASH_DRIVE_FILE);
		}

		//	bump header address
		volumeHeaderAddress += sizeof(FLASH_DRIVE_FILE);
	}

	//---------------------------------------------------------------------------//
	//	Erase all information between end of headers and start of file data.     //
	//---------------------------------------------------------------------------//

	//	if have flash erase
	if (NULL != Matrix.appInterface->flashErase)
	{
		if (0 != Matrix.appInterface->flashErase(volumeIndex, highestHeaderAddress,
			lowestDataAddress - highestHeaderAddress))
			return FDEC_FLASH_ERASE_ERROR;
	}
	else  //	else erase next key with flash write
	{
		key = FLASH_DRIVE_FILE_KEY_UNUSED;
		if (0 != Matrix.appInterface->flashWrite(volumeIndex, highestHeaderAddress,
			&key, sizeof(((FLASH_DRIVE_FILE*)0)->key)))
			return FDEC_FLASH_WRITE_ERROR;
	}
	
	//	return success
	return FDEC_OK;
}

