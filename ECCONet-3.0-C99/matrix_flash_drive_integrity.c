/**
  ******************************************************************************
  * @file    		flash_drive_integrity.c
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		File system using FLASH storage and 8.3 file names.
	*
	*							Methods for data integrity checking.
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
  * @brief  Checks a file's integrity.
	* @param  header: The address of the header.
	* @param  out_Checksum: The header checksum.
  * @retval Returns true if the header and data checksum are valid, else false.
  */
bool FlashDrive_CheckFileIntegrity(FLASH_DRIVE_FILE *header, uint16_t *out_Checksum)
{
	//	validate header checksum
	if (header->checksum != FlashDrive_ComputeHeaderCRC16(header))
			return false;

	//	validate data checksum
	*out_Checksum = FlashDrive_ComputeDataCRC16((uint8_t *)header->dataLocation, header->dataSize);
	return (header->dataChecksum == *out_Checksum);
}

/**
  * @brief  Computes a file header CRC checksum.
	* @param  header: The address of the header.
  * @retval Returns the checksum.
  */
uint16_t FlashDrive_ComputeHeaderCRC16(FLASH_DRIVE_FILE *header)
{
	return FlashDrive_ComputeDataCRC16(&header->name,
		sizeof(FLASH_DRIVE_FILE) - sizeof(((FLASH_DRIVE_FILE*)0)->key) -
		sizeof(((FLASH_DRIVE_FILE*)0)->checksum));
}

/**
  * @brief  Computes a flash drive file CRC.
	* @param  data: A pointer to the data.
	* @param  size: The data size.
  * @retval Returns the computed CRC.
  */
uint16_t FlashDrive_ComputeDataCRC16(void *data, uint16_t size)
{
	uint8_t b, *pData;
	uint16_t bit, crc;
	
	//	validate input
	if ((data == NULL) || (size == 0))
		return 0;
	pData = (uint8_t *)data;
	
	crc = 0;
	while (size--)
	{
		//	crc calc
		b = *pData++;
		bit = 8;
		while (bit--)
		{
			crc = ((b ^ (uint8_t)crc) & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
			b >>= 1;
		}
	}
	return crc;
}

/**
  * @brief  Verifies a file name for length and '.' separator.
	* @param  filename: The file name.
  * @retval If the filename is valid, then returns the length of the filename, else 0.
  */
uint16_t FlashDrive_ValidateFileName(char *filename)
{
	uint16_t i, n;
	
	//	check for null
	if (NULL == filename)
		return 0;

	//	check syntax
	n = 0;
	i = 0;
	while (*filename)
	{
		if (++i > MATRIX_FILE_NAME_LENGTH)
			return 0;
		if (*filename == '.')
			n = i;
		++filename;
	}
	if ((2 <= n) && (1 <= (i - n)) && (3 >= (i - n)))
		return i;
	return 0;
}
