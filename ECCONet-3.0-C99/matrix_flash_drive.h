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



#ifndef __FLASH_DRIVE_H
#define __FLASH_DRIVE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_file.h"


//	if you need to define erased flash bytes as 0x00
//	define this as a preprocessor symbol
#ifdef ERASED_FLASH_BYTES_ARE_ZERO

//	flash byte erase value
#define FLASH_DRIVE_FILE_ERASE_VALUE	0x00

//	flash header keys
#define FLASH_DRIVE_FILE_KEY_UNUSED   0x0000
#define FLASH_DRIVE_FILE_KEY_ACTIVE   0x3FAC 	// 16300
#define FLASH_DRIVE_FILE_KEY_DELETED  0xFFFF

#else  //  erased flash bytes are 0xff

//	flash byte erase value
#define FLASH_DRIVE_FILE_ERASE_VALUE	0xFF

//	flash header keys
#define FLASH_DRIVE_FILE_KEY_UNUSED   0xFFFF
#define FLASH_DRIVE_FILE_KEY_ACTIVE   0x3FAC 	// 16300
#define FLASH_DRIVE_FILE_KEY_DELETED  0x0000

#endif


/**
  * @brief  Flash drive status codes.
  */
typedef enum
{
	FDEC_OK														=  0,
	FDEC_UNKNOWN_ERROR								= -1,
	FDEC_FILE_NOT_FOUND								= -2,
	FDEC_NOT_ENOUGH_ROOM_IN_VOLUME		= -3,
	FDEC_VOLUME_CORRUPTED							= -4,
	FDEC_FILE_HEADER_CORRUPTED				= -5,
	FDEC_FILE_DATA_CORRUPTED					= -6,
	FDEC_INPUT_NOT_VALID							= -7,
	FDEC_NO_APP_SUPPORT								= -8,
	FDEC_INVALID_VOLUME_INDEX					= -9,
	FDEC_INVALID_FILE_NAME						= -10,
	FDEC_FLASH_READ_ERROR							= -11,
	FDEC_FLASH_WRITE_ERROR						= -12,
	FDEC_FLASH_ERASE_ERROR						= -13,
	FDEC_READ_BUFFER_TOO_SMALL				= -14,

} FLASH_DRIVE_STATUS;



/**
  * @brief  Flash drive volume statistics.
  */
typedef struct
{
	//	The amount of unused space, including deleted files.
	uint32_t AvailableSpace;

	//	The amount of space that is erased and not written.
	uint32_t WriteableSpace;

	//	The next address to write a file header.
	uint32_t NextHeaderAddress;
	
	//	The lowest address of written file data.
	uint32_t LowestDataAddress;
	
	//	Indicates whether any bad header checksums are detected.
	bool IsCorrupted;

} FLASH_DRIVE_VOLUME_STATS;

/**
  * @brief  The file header structure.
	*					The reserved variables pad the header size to 32 bytes in order to
	*					prevent flash write disturbance (a mechanism intrinsic to flash memories).
  */
typedef struct __attribute__((__packed__))
{
	//	the header key
	uint16_t key;

	//	the header checksum, excluding the key and header checksum
	uint16_t checksum;
	
	//	the 8.3 file name
	char name[MATRIX_FILE_NAME_LENGTH];
	
	//	the file data location
	uint32_t dataLocation;

	//	the file timestamp in seconds since 00:00:00 Jan 1, 2017 
	uint32_t timestamp;

	//	the file data size
	uint32_t dataSize;

	//	the data checksum
	uint16_t dataChecksum;

	//	the data location offset for extended features
	uint16_t dataLocationOffset;
	
} FLASH_DRIVE_FILE;


//---------------------------------------------------------------------------//
//	                    READ, WRITE AND ERASE FILES                          //
//---------------------------------------------------------------------------//

/**
  * @brief  Reads a file from the given flash drive volume.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  buffer: A data buffer to hold the data to read.
	* @param  bufferSize: The data buffer size in bytes.
	* @param  out_timestamp: Pointer to a 32-bit value to hold the file timestamp.
  * @retval Returns 0 on success, else -1.
  */
extern FLASH_DRIVE_STATUS FlashDrive_ReadFile(uint16_t volumeIndex, char *filename,
	void *buffer, uint32_t bufferSize, uint32_t *out_timestamp);

/**
  * @brief  Writes a file to the given flash drive volume.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: A file name.
	* @param  data: A pointer to the data to be written.
	* @param  dataSize: The number of data bytes to be written.
	* @param  timestamp: The file timestamp.
  * @retval Returns 0 on success, else -1.
  */
extern FLASH_DRIVE_STATUS FlashDrive_WriteFile(uint16_t volumeIndex, char *filename,
	void *data, uint32_t dataSize, uint32_t timestamp);

/**
  * @brief  Tags a file for deletion.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name to erase.
  * @retval Returns 0 if file found and erased, else -1.
  */
extern FLASH_DRIVE_STATUS FlashDrive_EraseFile(uint16_t volumeIndex, char *filename);



//---------------------------------------------------------------------------//
//	                     READ AND WRITE FILE DATA                            //
//---------------------------------------------------------------------------//

/**
  * @brief  Reads data from a flash file.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  data: A pointer to a provided data buffer.
	* @param  dataSize: The number of data bytes to be read.
	* @param  dataLocationOffset: The data location offset within the file.
	* @param  wrap: True if read wraps around at end of file.
  * @retval Returns 0 on success, else flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_ReadFileData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset, bool wrap);

/**
  * @brief  Writes data to a file.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The name of file for which to write data.
	* @param  data: A pointer to the data to be written.
	* @param  dataSize: The number of data bytes to be written.
	* @param  locationOffset: The data location offset within the file.
  * @retval Returns flash drive status code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_WriteData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset);

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
extern FLASH_DRIVE_STATUS FlashDrive_WriteFileData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset, bool wrap);

/**
  * @brief  Inserts data into a flash file, pushing data above it upward. 
	*					Stores the given data location offset in the file header.
	*					Does not grow the file.
	*
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  data: A pointer to the data to be inserted.
	* @param  dataSize: The number of data bytes to be inserted.
	* @param  dataLocationOffset: The data location offset within the file.
  * @retval Returns 0 on success, else flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_InsertFileData(uint16_t volumeIndex, char *filename,
	uint8_t *data, uint32_t dataSize, uint32_t dataLocationOffset);

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
extern FLASH_DRIVE_STATUS FlashDrive_RemoveFileData(uint16_t volumeIndex, char *filename,
	uint32_t dataSize, uint32_t dataLocationOffset);

/**
  * @brief  Moves data within a flash file.
	* @param  dest: The destination address.
	* @param  source: The source address.
	* @param  size: The size of data to move.
  * @retval Returns 0 on success, else flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_MoveFileData(uint16_t volumeIndex, uint8_t *dest, uint8_t *source, int32_t size);



//---------------------------------------------------------------------------//
//	                    READ AND WRITE FILE HEADER                           //
//---------------------------------------------------------------------------//

/**
  * @brief  Gets a file's header and header location in flash.
	*					Note: Does not validate the file data checksum.
	*
	* @param  volumeIndex: Index of flash drive volume to access.
	* @param  filename: The file name.
	* @param  out_Header: Pointer to a variable to hold the header.
	*											If null, no header is returned.
	* @param  out_Location: Pointer to an variable to hold the header address in flash.
	*											  If null, no address is returned.
  * @retval Returns 0 if file is found, else flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_GetFile(uint16_t volumeIndex, char* filename,
	FLASH_DRIVE_FILE *out_Header, uint32_t *out_Location);

/**
  * @brief  Gets an indexed file's header and header location in flash.
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
extern FLASH_DRIVE_STATUS FlashDrive_GetIndexedFile(uint16_t volumeIndex, uint32_t fileIndex,
	FLASH_DRIVE_FILE *out_Header, uint32_t *out_Location);

/**
  * @brief  Gets a file's metadata.
	* @param  file: A file metadata structure, with the volume and file name already set.
  * @retval Returns 0 if file is found, else flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_GetFileMetadata(MATRIX_FILE_METADATA *file);

/**
  * @brief  Gets an indexed file's metadata.
	* @param  fileIndex: The file header index.
	* @param  file: A file metadata structure, with the volume already set.
  * @retval Returns 0 if file is found, else flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_GetIndexedFileMetadata(uint32_t fileIndex, MATRIX_FILE_METADATA *file);

/**
  * @brief  Reads a file header from flash.
	* @param  volumeIndex: Index of flash drive volume to access.
	* @param  address: Address of header in flash.
	* @param  outHeader: Pointer to a header.
  * @retval None.
  */
extern FLASH_DRIVE_STATUS FlashDrive_ReadFileHeader(uint16_t volumeIndex, uint32_t address,
	FLASH_DRIVE_FILE *out_Header);

/**
  * @brief  Writes a file header to the given flash drive volume,
	*					and allocates the file data area.
	*
	*					If not enough room in volume for file header and data,
	*					then the header does not get written.
	*
	* @param  file: A file metadata object.
  */
extern FLASH_DRIVE_STATUS FlashDrive_WriteFileHeader(MATRIX_FILE_METADATA *file);



//---------------------------------------------------------------------------//
//	                             RESIZE FILE                                 //
//---------------------------------------------------------------------------//

/**
  * @brief  Changes a file data size in the volume.
	* @param  volumeIndex: The index of flash drive volume to access.
	* @param  filename: A file name.
	* @param  newDataSize: The new file data size.
  * @retval Returns 0 on success, else flash drive error code.
	*					
  */
extern FLASH_DRIVE_STATUS FlashDrive_ChangeFileSize(uint16_t volumeIndex, char *filename,
	uint32_t newDataSize);



//---------------------------------------------------------------------------//
//	                      		VOLUME METHODS                                 //
//---------------------------------------------------------------------------//

/**
  * @brief  Gets the number of available flash drive volumes.
	* @param  None.
  * @retval The number of available flash drive volumes.
  */
extern uint16_t GetNumVolumes(void);

/**
  * @brief  Gets a volume's statistics.
	* @param  volumeIndex: Index of flash drive volume to access.
	* @param  volumeStats: A pointer to an object that receives the volume statistics.
	* @param  ignoredFilename: If not null, the available space calculation includes the space
	*						used for all files with a matching name, including those marked for deletion.
  * @retval None.
  */
extern FLASH_DRIVE_STATUS FlashDrive_GetVolumeStatistics(uint16_t volumeIndex,
	FLASH_DRIVE_VOLUME_STATS *volumeStats, char *ignoredFilename);

/**
  * @brief  Tries to make space in a volume.
	* @param  volumeIndex: Index of flash drive volume.
	* @param  size: The required writable space.
  * @retval Returns 0 on success, else the flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_TryMakeSpace(uint16_t volumeIndex, uint32_t size);

/**
  * @brief  Compacts the files in a flash drive volume.
	* @param  volumeIndex: Index of flash drive volume to access.
  * @retval Returns 0 on success, else the flash drive error code.
  */
extern FLASH_DRIVE_STATUS FlashDrive_CompactVolume(uint16_t volumeIndex);


//---------------------------------------------------------------------------//
//	                        	DATA INTEGRITY                                 //
//---------------------------------------------------------------------------//

/**
  * @brief  Checks a file's integrity.
	* @param  header: The address of the header.
	* @param  out_Checksum: The header checksum.
  * @retval Returns true if the header and data checksum are valid, else false.
  */
extern bool FlashDrive_CheckFileIntegrity(FLASH_DRIVE_FILE *header, uint16_t *out_Checksum);

/**
  * @brief  Computes a file header CRC checksum.
	* @param  header: The address of the header.
  * @retval Returns the computed checksum.
  */
extern uint16_t FlashDrive_ComputeHeaderCRC16(FLASH_DRIVE_FILE *header);

/**
  * @brief  Computes a file data CRC checksum.
	* @param  data: A pointer to the data.
	* @param  size: The data size.
  * @retval Returns the computed checksum.
  */
extern uint16_t FlashDrive_ComputeDataCRC16(void *data, uint16_t size);

/**
  * @brief  Verifies a file name for length and '.' separator.
	* @param  filename: The file name.
  * @retval If the filename is good, then returns the length of the filename, else 0.
  */
extern uint16_t FlashDrive_ValidateFileName(char *filename);


#endif  //  __FLASH_DRIVE_H

