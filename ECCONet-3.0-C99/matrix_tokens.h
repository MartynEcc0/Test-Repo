/**
  ******************************************************************************
  * @file    		matrix_tokens.h
  * @copyright  � 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		ESG Matrix token key definitions and utility prototypes.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  *	OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#ifndef __MATRIX_TOKENS_H
#define __MATRIX_TOKENS_H

#include <stdint.h>
#include <stdbool.h>
#include "matrix_token_regions.h"


/**
  * @brief  The token structure encapsulates local and global variables for transport.
	*					Notes:
	*					1. The order of the fields in the structure is fixed for little-endian sort and search algorithms. 
	*					2. The key and value bytes are ordered big-ending in CAN message streams and token pattern tables. 
	*					3. The value size is 32 bits in order to hold values of up to 4 bytes. In CAN message streams and
	*						 pattern tables, the actual number of value bytes in the stream depends on the token, as listed
	*						 in "matrix_token_regions.h".
  */
typedef struct
{
	//	this field is for the time-logic equation processor
	//	added here because there was a spare byte in token
	uint8_t flags;

	//	For incoming tokens, the CAN address of the sender.
	//	For outgoing tokens, the CAN address of the recipient.
	uint8_t address;

	//	the private or ESG public key
	uint16_t key;

	//	The value associated with the key.
	int32_t value;
	
} TOKEN;
#define TOKEN_KEY_SIZE			2

/**
  * @brief  The token flags.  Note that the first five flags are used by the time-logic processor.
  */
typedef enum
{
	TF_DEFAULT_STATE = 0x80,
	
} TOKEN_FLAGS;


/**
  * @brief  The system state enumeration.
  */
typedef enum
{
	SYSTEM_STATE_OFF,
	SYSTEM_STATE_ON,
	SYSTEM_STATE_STANDBY,
	
} SYSTEM_STATE;


/**
  * @brief  Named token keys.  Note that Boolean is one byte.
  */
typedef enum
{
	//	reserved null key
	KeyNull,


	//	named lights																																	//	Value format
	//																																								//	===================================================
	KeyLight_Stop = Subreg_Base__Named_Lights,																				//	Boolean, 1=on
	KeyLight_Tail,																																		//	Boolean, 1=on
	KeyLight_LeftTurn,																																//	Boolean, 1=on
	KeyLight_RightTurn,																																//	Boolean, 1=on
	KeyLight_Takedown,																																//	Boolean, 1=on
	KeyLight_Worklight,																																//	Boolean, 1=on
	KeyLight_AlleyLeft,																																//	Boolean, 1=on
	KeyLight_AlleyRight,																															//	Boolean, 1=on
	KeyLight_WorklightLeft,																														//	Boolean, 1=on
	KeyLight_WorklightRight,																													//	Boolean, 1=on
	
	

	//	named one-byte																																//	Value format
	//																																								//	===================================================
	KeyRequestAddress = Region_Base__Named_Misc_One_Byte,															//	8-bit requesting address 1~120 during bus enum
	KeyResponseAddressInUse,																													//	8-bit address 1~120 in use during bus enum
	KeySystemPowerState,																															//	8-bit enum { Stby=0, Active=1 }
	KeyTokenSequencerIntensity,																												//	8-bit 0~100 (%) using local sequencer address
	KeyNextExpressionFront,																														//	Boolean, for field programming, rising edge = next available
	KeyNextExpressionRear,																														//	Boolean, for field programming, rising edge = next available
	KeyOutputAuxiliary,																																//	Boolean, 1=on
	KeyModeCruise,																																		//	Boolean, 1=on
	KeyModeNight,																																			//	Boolean, 1=on
	KeyExpressionPresetLightBarFront,																									//	8-bit bit flags, preset P1=0x01, P2=0x02, P3=0x04, etc.
	KeyExpressionPresetLightBarRear,																									//	8-bit bit flags, preset P1=0x01, P2=0x02, P3=0x04, etc.
	KeyExpressionPresetDirectorFront,																									//	8-bit bit flags, preset P1=0x01, P2=0x02, P3=0x04, etc.
	KeyExpressionPresetDirectorRear,																									//	8-bit bit flags, preset P1=0x01, P2=0x02, P3=0x04, etc.
	KeyIndexedPatternPresetSound,																											//	8-bit user preset selection
	KeyRotatingBeaconControl,																													//	8-bit enum for rotating beacon control
	KeyNodeType,																																			//	8-bit node type, 0 = unknown
	KeyAlertLevel,																																		//	8-bit bit flags L1 = 0x01, L2 = 0x02, L3 = 0x04
	KeyVehicleAlarm,																																	//	Boolean, special functions that can start while vehicle is off
	KeyUserProfile,																																		//	8-bit user profile enum
	KeyLeftCut,																																				//	Boolean, 1=active
	KeyRightCut,																																			//	Boolean, 1=active
	KeyFrontCut,																																			//	Boolean, 1=active
	KeyRearCut,																																				//	Boolean, 1=active
  KeyDirectorNumLights,                                                             //	8-bit, indicates number of director LEDs used
  KeyNextDirectorNumLights,                                                         //	Boolean, for field programming, rising edge = next available
  KeyDirectorLocation,                                                              //	8-bit enum, 0=rear, 1=front, 2=both
	KeyNextDirectorLocation,                                                          //	Boolean, for field programming, rising edge = next available
  KeyNextPrimaryExpression,                                                         //	Boolean, for field programming, rising edge = next available
	

	//	named OBD-II one-byte (extra)																									//	Value format
	//																																								//	===================================================
	KeyVehicleHorn = (Subreg_Base__Named_OBD2_One_Byte - 2), 													//	Boolean, 1=pressed
	KeyVehicleHornOEM = (Subreg_Base__Named_OBD2_One_Byte - 1), 													//	Boolean, 1=pressed

	//	named OBD-II one-byte																													//	Value format
	//																																								//	===================================================
	KeyACClutchButton = Subreg_Base__Named_OBD2_One_Byte, 														//	Boolean, 1=on
	KeyDriverSideFrontDoorOpen,																												//	Boolean, 1=open
	KeyDriverSideRearDoorOpen,																												//	Boolean, 1=open
	KeyPassengerSideFrontDoorOpen,																										//	Boolean, 1=open
	KeyPassSideRearDoorOpen,																													//	Boolean, 1=open
	KeyRearHatchOpen,																																	//	Boolean, 1=open
	KeyRearWindowPosition,																														//	Boolean, 1=open
	KeyDoorsLocked,																																		//	Boolean, 1=locked (all doors)
	KeyHeadlightLowBeam,  																														//	Boolean, 1=beam on
	KeyHeadlightHighBeam,  																														//	Boolean, 1=beam on
	KeyLeftTurnSignal,																																//	Boolean, 1=light on (as opposed to feature)
	KeyRightTurnSignal,																																//	Boolean, 1=light on (as opposed to feature)
	KeyHazards,																																				//	Boolean, 1=lights on (as opposed to feature)
	KeyMarkerLights,																																	//	Boolean, 1=on
	KeyParkBrake,																																			//	Boolean, 1=brake applied
	KeyServiceBrake,																																	//	Boolean, 1=brake pedal engaged
	KeyDriverSeatbeltFastened,																												//	Boolean, 1=fastened
	KeyPassengerSeatbeltFastened,																											//	Boolean, 1=fastened
	KeyRearSeatbeltsFastened,																													//	Boolean, 1=fastened
	KeyKeyPosition,																																		//	8-bit flags { Off=0,Acc=1,Run=2,Start=4 }
	KeyTransmissionPosition, 																													//	8-bit enum { P=0,N=1,D=2,R=3 }
	KeyThrottlePosition, 																															// 	8-bit 0~100, 0=least fuel metering
	KeyVehicleSpeed,																																	//	8-bit 0-255 km/h
	KeyFuelLevel,																																			//	8-bit 0~100, 0=empty
	KeyBatteryVoltage,																																//	8-bit decivolt
	KeyVehicleAcceleration,																														//	8-bit signed +/- 2G

	//	named two-byte																																//	Value format
	//																																								//	===================================================
	KeyTokenSequencerPattern = Region_Base__Named_Two_Byte,														//	16-bit pattern enum using local sequencer address (stop=0)
	KeyTokenSequencerSync,																														//	16-bit sync pattern enum
	KeyResponseAppFirmwareCrc,																												//	16-bit ECCONet 3.0 CRC
	KeyResponseBootloaderFirmwareCrc,																									//	16-bit ECCONet 3.0 CRC
	KeyEngineRPM,																																			//	16-bit OBD-II integer RPM
	KeySafetyDirFrontPattern,																													//	16-bit pattern enum (stop=0) (for front SD)
	KeySafetyDirRearPattern,																													//	16-bit pattern enum (stop=0) (for rear SD)
	KeyStepMethodDictionaryKey,																												//	16-bit light engine dictionary key
	KeyDeviceFault,																																		//	(4-bit flags<<12) | (12-bit faultCode) (faultCode 0=none)
    KeyJboxState,                                                                     //	16-bit JBox port bit state
    KeyReportedLightbarEnumSequencer0,                                                  //  pattern enumeration currently running on lightbar sequencer 0
    KeyReportedLightbarEnumSequencer1,                                                  //  pattern enumeration currently running on lightbar sequencer 1
    KeyReportedLightbarEnumSequencer2,                                                  //  pattern enumeration currently running on lightbar sequencer 2
    KeyReportedLightbarEnumSequencer3,                                                  //  pattern enumeration currently running on lightbar sequencer 3
    KeyReportedLightbarEnumSequencer4,                                                  //  pattern enumeration currently running on lightbar sequencer 4
    KeyReportedLightbarEnumSequencer5,                                                     //  pattern enumeration currently running on lightbar sequencer 5


	//	name four-byte
	//		
	KeyIndexedTokenSequencerWithPattern = Region_Base__Named_Four_Byte,								//	(exprEnum<<16)|(intensity<<8)|sequencerIndex (exprEnum 0=stop)
	KeyRequestSystemReboot,																														//	32-bit, must be 0x4C7E146F
	KeyRequestInvokeBootloader,																												//	32-bit, must be 0x5633870B
	KeyRequestEraseAppFirmware,																												//	32-bit, must be 0x6A783B52
	KeyRequestEraseAllFirmware,																												//	32-bit, must be 0xB8E0123C
	KeyRequestAllowStatus,																														//	32-bit, must be 0x320CCAEC
	KeyRequestSuppressStatus,																													//	32-bit, must be 0xA7CA28EE
	KeyTokenSequencerSyncRange,																												//	2 x 16-bit, must use local sequencer address	
	KeySoundEnumWithIndexedAmp,																												//	(soundEnum<<16)|(intensity<<8)|ampIndex (soundEnum 0=stop)
	KeyLedMatrixMessage,																															//	32-bit, LED Matrix mode, see "led_matrix_controller.h"
	KeyLedMatrixMessageFront,																													//	32-bit, LED Matrix mode, see "led_matrix_controller.h"
	KeyLedMatrixMessageRear,																													//	32-bit, LED Matrix mode, see "led_matrix_controller.h"
	
    //	C3Net																			//	Value format
    //																					//	===================================================
    KeyRequestC3NetNodeLightEngineTestFlashOn = Subreg_Base__Named_Four_Byte_C3Net,     //	29-bit, the C3Net node location ID
    KeyRequestC3NetNodeLightEngineTestFlashOff,                                         //	29-bit, the C3Net node location ID
    KeyRequestC3NetNodeInvokeBootloaderApp,                                             //	29-bit, the C3Net node location ID
    KeyRequestC3NetNodeInvokeBootloaderPattern,                                         //	29-bit, the C3Net node location ID
    KeyResponseC3NetNodeBootloaderComplete,                                             //	29-bit, the C3Net node location ID
    KeyResponseC3NetNodeBootloaderError,                                                //	29-bit, the C3Net node location ID
    KeyRequestC3NetNodeAppVersion,                                                      //	29-bit, the C3Net node location ID
    KeyResponseC3NetNodeAppVersion,                                                     //  Firmware Version [MMM][mm] M = major version, m = minor version
    KeyRequestC3NetNodeBootVersion,
    KeyResponseC3NetNodeBootVersion,
    KeyRequestC3NetNodeUpdateTempCalValue,
    KeyResponseC3NetNodeUpdateTempCalValue,
    KeyRequestC3NetNodeTemperature,
    KeyResponseC3NetNodeTemperature,
	KeyResponseC3NetNodesAssignLocationIds,
	KeyResponseC3NetNodesClearLocationIds,
	KeyResponseC3NetPowerCycle,
    KeyResponseC3NetNodeGenerateProductEPA,												//	0: success, non-zero: error

    //	named zero-byte, no token value													//	Description
    //																					//	===================================================
    KeyRequestAppFirmwareCrc = Region_Base__Named_Zero_Byte,                            //  Request the application firmware CRC
    KeyRequestBootloaderFirmwareCrc,                                                    //  Request the bootloader firmware CRC
    KeyRequestC3NetNodesAssignLocationIds,                                              //	Request LBC assign loc. IDs from inventory file to all LEs
	KeyRequestC3NetNodesClearLocationIds,                                               //	Request LBC clear location IDs
	KeyRequestC3NetNodeGenerateProductEPA,												//	Request LBC to generate a new product EPA
	KeyRequestC3NetPowerCycle,
	
	//	indexed sequencer pattern and intensity																				//	Value format
	//																																								//	===================================================
	KeyIndexedSequencer = Region_Base__Indexed_Sequencer_Three_Byte,									//	(intensity << 16) | pattern_enumeration

	//	ftp requests
	KeyRequestFileIndexedInfo = Region_Base__FTP_Requests,
	KeyRequestFileInfo,
	KeyRequestFileReadStart,
	KeyRequestFileReadSegment,
	KeyRequestFileWriteStart,
	KeyRequestFileWriteSegment,
	KeyRequestFileDelete,
	KeyRequestFileTransferComplete,
	KeyRequestFileWriteFixedSegment,

	//	ftp responses
	KeyResponseFileIndexedInfo = Region_Base__FTP_Responses,
	KeyResponseFileInfo,
  KeyResponseFileInfoComplete,
	KeyResponseFileReadStart,
	KeyResponseFileReadSegment,
  KeyResponseFileReadComplete,
	KeyResponseFileWriteStart,
	KeyResponseFileWriteSegment,
  KeyResponseFileWriteComplete,
	KeyResponseFileDelete,
  KeyResponseFileDeleteComplete,
	KeyResponseFileNotFound,
	KeyResponseFileChecksumError,
	KeyResponseFtpDiskFull,
	KeyResponseFtpClientError,
	KeyResponseFtpServerBusy,
	KeyResponseFtpServerError,
  KeyResponseFtpTransactionComplete,
	KeyResponseFtpTransactionTimedOut,
	KeyResponseFileWriteFixedSegment,

	
} TokenKeys;

/**
 * @brief faultCode values for KeyDeviceFault token.
 */
typedef enum{
    no_fault=0,
    hard_fault,
    watchdog,
    power_loss,
    under_voltage,
    over_voltage,
    blown_fuse,
    output_short,
    system_short,
    aux_a_short,
    aux_b_short,
    aux_c_short,
    aux_d_short,
    temperature_shutoff,
    com_failure, //SPI,USB,etc, possibly CAN depending on the message.
}fault_types;

/**
  * @brief  Named product categories.
  */
typedef enum
{
	//	product categories
	ProductCategoryNull,
	ProductCategoryLightBar,
	ProductCategorySafetyDirector,
	ProductCategoryKeypad,
	ProductCategoryJunctionBox,
	
} ProductCategories;

/**
  * @brief  Key prefixes.
  */
typedef enum
{
	//	token key prefixes
	KeyPrefix_Command																		= 0x00,
	KeyPrefix_OutputStatus															= 0x20,
	KeyPrefix_InputStatus																= 0x40,
	KeyPrefix_BinaryRepeat															= 0x60,
	KeyPrefix_AnalogRepeat															= 0x80,
	KeyPrefix_PatternSync																= 0xA0,
	KeyPrefix_Mask																			= 0xE0,

} KeyPrefix;

/**
  * @brief  Token values for certain requests.
  */
#define TOKEN_VALUE_SYSTEM_REBOOT							0x4C7E146F
#define TOKEN_VALUE_INVOKE_BOOTLOADER					0x5633870B
#define TOKEN_VALUE_ERASE_APP_FIRMWARE				0x6A783B52
#define TOKEN_VALUE_ERASE_ALL_FIRMWARE				0xB8E0123C


/**
  * @brief  Key prefix setter.
	* @param  prefix: The key prefix to set.
	* @param  key: The key to modify.
  * @retval The modified key.
  */
extern uint16_t Key_SetPrefix(uint8_t prefix, uint16_t key);	

/**
  * @brief  Key prefix getter.
	* @param  key: The key.
  * @retval The key prefix.
  */
extern uint8_t Key_GetPrefix(uint16_t key);	

/**
  * @brief  Returns the key without the prefix.
	* @param  key: The key.
  * @retval The key without the prefix.
  */
extern uint16_t Key_WithoutPrefix(uint16_t key);	

/**
  * @brief  Returns a value indicating whether is the key is an input status.
	* @param  key: The key.
  * @retval A value indicating whether is the key is an input status.
  */
extern bool Key_IsInputStatus(uint16_t key);

/**
  * @brief  Returns a value indicating whether is the key is an output status.
	* @param  key: The key.
  * @retval A value indicating whether is the key is an output status.
  */
extern bool Key_IsOutputStatus(uint16_t key);

/**
  * @brief  Returns a value indicating whether is the key is a command.
	* @param  key: The key.
  * @retval A value indicating whether is the key is an command.
  */
extern bool Key_IsCommand(uint16_t key)	;

/**
  * @brief  Returns a value indicating a key value size.
	* @param  key: The key to check.
  * @retval The key value size.
  */
extern uint16_t Key_ValueSize(uint16_t key);

/**
  * @brief  Returns a value indicating whether the address is in the range of 128-255.
	* @param  address: The address.
  * @retval A value indicating whether the address is in the range of 128-255.
  */
extern bool Address_IsInternal(uint8_t address);

/**
  * @brief  Returns a value indicating whether the address is in the range of 0-127.
	* @param  address: The address.
  * @retval A value indicating whether the address is in the range of 0-127.
  */
extern bool Address_IsExternal(uint8_t address);



#endif  //  __MATRIX_TOKENS_H








