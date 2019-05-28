/**
  ******************************************************************************
  * @file    		matrix_lib_interface.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		The Matrix library for communication between
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

#ifndef __MATRIX_LIB_INTERFACE_H
#define __MATRIX_LIB_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "matrix_tokens.h"
#include "matrix_patterns.h"
#include "matrix_file.h"
#include "matrix_flash_drive.h"
#include "matrix_ftp_public.h"



///////////////////////////////////////////////////////////////////////
//
//	Calls into the application that hosts the library.

/**
  * @brief  Prototype for Matrix interface CAN token callback.
  * @param  token: The received token.
  * @retval None.
  */
typedef void (*MATRIX_TOKEN_CALLBACK)(TOKEN *token);

/**
  * @brief  Prototype to send a CAN frame to the bus.
	* @param  id: The CAN frame ID field.
	* @param  data: The CAN frame DATA field.
	* @param  dataSize: The number of bytes in data.
  * @retval Returns 0 if the frame is sent, else -1 if the transmitter is busy.
	*					Note: Application must be able to return -1 if the CAN transmitter is busy.
  */
typedef int (*MATRIX_SEND_CAN_FRAME)(uint32_t id, uint8_t* data, uint8_t dataSize);

/**
  * @brief  Prototype for Matrix interface flash read.
  * @param  address: The starting location in EEPROM address space.
  * @param  buffer: A pointer to a buffer to hold the read bytes.
  * @param  bufferSize: The size of the buffer in bytes.
  * @retval Returns 0 on success, else -1.
  */
typedef int (*MATRIX_FLASH_READ)(uint16_t volume, uint32_t address, void *buffer, uint32_t bufferSize);

/**
  * @brief  Prototype for Matrix interface flash write.
  * @param  address: The starting location in flash address space.
  * @param  data: A pointer to the data to be written.
  * @param  dataSize: The number of bytes to be written.
  * @retval Returns 0 on success, else error code.
  */
typedef int (*MATRIX_FLASH_WRITE)(uint16_t volume, uint32_t address, void *data, uint32_t dataSize);

/**
  * @brief  Prototype for Matrix interface flash erase.
  * @param  address: The starting location in flash address space.
  * @param  data: A pointer to the data to be written.
  * @param  dataSize: The number of bytes to be written.
  * @retval Returns 0 on success, else error code.
  */
typedef int (*MATRIX_FLASH_ERASE)(uint16_t volume, uint32_t address, uint32_t dataSize);

/**
  * @brief  Prototype for Matrix file name to volume index.
	* @param  filename: The file name.
  * @retval Returns the volume index.
  */
typedef uint8_t (*MATRIX_FILE_NAME_TO_VOLUME_INDEX)(char *filename);

/**
  * @brief  Prototype to get the device's 128-bit GUID.
	* @param  A pointer to an array of 4 32-bit values to receive the GUID.
  * @retval None.
  */
typedef void (*MATRIX_GET_GUID)(uint32_t guid[4]);

/**
  * @brief  Prototype for the FTP server to route a file read request through the application.
	*
	*					The application can handle the request by setting the file info's data location and size,
	*					and then returning a 0.  The FTP server will calculate the data checksum.
	*
	*					The application can reject the request by returning a -1, in which case the server
	*					will use the flash file system.
	*
	* @param  requesterAddress: The CAN address of the device that made the request.
	* @param  fileInfo: Pointer to the server's file info object.
  * @retval Return 0 to accept the request, else -1.
  */
typedef int (*MATRIX_FTP_SERVER_FILE_READ_HANDLER)(uint16_t requesterAddress, MATRIX_FILE_METADATA *fileInfo);


///////////////////////////////////////////////////////////////////////
//
//	Interface table and drive volume structures.

//	The maximum number of drive volumes supported.
#define MATRIX_FLASH_DRIVE_MAX_NUM_VOLUMES  3

/**
  * @brief  A flash drive volume structure.
	*					Note that volume zero MUST be accessable via pointer, which would
	*					normally be the same flash memory as the code execution space.
  */
typedef struct
{
	//	the base address of the volume
	uint32_t baseAddress;

	//	the size of the volume
	uint32_t size;

} MATRIX_DRIVE_VOLUME;

/**
  * @brief  Prototype for Matrix interface.
  */
typedef struct
{
	//	the token callback
	MATRIX_TOKEN_CALLBACK tokenCallback;
	
	//	the method to send a CAN frame to the bus
	MATRIX_SEND_CAN_FRAME sendCanFrame;
	
	//	the method to read from a storage device at a given location
	MATRIX_FLASH_READ flashRead;
	
	//	the method to write to a storage device at a given location
	MATRIX_FLASH_WRITE flashWrite;

	//	the method to erase memory on a storage device at a given location
	MATRIX_FLASH_ERASE flashErase;
	
  //	the method for the FTP server to route a file read request through the application
	MATRIX_FTP_SERVER_FILE_READ_HANDLER ftpServerReadHandler;

	//	the method to convert a file name into a storage device index
	MATRIX_FILE_NAME_TO_VOLUME_INDEX fileNameToVolumeIndex;
	
	//	the method to get the 128-bit device guid
	MATRIX_GET_GUID get128BitGuid;
		
	//	The available flash storage devices.  A device with zero size is considered not present.
	//
	//	Note that volume 0 (zero) MUST be accessable via pointer, which would
	//	normally be the same flash memory as the code execution space.
	//
	MATRIX_DRIVE_VOLUME flashVolumes[MATRIX_FLASH_DRIVE_MAX_NUM_VOLUMES];
	

} MATRIX_INTERFACE_TABLE;



///////////////////////////////////////////////////////////////////////
//
//	Files used by the Matrix library.

/**
  * @brief  The Matrix CAN address data file object.
	*					This object may be stored as a file in flash volume 0.
	*/
typedef struct __attribute__((__packed__))
{
	//	the device CAN address
	uint8_t address;

	//	is a static address
	//	functions as bool but cast as uint8_t to keep file format consistent
	uint8_t isStatic;
	
} MATRIX_CAN_ADDRESS_FILE_OBJECT;


/**
  * @brief  The Matrix product information data file object.
	*					This object may be stored as a file in flash volume 0.
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
	
} MATRIX_PRODUCT_INFO_FILE_OBJECT;


/**
  * @brief  The equation file data header when initial constants are present.
  */
typedef struct
{
	//	the file security key
	uint32_t SecurityKey;
	
	//	the file constants key
	uint16_t ConstantsKey;
	
	//	the file constants size
	uint16_t ConstantsSize;
	
} EQUATION_FILE_DATA_HEADER_WITH_CONSTANTS;


//	The CAN address file name.
#define MATRIX_CAN_ADDRESS_FILE_NAME	"address.can"

//	The CAN address file is always stored in volume 0.
#define MATRIX_CAN_ADDRESS_FILE_VOLUME_INDEX	0


//	The product info file name.
#define MATRIX_PRODUCT_INFO_FILE_NAME	"product.inf"

//	The product info file is always stored in volume 0.
#define MATRIX_PRODUCT_FILE_VOLUME_INDEX	0


//	The product assembly file name.
#define MATRIX_PRODUCT_ASSEMBLY_FILE_NAME	"assembly.epa"

//	The product assembly file is always stored in volume 0.
#define MATRIX_PRODUCT_ASSEMBLY_FILE_VOLUME_INDEX	0



//	The behavioral equations file names.
#define MATRIX_TIME_LOGIC_FILE_KEY				0x1C3D5C47
#define MATRIX_TIME_LOGIC_FILE_NAME_0			"equation.btc"
#define MATRIX_TIME_LOGIC_FILE_NAME_1			"eq_user1.btc"
#define MATRIX_TIME_LOGIC_FILE_NAME_2			"eq_user2.btc"
#define MATRIX_TIME_LOGIC_FILE_NAME_3			"eq_user3.btc"
#define MATRIX_TIME_LOGIC_FILE_NAME_4			"eq_user4.btc"
#define MATRIX_TIME_LOGIC_FILE_NAME_5			"eq_user5.btc"
#define MATRIX_TIME_LOGIC_FILE_NAME_6			"eq_user6.btc"

//	The Time-Logic behavioral file is always stored in volume 0.
#define MATRIX_TIME_LOGIC_FILE_VOLUME_INDEX		0

//	The expression patterns file name.
#define MATRIX_TOKEN_PATTERN_FILE_KEY			0x4865433B
#define MATRIX_TOKEN_PATTERN_FILE_NAME		"patterns.tbl"

//	The LED Matrix messages file name.
#define MATRIX_MESSAGE_DISPLAY_FILE_KEY		0x083FB876
#define MATRIX_MESSAGE_DISPLAY_FILE_NAME	"messages.tbl"
#define MATRIX_MESSAGE_DISPLAY_ENTRY_KEY	0x9D86			

//	The dictionary file name.
#define MATRIX_STEP_DICTIONARY_FILE_KEY		0x38B1E2BA
#define MATRIX_STEP_DICTIONARY_FILE_NAME	"lighteng.dct"

//	The token sequencer files are always stored in volume 0.
#define MATRIX_TOKEN_PATTERN_VOLUME_INDEX			0



///////////////////////////////////////////////////////////////////////
//
//	Fixed network addresses.
//
// CAN bus addresses 0-127
#define MATRIX_CAN_BROADCAST_ADDRESS               0    // broadcast address
#define MATRIX_CAN_MIN_STANDARD_ADDRESS            1    // self-assigned range is 1-120
#define MATRIX_CAN_MAX_STANDARD_ADDRESS            120  //  
#define MATRIX_CAN_MIN_RESERVED_ADDRESS            121  // reserved range is 121-127
#define MATRIX_CAN_MAX_RESERVED_ADDRESS            127  //  
#define MATRIX_VEHICLE_BUS_ADDRESS                 121  // reserved: vehicle bus gateway 
#define MATRIX_PC_ADDRESS                          126  // reserved: PC USB-CAN

// device internal address 128-255
#define MATRIX_EQUATION_PROCESSOR_NETWORK_ADDRESS  132  // reserved: equation processor
#define MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS	 133  // reserved: sequencers 1-6 (0-5)
#define MATRIX_TOKEN_SEQUENCER_1_NETWORK_ADDRESS	 134
#define MATRIX_TOKEN_SEQUENCER_2_NETWORK_ADDRESS	 135
#define MATRIX_TOKEN_SEQUENCER_3_NETWORK_ADDRESS	 136
#define MATRIX_TOKEN_SEQUENCER_4_NETWORK_ADDRESS	 137
#define MATRIX_TOKEN_SEQUENCER_5_NETWORK_ADDRESS	 138

//	macro to determine if token is from CAN bus
#define Address_IsCanBus(address) (128 > (address))

//	macro to determine if token is from internal module
#define Address_IsInternal(address) (128 <= (address))

//	macro to determine if token is from equation processor
#define Address_IsEquationProcessor(address) (128 <= (address))

//	macro to determine if token is from sequencer module
#define Address_IsSequencer(address) \
     ((MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS <= (address)) \
   && (MATRIX_TOKEN_SEQUENCER_5_NETWORK_ADDRESS >= (address)))


///////////////////////////////////////////////////////////////////////
//
//	Public library methods.

/**
  * @brief  Resets and configures the Matrix library.
	* @param  appInterface: The application interface table.
	* @param  systemTime: The 32-bit system time in milliseconds.
  * @retval None.
  */
extern void Matrix_Reset(const MATRIX_INTERFACE_TABLE *appInterface, uint32_t systemTime);

/**
  * @brief  Clocks the Matrix library.
	*					This method supports cooperative task scheduling (round-robin).
	* @param  systemTime: The 32-bit system time in milliseconds.
  * @retval None.
  */
extern void Matrix_Clock(uint32_t systemTime);

/**
  * @brief  Handles incoming tokens from the application.
	* @param  token: A pointer to a token.
  * @retval None.
  */
extern void Matrix_TokenIn(TOKEN *token);

/**
  * @brief  Sends a sync token over the CAN bus.
	* @param  token: The token to send.
  * @retval Returns zero on success, else -1.
  */
extern int Matrix_SendSync(TOKEN *token);

/**
  * @brief  Receives a CAN frame from the bus.
	* @param  id: The CAN frame ID field.
	* @param  data: The CAN frame DATA field.
	* @param  dataSize: The number of bytes in data.
    * @param  systemTime: the current epoch time in milliseconds
  * @retval None.
  */
extern void Matrix_ReceiveCanFrame(uint32_t id, uint8_t* data, uint8_t dataSize, uint32_t systemTime);

/**
  * @brief  Returns the system event index managed by the library.
	* @param  None.
  * @retval The system event index.
  */
extern uint8_t Matrix_GetEventIndex(void);

/**
  * @brief  Returns the device CAN address managed by the library.
	* @param  None.
  * @retval The device CAN address.
  */
extern uint8_t Matrix_GetCanAddress(void);

/**
  * @brief  Returns a value indicating whether the CAN address managed by the library is valid.
	*					An address is valid if it static or is in the range 1 to 120.
	* @param  None.
  * @retval True if the address is static.
  */
extern bool Matrix_IsCanAddressValid(void);

/**
  * @brief  Returns a value indicating whether the CAN address managed by the library is static.
	* @param  None.
  * @retval True if the address is static.
  */
extern bool Matrix_IsCanAddressStatic(void);

/**
  * @brief  Returns a value indicating whether a library token sequencer is running.
  * @param  sequencerIndex: The token sequencer index 0~2.
  * @retval True if the sequencer is running.
  */
extern bool Matrix_IsTokenSequencerRunning(uint16_t sequencerIndex);

/**
  * @brief  Returns the number of available patterns.
  * @param  None.
  * @retval The number of available patterns, or zero on error.
  */
extern uint16_t Matrix_GetTokenSequencerNumPatterns(void);

/**
  * @brief  Gets a pointer to the current equation file.
	* @param  None.
  * @retval A pointer to the current equation file, or null if none found.
  */
extern const uint8_t *Matrix_GetCurrentEquationFile(void);




#endif  //  __MATRIX_LIB_INTERFACE_H
