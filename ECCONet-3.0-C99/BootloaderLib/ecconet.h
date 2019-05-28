/**
  ******************************************************************************
  * @file    		ecconet.h
  * @copyright  © 2018 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		Feb 2018
  * @brief   		ECCONet 3.0 defines.
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

#ifndef __ECCONET_H
#define __ECCONET_H


#include <stdint.h>
#include <stdbool.h>


//	flash write message
//	all multi-byte values in big-endian format
//
//	item    bytes     index  function
//  ======================================
//  1       1         0      event index					
//  2       2         1      token
//  3       4         3      access code
//  4       31        7      model name
//  5       4         38     data location
//  6       2         42     data size
//  7       256 max   44     data
//  8       2                message checksum
//	total max bytes = 302 bytes

typedef enum
{
	BSC_OK,
	BSC_INVALID_ACCESS_CODE,
	BSC_INVALID_MODEL_NAME,
	BSC_INVALID_FLASH_AREA,
	BSC_FLASH_WRITE_ERROR,
	
} BOOTLOADER_STATUS_CODES;




#define ENET_MAX_SENDER_ADDRESS_FILTER_TIME_MS	 1000


//	The CAN identifier HCTP codes
#define ENET_MESSAGE_FRAME_TYPE_BODY					   0x1C
#define ENET_MESSAGE_FRAME_TYPE_LAST					   0x1D
#define ENET_MESSAGE_FRAME_TYPE_SINGLE				   0x1E

//	CAN Identifier Bit Widths
#define ENET_CAN_ID_FRAME_INDEX_BIT_WIDTH			      5
#define ENET_CAN_ID_EVENT_INDEX_BIT_WIDTH			      5
#define ENET_CAN_ID_FRAME_TYPE_BIT_WIDTH			      5
#define ENET_CAN_ID_ADDRESS_BIT_WIDTH					      7

//	CAN Identifier Shifts
#define ENET_CAN_ID_FRAME_INDEX_SHIFT					      0
#define ENET_CAN_ID_DEST_ADDRESS_SHIFT				      5
#define ENET_CAN_ID_EVENT_INDEX_SHIFT					     12
#define ENET_CAN_ID_SOURCE_ADDRESS_SHIFT			     17
#define ENET_CAN_ID_FRAME_TYPE_SHIFT					     24

//	CAN Identifier Masks
#define ENET_CAN_ID_FRAME_INDEX_MASK					   0x1F
#define ENET_CAN_ID_EVENT_INDEX_MASK					   0x1F
#define ENET_CAN_ID_FRAME_TYPE_MASK						   0x1F
#define ENET_CAN_ID_ADDRESS_MASK							   0x7F

//	CAN Addresses
#define ENET_CAN_BROADCAST_ADDRESS						      0
#define ENET_CAN_MIN_STANDARD_ADDRESS					      1
#define ENET_CAN_MAX_STANDARD_ADDRESS				      120
#define ENET_CAN_MIN_RESERVED_ADDRESS				      121
#define ENET_CAN_MAX_RESERVED_ADDRESS				      127


///////////////////////////////////////////////////////////////////////
//
//	Fixed network addresses.
//
// CAN bus addresses 0-127
#define ECCONET_CAN_BROADCAST_ADDRESS               0    // broadcast address
#define ECCONET_CAN_MIN_STANDARD_ADDRESS            1    // self-assigned range is 1-120
#define ECCONET_CAN_MAX_STANDARD_ADDRESS            120  //  
#define ECCONET_CAN_MIN_RESERVED_ADDRESS            121  // reserved range is 121-127
#define ECCONET_CAN_MAX_RESERVED_ADDRESS            127  //  
#define ECCONET_VEHICLE_BUS_ADDRESS                 121  // reserved: vehicle bus gateway 
#define ECCONET_PC_ADDRESS                          126  // reserved: PC USB-CAN


//	macro to determine if token is from CAN bus
#define Address_IsCanBus(address) (128 > (address))



//	some handy macros
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? a : b)
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? a : b)
#endif

#ifndef ABS
#define ABS(a) (((a) < 0) ? -(a) : a)
#endif

#pragma anon_unions

//	the ECCONet CAN frame ID structure
typedef struct
{
//	//	the frame index for packet order
//	uint32_t frameIndex:5;
//	
//	//	the destination address
//	uint32_t destinationAddress:7;
//	
//	//	reserved
//	uint32_t reserved:5;
//	
//	//	the sender address
//	uint32_t sourceAddress:7;
//	
//	//	the frame type
//	uint32_t frameType:5;
    
    	//	the frame index for packet order
	unsigned frameIndex:5;
	
	//	the destination address
	unsigned destinationAddress:7;
	
	//	reserved
	unsigned reserved:5;
	
	//	the sender address
	unsigned sourceAddress:7;
	
	//	the frame type
	unsigned frameType:5;
	
} ENET_CAN_FRAME_ID;


//	the ECCONet simple CAN frame structure
typedef struct
{
	union
	{
		//	the ID
		uint32_t id;
		
		//	the ID bits
		ENET_CAN_FRAME_ID idBits;
	};
	
	//	the data
	uint8_t data[8] __attribute__((aligned(4)));
	
	//	the data size
	uint8_t dataSize;
	
} ENET_CAN_FRAME;




//	the product info file name
#define ENET_PRODUCT_INFO_FILE_NAME	"product.inf"

//	the product info file name length, not including the zero termination
#define ENET_PRODUCT_INFO_FILE_NAME_SIZE  11



#endif  //  __ECCONET_H

