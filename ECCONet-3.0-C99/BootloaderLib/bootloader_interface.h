/**
  ******************************************************************************
  * @file    		matrix_lib_interface.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		The bootloader library for communication between
	*							the CAN bus and the internal message system.
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

#ifndef __BOOTLOADER_INTERFACE_H
#define __BOOTLOADER_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "ecconet.h"
#include "matrix_tokens.h"


/**
  * @brief  A footer to be embedded at the end of the application FLASH image.
	*         This 48-byte structure is required for the ECCONet bootloader.
  */
#define ECCONET_FLASH_FILE_FOOTER_KEY 0xC0DEBA5E
typedef struct
{
	//	MUST be the identifier key 0xC0DEBA5E
	uint32_t codebaseKey;

	//	MUST match the product info file model name
  char modelName[31];

	//	reserved
	uint8_t  reserved_0;
	uint32_t reserved_1;
	
	//	the app address and application CRC32
	uint32_t appAddress;
	uint32_t appCRC32;
	
} ECCONET_FLASH_FILE_FOOTER;



/**
  * @brief  The bootloader CAN address data file object.
  *
  *         IMPORTANT:  You can leave the pointer to this struct as null
	*                     to just use automatic self-addressing.
	*
  *                     When providing a pointer, both fields 'address' and
  *                   	'isStatic' must be zero for automatic self-addressing.
  */
typedef struct __attribute__((__packed__))
{
	//	the device CAN address
	uint8_t address;

	//	is a static address
	//	functions as bool but cast as uint8_t to keep file format consistent
	uint8_t isStatic;
	
} BOOTLOADER_CAN_ADDRESS_STRUCT;


/**
  * @brief  The bootloader product information data file object.
  *
  *         IMPORTANT: The app firmware revision and lighthead enums should be left zero.
  */
typedef struct __attribute__((__packed__))
{
  //	the product model name
  char modelName[31];

  //	the product manufacturer name
  char manufacturerName[31];

  //	the hardware revision string
  char hardwareRevision[6];

  //	the app firmware revision string
  char appFirmwareRevision[6];

  //	the bootloader firmware revision
  char bootloaderFirmwareRevision[6];

  //	the first indexed output enumeration
  char baseLightheadEnumeration[6];

  //	the maximum indexed output enumeration
  char maxLightheadEnumeration[6];
	
} BOOTLOADER_PRODUCT_INFO_STRUCT;



///////////////////////////////////////////////////////////////////////
//
//	Calls into the application that hosts the bootloader library.

/**
  * @brief  Prototype to send a CAN frame to the bus.
	* @param  frame: A pointer to the frame to send.
  * @retval Returns 0 if the frame is sent, else -1 if the transmitter is busy.
  *
  *					Note: Application MUST block until CAN frame is sent.
  */
typedef void (*BOOTLOADER_SEND_CAN_FRAME)(ENET_CAN_FRAME *frame);

/**
  * @brief  Prototype for bootloader interface flash write.
  * @param  address: The starting location in flash address space.
  * @param  data: A pointer to the data to be written.
  * @param  dataSize: The number of bytes to be written.
  * @retval Returns true on success.
  *
  *					Note: Application MUST block until FLASH is written.
  */
typedef bool (*BOOTLOADER_FLASH_WRITE)(uint32_t address, void *data, uint32_t dataSize);

/**
  * @brief  Prototype for bootloader to reboot.
  * @param  None.
  * @retval An int, but not used.
  */
typedef int (*BOOTLOADER_REBOOT)(void);

/**
  * @brief  Prototype to get the device's 128-bit GUID.
	* @param  A pointer to an array of 4 32-bit values to receive the GUID.
  * @retval None.
  */
typedef void (*BOOTLOADER_GET_GUID)(uint32_t guid[4]);




/**
  * @brief  Prototype for bootloader interface.
  */
typedef struct
{
	//	a pointer to the CAN address struct
	//	you can leave this pointer null for automatic self-addresing
	const BOOTLOADER_CAN_ADDRESS_STRUCT *canAddressStruct;

	//	a pointer to the device info struct
	//	you must provide this in order for the device to be fully identified
	const BOOTLOADER_PRODUCT_INFO_STRUCT *productInfoStruct;

	//	the application flash starting address
	const uint32_t appFlashAddress;
	
	//	the application flash size
	const uint32_t appFlashSize;
	
	
	//	the method to send a CAN frame to the bus
	BOOTLOADER_SEND_CAN_FRAME sendCanFrame;
	
	//	the method to write to processor FLASH memory
	BOOTLOADER_FLASH_WRITE flashWrite;

	//	the method to erase the processor FLASH memory
	BOOTLOADER_REBOOT reboot;

	//	the method to get the 128-bit device guid
	BOOTLOADER_GET_GUID get128BitGuid;
	

} BOOTLOADER_INTERFACE_TABLE;



///////////////////////////////////////////////////////////////////////
//
//	Public library methods.

/**
  * @brief  Resets and configures the bootloader library.
	* @param  appInterface: The application interface table.
	* @param  systemTime: The 32-bit system time in milliseconds.
  * @retval None.
  */
extern void Bootloader_Reset(const BOOTLOADER_INTERFACE_TABLE *appInterface, uint32_t systemTime);

/**
  * @brief  Clocks the bootloader library.
	*					This method supports cooperative task scheduling (round-robin).
	* @param  systemTime: The 32-bit system time in milliseconds.
  * @retval None.
  */
extern void Bootloader_Clock(uint32_t systemTime);

/**
  * @brief  Receives a CAN frame from the bus.
	* @param  frame: A pointer to the CAN frame to receive.
  * @retval None.
  */
extern void Bootloader_ReceiveCanFrame(ENET_CAN_FRAME *frame);

/**
  * @brief  Returns the device CAN address managed by the library.
	* @param  None.
  * @retval The device CAN address.
  */
extern uint8_t Bootloader_GetCanAddress(void);

/**
  * @brief  Returns a value indicating whether the CAN address managed by the library is valid.
	*					An address is valid if it static or is in the range 1 to 120.
	* @param  None.
  * @retval True if the address is static.
  */
extern bool Bootloader_IsCanAddressValid(void);

/**
  * @brief  Calculates CRC32/BZIP2 with poly=0x04C11DB7, no reflection,
  *					start=~0, output inverted (to match ELFfile.crc32)
	* @param  data: The data to perform the CRC on.
	* @param  length: The data length in bytes.
  * @retval Returns the CRC32 result.
  */
extern uint32_t Bootloader_ComputeCRC32(uint8_t *data, uint32_t length);

#endif  //  __BOOTLOADER_INTERFACE_H
