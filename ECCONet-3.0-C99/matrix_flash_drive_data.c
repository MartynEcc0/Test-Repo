/**
  ******************************************************************************
  * @file    		flash_drive_data.c
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		Dec 2017
  * @brief   		File system using FLASH storage and 8.3 file names.
	*
	*							Methods for reading and writing file data.
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
  * @brief  Reads data from a flash file.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  data: A pointer to a provided data buffer.
	* @param  dataSize: The number of data bytes to be read.
	* @param  dataLocationOffset: The data location offset within the file.
	* @param  wrap: True if data wraps around at end of file.
  * @retval Returns 0 on success, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_ReadFileData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset, bool wrap)
{
	FLASH_DRIVE_FILE file;
	uint32_t headerLocation, size;
	FLASH_DRIVE_STATUS status;
	
	//	get the file header
	status = FlashDrive_GetFile(volumeIndex, filename, &file, &headerLocation);
	if (0 != status)
		return status;
	
	//	validate inputs
	if ((NULL == data) || (0 == dataSize) || (dataSize > file.dataSize) || (dataLocationOffset >= file.dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	if not wrapping, read must fit within file
	if (!wrap && ((dataLocationOffset + dataSize) > file.dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	read data
	size = (file.dataSize >= (dataLocationOffset + dataSize)) ? dataSize : file.dataSize - dataLocationOffset;
	if (0 != Matrix.appInterface->flashRead(volumeIndex, file.dataLocation + dataLocationOffset,	data, size))
		return FDEC_FLASH_WRITE_ERROR;
	
	//	if wrapping
	if (size < dataSize)
	{
		if (0 != Matrix.appInterface->flashRead(volumeIndex, file.dataLocation, data + size, dataSize - size))
			return FDEC_FLASH_WRITE_ERROR;
	}
	
	//	return success
	return FDEC_OK;
}

/**
  * @brief  Writes data to a flash file, wrapping around to start of file if needed.
	*					Stores the data location offset and updated data checksum in the file header.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  data: A pointer to the data to be written.
	* @param  dataSize: The number of data bytes to be written.
	* @param  dataLocationOffset: The data location offset within the file.
	* @param  wrap: True if write wraps around at end of file.
  * @retval Returns 0 on success, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_WriteFileData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset, bool wrap)
{
	FLASH_DRIVE_FILE file;
	uint32_t headerLocation, size;
	FLASH_DRIVE_STATUS status;
	
	//	get the file header
	status = FlashDrive_GetFile(volumeIndex, filename, &file, &headerLocation);
	if (0 != status)
		return status;
	
	//	validate inputs
	if ((NULL == data) || (0 == dataSize) || (dataSize > file.dataSize) || (dataLocationOffset >= file.dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	if not wrapping, write must fit within file
	if (!wrap && ((dataLocationOffset + dataSize) > file.dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	write data
	size = (file.dataSize >= (dataLocationOffset + dataSize)) ? dataSize : file.dataSize - dataLocationOffset;
	if (0 != Matrix.appInterface->flashWrite(volumeIndex, file.dataLocation + dataLocationOffset,	data, size))
		return FDEC_FLASH_WRITE_ERROR;
	
	//	if wrapping
	if (size < dataSize)
	{
		if (0 != Matrix.appInterface->flashWrite(volumeIndex, file.dataLocation, data + size, dataSize - size))
			return FDEC_FLASH_WRITE_ERROR;
	}
	
	//	update file's location offset and checksums and return
	file.dataLocationOffset = dataLocationOffset;
	file.dataChecksum = FlashDrive_ComputeDataCRC16((uint8_t *)file.dataLocation, file.dataSize);
	file.checksum = FlashDrive_ComputeHeaderCRC16(&file);
	if (0 != Matrix.appInterface->flashWrite(volumeIndex, headerLocation, &file, sizeof(FLASH_DRIVE_FILE)))
		return FDEC_FLASH_WRITE_ERROR;
	return FDEC_OK;
}

/**
  * @brief  Inserts data into a flash file, pushing data above it upward. 
	*					Does not grow the file.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  data: A pointer to the data to be inserted.
	* @param  dataSize: The number of data bytes to be inserted.
	* @param  dataLocationOffset: The data location offset within the file.
  * @retval Returns 0 on success, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_InsertFileData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset)
{
	FLASH_DRIVE_FILE header;
	uint32_t headerLocation;
	FLASH_DRIVE_STATUS status;
	
	//	get the file header
	status = FlashDrive_GetFile(volumeIndex, filename, &header, &headerLocation);
	if (0 != status)
		return status;
	
	//	validate inputs
	if ((NULL == data) || (0 == dataSize) || (dataSize > header.dataSize) || (dataLocationOffset >= header.dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	check for not overwriting file
	if ((dataLocationOffset + dataSize) > header.dataSize)
		return FDEC_NOT_ENOUGH_ROOM_IN_VOLUME;
	
	//	move previous data upward
	if (0 != (status = FlashDrive_MoveFileData(volumeIndex, (uint8_t *)header.dataLocation + (dataLocationOffset + dataSize),
		(uint8_t *)header.dataLocation + dataLocationOffset, header.dataSize - (dataLocationOffset + dataSize))))
		return status;
	
	//	insert the new data
	if (0 != Matrix.appInterface->flashWrite(volumeIndex, header.dataLocation + dataLocationOffset,	data, dataSize))
		return FDEC_FLASH_WRITE_ERROR;
	
	//	update file's checksums and return
	header.dataLocationOffset = dataLocationOffset;
	header.dataChecksum = FlashDrive_ComputeDataCRC16((uint8_t *)header.dataLocation, header.dataSize);
	header.checksum = FlashDrive_ComputeHeaderCRC16(&header);
	if (0 != Matrix.appInterface->flashWrite(volumeIndex, headerLocation, &header,
		sizeof(FLASH_DRIVE_FILE)))
		return FDEC_FLASH_WRITE_ERROR;
	return FDEC_OK;
}

/**
  * @brief  Removes data from a flash file, pulling data above it downward.
	*					Stores the data location offset in the file header.
	*					Does not shrink the file.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  dataSize: The number of data bytes to be removed.
	* @param  locationOffset: The data location offset within the file.
  * @retval Returns 0 on success, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_RemoveFileData(uint16_t volumeIndex, char *filename,
	uint32_t dataSize, uint32_t dataLocationOffset)
{
	FLASH_DRIVE_FILE header;
	uint32_t headerLocation;
	FLASH_DRIVE_STATUS status;
	
	//	get the file header
	status = FlashDrive_GetFile(volumeIndex, filename, &header, &headerLocation);
	if (0 != status)
		return status;
	
	//	validate inputs
	if ((0 == dataSize) || (dataSize > header.dataSize) || (dataLocationOffset >= header.dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	check for not overwriting file
	if ((dataLocationOffset + dataSize) > header.dataSize)
		return FDEC_NOT_ENOUGH_ROOM_IN_VOLUME;
	
	//	move previous data downward
	if (0 != (status = FlashDrive_MoveFileData(volumeIndex, (uint8_t *)header.dataLocation + dataLocationOffset,
		(uint8_t *)header.dataLocation + (dataLocationOffset + dataSize), header.dataSize - (dataLocationOffset + dataSize))))
		return status;
	
	//	erase copied data at end of file
	if (0 != Matrix.appInterface->flashErase(volumeIndex, header.dataLocation + header.dataSize - dataSize,	dataSize))
		return FDEC_FLASH_ERASE_ERROR;
	
	//	update file's location offset and checksums and return
	header.dataLocationOffset = dataLocationOffset;
	header.dataChecksum = FlashDrive_ComputeDataCRC16((uint8_t *)header.dataLocation, header.dataSize);
	header.checksum = FlashDrive_ComputeHeaderCRC16(&header);
	if (0 != Matrix.appInterface->flashWrite(volumeIndex, headerLocation, &header,
		sizeof(FLASH_DRIVE_FILE)))
		return FDEC_FLASH_WRITE_ERROR;
	return FDEC_OK;
}

/**
  * @brief  Moves data within a flash file.
	* @param  dest: The destination address.
	* @param  source: The source address.
	* @param  size: The size of data to move.
  * @retval Returns 0 on success, else flash drive error code.
  */
FLASH_DRIVE_STATUS FlashDrive_MoveFileData(uint16_t volumeIndex, uint8_t *dest, uint8_t *source, int32_t size)
{
	uint8_t buffer[32];
	uint16_t csz;
	
	//	validate inputs
	if ((NULL == dest) || (NULL == source) || (0 >= size))
		return FDEC_INPUT_NOT_VALID;
	
	//	validate volume
	if (volumeIndex >= GetNumVolumes())
		return FDEC_INVALID_VOLUME_INDEX;
	
	//	if moving data to lower address
	if (source > dest)
	{
		while (size > 0)
		{
			csz = (size > 32) ? 32 : size;
			size -= csz;
			if (0 != Matrix.appInterface->flashRead(volumeIndex, (uint32_t)source, buffer, csz))
				return FDEC_FLASH_READ_ERROR;
			source += csz;
			if (0 != Matrix.appInterface->flashWrite(volumeIndex, (uint32_t)dest, buffer, csz))
				return FDEC_FLASH_WRITE_ERROR;
			dest += csz;
		}
	}
	//	else if moving data to higher address
	else if (source < dest)
	{
		source += size;
		dest += size;
		while (size > 0)
		{
			csz = (size > 32) ? 32 : size;
			size -= csz;
			source -= csz;
			if (0 != Matrix.appInterface->flashRead(volumeIndex, (uint32_t)source, buffer, csz))
				return FDEC_FLASH_READ_ERROR;
			dest -= csz;
			if (0 != Matrix.appInterface->flashWrite(volumeIndex, (uint32_t)dest, buffer, csz))
				return FDEC_FLASH_WRITE_ERROR;
		}
	}
	return FDEC_OK;
}

/**
  * @brief  Writes data to a file.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The name of file for which to write data.
	* @param  data: A pointer to the data to be written.
	* @param  dataSize: The number of data bytes to be written.
	* @param  locationOffset: The data location offset within the file.
  * @retval Returns flash drive status code.
  */
FLASH_DRIVE_STATUS FlashDrive_WriteData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset)
{
	FLASH_DRIVE_FILE file;
	FLASH_DRIVE_STATUS status;
	
	//	get the file header, which validates app support, file name, and volume index
	status = FlashDrive_GetFile(volumeIndex, filename, &file, NULL);
	if (0 != status)
		return status;

	//	validate inputs
	if ((NULL == data) || (0 == dataSize))
		return FDEC_INPUT_NOT_VALID;
	
	//	validate that write will be within file data space
	if ((dataSize > file.dataSize) || (dataLocationOffset >= file.dataSize)
		|| ((dataLocationOffset + dataSize) > file.dataSize))
		return FDEC_INPUT_NOT_VALID;

	//	write the data	
	return (FLASH_DRIVE_STATUS)Matrix.appInterface->flashWrite(volumeIndex,	file.dataLocation + dataLocationOffset,	data, dataSize);
}



#ifdef PREVIOUS_VERSION

/**
  * @brief  Writes data to a file.
	*
	*					If not enough room in file for the data,
	*					then the data does not get written, and returns -1.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: A file name.
	* @param  data: A pointer to the data to be written.
	* @param  dataSize: The number of data bytes to be written.
	* @param  locationOffset: The data location offset within the file.
  * @retval Returns 0 on success, else -1.
  */
FLASH_DRIVE_STATUS FlashDrive_WriteFileData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t locationOffset)
{
	FLASH_DRIVE_FILE header;
	uint32_t dataLocation, lastDataLocation;
	FLASH_DRIVE_STATUS status;
	
	//	verify app support
	if ((NULL == Matrix.appInterface) || (NULL == Matrix.appInterface->flashWrite))
		return -1;
	
		//	validate inputs
	if ((volumeIndex >= GetNumVolumes()) ||	(data == NULL) || (dataSize == 0))
		return -1;
	
	//	validate filename
	if (0 == FlashDrive_ValidateFileName(filename))
		return -1;
	
	//	get the file header
	status = FlashDrive_GetFile(volumeIndex, filename, &header, NULL);
	if (0 != status)
		return status;
	
	//	validate the data size and location offset
	dataLocation = header.dataLocation + locationOffset; 
	lastDataLocation = header.dataLocation + header.dataSize;
	if ((dataLocation >= lastDataLocation) ||
		((dataLocation + dataSize) > lastDataLocation))
		return -1;

	//	write the data	
	return Matrix.appInterface->flashWrite(volumeIndex,	dataLocation,	data, dataSize);
}

#endif

