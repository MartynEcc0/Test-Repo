/**
  ******************************************************************************
  * @file    		flash_drive_data.c
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		Dec 2017
  * @brief   		File system using FLASH storage and 8.3 file names.
	*
	*							Methods for reading and writing file headers.
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
	
#include <string.h>
#include "matrix.h"
#include "matrix_flash_drive.h"
#include "matrix_lib_interface.h"



/**
  * @brief  Reads a file header from flash.
	* @param  volumeIndex: Index of flash drive volume to access.
	* @param  address: Address of header in flash.
	* @param  outHeader: Pointer to a header.
  * @retval None.
  */
FLASH_DRIVE_STATUS FlashDrive_ReadFileHeader(uint16_t volumeIndex, uint32_t address, FLASH_DRIVE_FILE *out_Header)
{
	//	validate inputs
	if (NULL == out_Header)
		return FDEC_INPUT_NOT_VALID;
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashRead))
		return FDEC_NO_APP_SUPPORT;
	
	//	validate volume
	if (volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;

	//	read the header
	if (0 != Matrix.appInterface->flashRead(volumeIndex, address, (uint8_t *)out_Header,
		sizeof(FLASH_DRIVE_FILE)))
		return FDEC_FLASH_READ_ERROR;
	return FDEC_OK;
}

/**
  * @brief  Gets a file's header and header location in flash.
	*					Note: Does not validate the file data checksum.
	*
	* @param  volumeIndex: The flash drive volume index.
	* @param  filename: The file name.
	* @param  out_Header: Pointer to a variable to hold the header.
	*											If null, no header is returned.
	* @param  out_Location: Pointer to an variable to hold the header address in flash.
	*											  If null, no address is returned.
  * @retval Returns 0 if file is found, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_GetFile(uint16_t volumeIndex, char* filename,
	FLASH_DRIVE_FILE *out_Header, uint32_t *out_Location)
{
	FLASH_DRIVE_FILE header;
	uint32_t volumeHeaderAddress, volumeLastAddress;
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashRead))
		return FDEC_NO_APP_SUPPORT;
	
	//	valid file name
	if (0 == FlashDrive_ValidateFileName(filename))
		return FDEC_INVALID_FILE_NAME;
	
	//	validate volume
	if (volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
	//	while file headers
	volumeHeaderAddress = Matrix.appInterface->flashVolumes[volumeIndex].baseAddress;
	volumeLastAddress = volumeHeaderAddress + Matrix.appInterface->flashVolumes[volumeIndex].size;
	while (volumeHeaderAddress < volumeLastAddress)
	{
		//	read in header
		FlashDrive_ReadFileHeader(volumeIndex, volumeHeaderAddress, &header);
		
		//	if header not written, then break
		if (FLASH_DRIVE_FILE_KEY_UNUSED == header.key)
			break;
		
		//	if file found
		if ((FLASH_DRIVE_FILE_KEY_ACTIVE == header.key) &&
			(0 == strncmp(filename, header.name, 12)))
		{
			//	if file header not corrupt
			if (header.checksum == FlashDrive_ComputeHeaderCRC16(&header))
			{
				//	return header and address with success
				if (out_Header != NULL)
					*out_Header = header;
				if (out_Location != NULL)
					*out_Location = volumeHeaderAddress;
				return FDEC_OK;
			}
			else
				return FDEC_FILE_HEADER_CORRUPTED;
		}
		
		//	bump address
		volumeHeaderAddress += sizeof(FLASH_DRIVE_FILE);
	}
	return FDEC_FILE_NOT_FOUND;
}

/**
  * @brief  Gets an indexed file's header and data location in flash.
	*					Note: Does not validate the file data checksum.
	*
	* @param  volumeIndex: Index of flash drive volume to access.
	* @param  fileIndex: The file header index.
	* @param  out_Header: Pointer to a variable to hold the header.
	*											If null, no header is returned.
	* @param  out_Location: Pointer to an variable to hold the header address in flash.
	*											  If null, no address is returned.
  * @retval Returns 0 if file is found, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_GetIndexedFile(uint16_t volumeIndex, uint32_t fileIndex,
	FLASH_DRIVE_FILE *out_Header, uint32_t *out_Location)
{
	FLASH_DRIVE_FILE header;
	uint16_t index;
	uint32_t volumeHeaderAddress, volumeLastAddress;
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashRead))
		return FDEC_NO_APP_SUPPORT;
	
	//	validate volume
	if (volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
	//	while file headers
	volumeHeaderAddress = Matrix.appInterface->flashVolumes[volumeIndex].baseAddress;
	volumeLastAddress = volumeHeaderAddress + Matrix.appInterface->flashVolumes[volumeIndex].size;
	index = 0;
	while (volumeHeaderAddress < volumeLastAddress)
	{
		//	read in header
		FlashDrive_ReadFileHeader(volumeIndex, volumeHeaderAddress, &header);
		
		//	if header not written, then break
		if (FLASH_DRIVE_FILE_KEY_UNUSED == header.key)
			break;
		
		//	if active file found and header not corrupt
		if ((FLASH_DRIVE_FILE_KEY_ACTIVE == header.key)
			&& (header.checksum == FlashDrive_ComputeHeaderCRC16(&header)))
		{
			//	if have indexed file
			if (fileIndex == index)
			{
				//	return header and address with success
				if (out_Header != NULL)
					*out_Header = header;
				if (out_Location != NULL)
					*out_Location = volumeHeaderAddress;
				return FDEC_OK;
			}
			else
				++index;
		}
		
		//	bump address
		volumeHeaderAddress += sizeof(FLASH_DRIVE_FILE);
	}
	return FDEC_FILE_NOT_FOUND;
}

/**
  * @brief  Gets a file's metadata.
	* @param  file: A file metadata structure, with the volume and file name already set.
  * @retval Returns 0 if file is found, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_GetFileMetadata(MATRIX_FILE_METADATA *file)
{
	FLASH_DRIVE_FILE header;
	uint32_t volumeHeaderAddress, volumeLastAddress;
	
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashRead))
		return FDEC_NO_APP_SUPPORT;
	
	//	validate volume
	if (file->volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
	//	valid file name
	if (0 == FlashDrive_ValidateFileName(file->name))
		return FDEC_INVALID_FILE_NAME;
	
	//	while file headers
	volumeHeaderAddress = Matrix.appInterface->flashVolumes[file->volumeIndex].baseAddress;
	volumeLastAddress = volumeHeaderAddress + Matrix.appInterface->flashVolumes[file->volumeIndex].size;
	while (volumeHeaderAddress < volumeLastAddress)
	{
		//	read in header
		FlashDrive_ReadFileHeader(file->volumeIndex, volumeHeaderAddress, &header);
		
		//	if header not written, then break
		if (FLASH_DRIVE_FILE_KEY_UNUSED == header.key)
			break;
		
		//	if file found
		if ((FLASH_DRIVE_FILE_KEY_ACTIVE == header.key) &&
			(0 == strncmp(file->name, header.name, 12)))
		{
			//	if file header not corrupt
			if (header.checksum == FlashDrive_ComputeHeaderCRC16(&header))
			{
				//	set the file metadata and return
				file->dataLocation = header.dataLocation;
				file->dataSize = header.dataSize;
				file->dataChecksum = header.dataChecksum;
				file->timestamp = header.timestamp;
				return FDEC_OK;
			}
			else
				return FDEC_FILE_HEADER_CORRUPTED;
		}
		
		//	bump address
		volumeHeaderAddress += sizeof(FLASH_DRIVE_FILE);
	}
	return FDEC_FILE_NOT_FOUND;
}

/**
  * @brief  Gets an indexed file's metadata.
	* @param  fileIndex: The file header index.
	* @param  file: A file metadata structure, with the volume already set.
  * @retval Returns 0 if file is found, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_GetIndexedFileMetadata(uint32_t fileIndex, MATRIX_FILE_METADATA *file)
{
	FLASH_DRIVE_FILE header;
	uint16_t index;
	uint32_t volumeHeaderAddress, volumeLastAddress;
	
	//	validate input
	if (NULL == file)
		return FDEC_INPUT_NOT_VALID;
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashRead))
		return FDEC_NO_APP_SUPPORT;
	
	//	validate file volume
	if (file->volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
	//	while file headers
	volumeHeaderAddress = Matrix.appInterface->flashVolumes[file->volumeIndex].baseAddress;
	volumeLastAddress = volumeHeaderAddress + Matrix.appInterface->flashVolumes[file->volumeIndex].size;
	index = 0;
	while (volumeHeaderAddress < volumeLastAddress)
	{
		//	read in header
		FlashDrive_ReadFileHeader(file->volumeIndex, volumeHeaderAddress, &header);
		
		//	if header not written, then break
		if (FLASH_DRIVE_FILE_KEY_UNUSED == header.key)
			break;
		
		//	if active file found and header not corrupt
		if ((FLASH_DRIVE_FILE_KEY_ACTIVE == header.key)
			&& (header.checksum == FlashDrive_ComputeHeaderCRC16(&header)))
		{
			//	if have indexed file
			if (fileIndex == index)
			{
				//	set the file metadata and return
				strncpy(file->name, header.name, MATRIX_FILE_NAME_LENGTH);
				file->name[MATRIX_FILE_NAME_LENGTH] = 0;
				file->dataLocation = header.dataLocation;
				file->dataSize = header.dataSize;
				file->dataChecksum = header.dataChecksum;
				file->timestamp = header.timestamp;
				return FDEC_OK;
			}
			++index;
		}
		
		//	bump address
		volumeHeaderAddress += sizeof(FLASH_DRIVE_FILE);
	}
	return FDEC_FILE_NOT_FOUND;
}

/**
  * @brief  Writes a file header to the given flash drive volume,
	*					and allocates the file data area.
	*
	*					If not enough room in volume for file header and data,
	*					then the header does not get written.
	*
	* @param  file: A file metadata object.
  */
FLASH_DRIVE_STATUS FlashDrive_WriteFileHeader(MATRIX_FILE_METADATA *file)
{
	FLASH_DRIVE_VOLUME_STATS volumeStats;
	uint32_t i, tries, fileStoredSize;
	uint8_t hdr[sizeof(FLASH_DRIVE_FILE) + 2];
    FLASH_DRIVE_FILE *p_hdr = (FLASH_DRIVE_FILE *)hdr;
	FLASH_DRIVE_STATUS status;
	
	//	validate input
	if (0 >= file->dataSize)
		return FDEC_INPUT_NOT_VALID;
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashWrite))
		return FDEC_NO_APP_SUPPORT;
	
	//	validate file volume
	if (file->volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
		//	validate file name
	if (0 == FlashDrive_ValidateFileName(file->name))
		return FDEC_INVALID_FILE_NAME;

	//	get total size of file in memory
	fileStoredSize = sizeof(FLASH_DRIVE_FILE) + file->dataSize + 4;
	
	//	try to up to five times
	//	will clean and/or compact the volume if necessary to make room
	status = FDEC_NOT_ENOUGH_ROOM_IN_VOLUME;
	for (tries = 0; tries < 5; tries++)
	{
		//	get volume stats
		FlashDrive_GetVolumeStatistics(file->volumeIndex, &volumeStats, file->name);
		
		//	if volume is corrupted
		if (volumeStats.IsCorrupted)
		{
			//	compact the volume
			FlashDrive_CompactVolume(file->volumeIndex);
		}
		//	else if writeable space is not enough to write file
		else if (volumeStats.WriteableSpace < fileStoredSize)
		{
			//	if available space is enough to write file
			if (volumeStats.AvailableSpace >= fileStoredSize)
			{
				//	erase any previous version of the file
				for (i = 0; i < 2; i++)
					FlashDrive_EraseFile(file->volumeIndex, file->name);

				//	compact the volume
				FlashDrive_CompactVolume(file->volumeIndex);
			}
		}
		//	else volume is good and has enough writable space to write file
		else
		{
			//	erase any previous version of the file
			for (i = 0; i < 2; i++)
				FlashDrive_EraseFile(file->volumeIndex, file->name);
	
			//	write the file header
			p_hdr->key = FLASH_DRIVE_FILE_KEY_ACTIVE;
			strncpy(p_hdr->name, file->name, 12);
			p_hdr->timestamp = file->timestamp;
			//	header data location is on 4-byte boundary
			p_hdr->dataLocation = (volumeStats.LowestDataAddress - file->dataSize) & 0xfffffffc;
			p_hdr->dataSize = file->dataSize;
			p_hdr->dataChecksum =	file->dataChecksum;
			p_hdr->dataLocationOffset = 0;
			p_hdr->checksum = FlashDrive_ComputeHeaderCRC16(p_hdr);
			hdr[sizeof(FLASH_DRIVE_FILE) + 0] = FLASH_DRIVE_FILE_ERASE_VALUE;
			hdr[sizeof(FLASH_DRIVE_FILE) + 1] = FLASH_DRIVE_FILE_ERASE_VALUE;
			if (0 == Matrix.appInterface->flashWrite(file->volumeIndex, volumeStats.NextHeaderAddress,
				hdr, sizeof(FLASH_DRIVE_FILE) + 2))
				return FDEC_OK;
		}
	}
	return status;
}


#ifdef PREVIOUS_VERSION
/**
  * @brief  Writes a file header to the given flash drive volume,
	*					and allocates the file data area.
	*
	*					If not enough room in volume for file header and data,
	*					then the header does not get written.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: A null-terminated 8.3 file name.
	* @param  dataSize: The file size in bytes.
	* @param  timestamp: The file timestamp.
  * @retval Returns 0 on success, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_WriteFileHeader(uint16_t volumeIndex, char* filename,
	int32_t dataSize, uint32_t timestamp)
{
	FLASH_DRIVE_VOLUME_STATS volumeStats;
	FLASH_DRIVE_FILE header;
	uint32_t i, tries, fileStoredSize;
	int status;
	
	//	validate input
	if (0 >= dataSize)
		return FDEC_INPUT_NOT_VALID;
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashWrite))
		return FDEC_NO_APP_SUPPORT;
	
	//	validate file volume
	if (volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
		//	valid file name
	if (0 == FlashDrive_ValidateFileName(filename))
		return FDEC_INVALID_FILE_NAME;

	//	get total size of file in memory
	fileStoredSize = sizeof(FLASH_DRIVE_FILE) + dataSize + 4;
	
	//	try to up to five times
	//	will clean and/or compact the volume if necessary to make room
	status = FDEC_NOT_ENOUGH_ROOM_IN_VOLUME;
	for (tries = 0; tries < 5; tries++)
	{
		//	get volume stats
		FlashDrive_GetVolumeStatistics(volumeIndex, &volumeStats, filename);
		
		//	if volume is corrupted
		if (volumeStats.IsCorrupted)
		{
			//	compact the volume
			FlashDrive_CompactVolume(volumeIndex);
		}
		//	else if writeable space is not enough to write file
		else if (volumeStats.WriteableSpace < fileStoredSize)
		{
			//	if available space is enough to write file
			if (volumeStats.AvailableSpace >= fileStoredSize)
			{
				//	erase any previous version of the file
				for (i = 0; i < 2; i++)
					FlashDrive_EraseFile(volumeIndex, filename);

				//	compact the volume
				FlashDrive_CompactVolume(volumeIndex);
			}
		}
		//	else volume is good and has enough writable space to write file
		else
		{
			//	erase any previous version of the file
			for (i = 0; i < 2; i++)
				FlashDrive_EraseFile(volumeIndex, filename);
	
			//	write the file header
			header.key = FLASH_DRIVE_FILE_KEY_ACTIVE;
			strncpy(header.name, filename, 12);
			header.timestamp = timestamp;
			//	header data location is on 4-byte boundary
			header.dataLocation = (volumeStats.LowestDataAddress - dataSize) & 0xfffffffc;
			header.dataSize = dataSize;
			header.dataChecksum =	FlashDrive_ComputeDataCRC16((uint8_t *)header.dataLocation, dataSize);
			header.dataLocationOffset = 0;
			header.checksum = FlashDrive_ComputeHeaderCRC16(&header);
			if (0 == Matrix.appInterface->flashWrite(volumeIndex, volumeStats.NextHeaderAddress,
				&header, sizeof(FLASH_DRIVE_FILE)))
				return FDEC_OK;
		}
	}
	return status;
}

#endif

