/**
  ******************************************************************************
  * @file    		matrix_flash_drive_statistics.c
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
  * @brief  Gets the number of available flash drive volumes.
	* @param  None.
  * @retval The number of available flash drive volumes.
  */
uint16_t GetNumVolumes(void)
{
	int i;
	
	if (NULL != Matrix.appInterface)
	{
		for (i = 0; i < 3; i++)
			if (Matrix.appInterface->flashVolumes[i].size == 0)
				break;
		return i;
	}
	return 0;
}

/**
  * @brief  Gets a volume's statistics.
	* @param  volumeIndex: Index of flash drive volume to access.
	* @param  volumeStats: A pointer to an object that receives the volume statistics.
	* @param  ignoredFilename: If not null, the available space calculation includes the space
	*						used for all files with a matching name, including those marked for deletion.
  * @retval None.
  */
FLASH_DRIVE_STATUS FlashDrive_GetVolumeStatistics(uint16_t volumeIndex,
	FLASH_DRIVE_VOLUME_STATS *volumeStats, char *ignoredFilename)
{
	FLASH_DRIVE_FILE header;
	uint32_t volumeSize, volumeHeaderAddress, volumeLastAddress;
	uint32_t fileStoredSize;
	bool isDeleted, isNameMatch, isGoodChecksum;

	//	verify app support
	if (NULL == Matrix.appInterface)
		return FDEC_NO_APP_SUPPORT;
	
		//	validate volume index
	if (volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
	//	validate volume stats
	if (volumeStats == NULL)
		return FDEC_INPUT_NOT_VALID;
	
	//	volume params
	volumeSize = Matrix.appInterface->flashVolumes[volumeIndex].size;
	volumeHeaderAddress = Matrix.appInterface->flashVolumes[volumeIndex].baseAddress;
	volumeLastAddress = volumeHeaderAddress + volumeSize;

	//	init stats
	volumeStats->AvailableSpace = volumeSize;
	volumeStats->WriteableSpace = volumeSize;
	volumeStats->NextHeaderAddress = volumeHeaderAddress;
	volumeStats->LowestDataAddress = volumeLastAddress;
	volumeStats->IsCorrupted = false;

	//	while file headers
	while (volumeHeaderAddress < volumeLastAddress)
	{
		//	read in header
		FlashDrive_ReadFileHeader(volumeIndex, volumeHeaderAddress, &header);
		
		//	if header not written, then save next header address and break
		if (FLASH_DRIVE_FILE_KEY_UNUSED == header.key)
		{
			volumeStats->NextHeaderAddress = volumeHeaderAddress;
			break;
		}
		
		//	get stats
		isNameMatch = ((ignoredFilename != NULL) && (0 == strncmp(ignoredFilename, header.name, 12)));
		isDeleted = (FLASH_DRIVE_FILE_KEY_ACTIVE != header.key);
		isGoodChecksum = (header.checksum == FlashDrive_ComputeHeaderCRC16(&header));
				
		if (!isGoodChecksum)
			volumeStats->IsCorrupted = true;
		else
		{
			fileStoredSize = sizeof(FLASH_DRIVE_FILE) + header.dataSize;
			
			//	reduce writeable space by total size of file
			volumeStats->WriteableSpace -= fileStoredSize;
			
			//	if file is not deleted and does not match given name,
			//	then reduce available space by total size of file
			if (!isDeleted && !isNameMatch)
				volumeStats->AvailableSpace -= fileStoredSize;
			
			//	get lowest data address
			if (volumeStats->LowestDataAddress > header.dataLocation)
				volumeStats->LowestDataAddress = header.dataLocation;
		}
		
		//	bump address
		volumeHeaderAddress += sizeof(FLASH_DRIVE_FILE);
	}
	
	//	reduce available space and writeable space by one file header
	//	to always be able to flag the end of the file headers
	if (volumeStats->AvailableSpace >= sizeof(FLASH_DRIVE_FILE))
		volumeStats->AvailableSpace -= sizeof(FLASH_DRIVE_FILE);
	if (volumeStats->WriteableSpace >= sizeof(FLASH_DRIVE_FILE))
		volumeStats->WriteableSpace -= sizeof(FLASH_DRIVE_FILE);
	
	//	return success
	return FDEC_OK;
}
