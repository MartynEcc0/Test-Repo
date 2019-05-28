/**
  ******************************************************************************
  * @file    		matrix_config.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Matrix CAN bus codec configuration.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  * OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */


#ifndef __MATRIX_CONFIG_H
#define __MATRIX_CONFIG_H

#include <stdint.h>


//  Start RAM allocation section

//	if you need custom RAM allocation,
//	define this as a preprocessor symbol
#ifdef CUSTOM_MATRIX_LIB_RAM_ALLOCATION

#include "matrix_ram_allocations.h"

#else  //  using the minimum allocations below

//	Cache tokens for time logic controller token table.
#define MATRIX_TIME_LOGIC_TOKEN_TABLE_SIZE		50

//	Time logic operator and operand stack sizes.
#define MTL_OPERAND_STACK_SIZE								20
#define MTL_OPERATOR_STACK_SIZE								20

//	Cashe frames in rx message stream.
#define CAN_RX_STREAM_BUFFER_FRONT_SIZE				72

//	Cache frames for ansynchronous CAN callbacks.
#define CAN_RX_STREAM_BUFFER_BACK_SIZE				20

//	Cache frames in tx message stream.
#define CAN_TX_STREAM_BUFFER_SIZE						  40

#endif

//  End RAM allocation section





#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? a : b)
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? a : b)
#endif

#ifndef ABS
#define ABS(a) (((a) < 0) ? -(a) : a)
#endif


//	The industry-standard CAN frame maximum number of data bytes.
#define CAN_FRAME_MAX_NUM_BYTES 								8

//	The industry-standard CAN frame maximum number of devices.
#define CAN_FRAME_MAX_NUM_DEVICES								120

//	The the CAN broadcast address.
#define CAN_BROADCAST_ADDRESS										0


//	The number of repeats in token compression.
#define MATRIX_MESSAGE_MAX_TOKEN_REPEATS				32

//	The portion of the CAN identifier that indicates the message body frames and last frame.
//	Choosing these allows coexistence with HazCAN.

/* previous
#define MATRIX_MESSAGE_FRAME_TYPE_BODY					0x1C
#define MATRIX_MESSAGE_FRAME_TYPE_LAST					0x1D
#define MATRIX_MESSAGE_FRAME_TYPE_SINGLE				0x1E
*/

//	optimized
#define MATRIX_MESSAGE_FRAME_TYPE_SINGLE				0x1C
#define MATRIX_MESSAGE_FRAME_TYPE_BODY					0x1D
#define MATRIX_MESSAGE_FRAME_TYPE_LAST					0x1E

//	CAN Identifier Bit Widths
#define MATRIX_CAN_ID_FRAME_INDEX_BIT_WIDTH			5
#define MATRIX_CAN_ID_EVENT_INDEX_BIT_WIDTH			5
#define MATRIX_CAN_ID_FRAME_TYPE_BIT_WIDTH			5
#define MATRIX_CAN_ID_ADDRESS_BIT_WIDTH					7

//	CAN Identifier Shifts
#define MATRIX_CAN_ID_FRAME_INDEX_SHIFT					0
#define MATRIX_CAN_ID_DEST_ADDRESS_SHIFT				5
#define MATRIX_CAN_ID_EVENT_INDEX_SHIFT					12
#define MATRIX_CAN_ID_SOURCE_ADDRESS_SHIFT			17
#define MATRIX_CAN_ID_FRAME_TYPE_SHIFT					24

//	CAN Identifier Masks
#define MATRIX_CAN_ID_FRAME_INDEX_MASK					0x1F
#define MATRIX_CAN_ID_EVENT_INDEX_MASK					0x1F
#define MATRIX_CAN_ID_FRAME_TYPE_MASK						0x1F
#define MATRIX_CAN_ID_ADDRESS_MASK							0x7F

//	CAN Addresses
#define MATRIX_CAN_BROADCAST_ADDRESS						0
#define MATRIX_CAN_MIN_STANDARD_ADDRESS					1
#define MATRIX_CAN_MAX_STANDARD_ADDRESS				120
#define MATRIX_CAN_MIN_RESERVED_ADDRESS				121
#define MATRIX_CAN_MAX_RESERVED_ADDRESS				127


//	File crc.
#define MATRIX_MESSAGE_CRC_INIT_VALUE						0
#define MATRIX_MESSAGE_CRC_POLY_VALUE						0xA001

//	Exclusive-OR value for creating device address.
#define DEVICE_ADDRESS_XOR_VALUE								0x64

//	Maximum guid index for creating an address.
#define DEVICE_ADDRESS_MAX_GUID_INDEX						((128 / 7) + 1)

//	FTP params
#define MATRIX_MAX_FILE_NAME_LENGTH									12
#define MATRIX_MAX_FILE_SEGMENT_LENGTH						 256
#define MATRIX_MAX_FILE_SEGMENT_LENGTH_SHIFT				 8
#define MATRIX_MAX_FILE_REQUEST_RESPONSE_TIME_MS	 1000
#define MATRIX_MAX_SENDER_ADDRESS_FILTER_TIME_MS	 1000
#define MATRIX_SERVER_ACCESS_POLY						 0x5EB9417D

//	CRC Checksum size
#define MATRIX_MESSAGE_CRC_SIZE											 2

//	the shelf life of a received CAN frame
#define MATRIX_RECEIVED_FRAME_TIMEOUT_MS           750

//	the ECCONet CAN frame ID structure
#ifdef THIRTY_TWO_BIT
typedef struct
{
	//	the frame index for packet order
	uint32_t frameIndex:5;
	
	//	the destination address
	uint32_t destinationAddress:7;
	
	//	is event flag
	uint32_t isEvent:1;
	
	//	reserved
	uint32_t reserved:4;
	
	//	the sender address
	uint32_t sourceAddress:7;
	
	//	the frame type
	uint32_t frameType:5;
	
} ECCONET_CAN_FRAME_ID;
#else // THIRTY_TWO_BIT
typedef struct
{
	//	the frame index for packet order
	uint8_t frameIndex;
	
	//	the destination address
	uint8_t destinationAddress;
	
	//	is event flag
	uint8_t isEvent;
	
	//	reserved
	uint8_t reserved;
	
	//	the sender address
	uint8_t sourceAddress;
	
	//	the frame type
	uint8_t frameType;
	
} ECCONET_CAN_FRAME_ID;
#endif // THIRTY_TWO_BIT

#endif  //  __MATRIX_CONFIG_H
