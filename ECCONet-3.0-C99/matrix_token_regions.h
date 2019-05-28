/**
  ******************************************************************************
  * @file    		matrix_token_regions.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		ESG Matrix token region definitions.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  *	OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#ifndef __MATRIX_TOKEN_REGIONS_H
#define __MATRIX_TOKEN_REGIONS_H

#include <stdint.h>
#include <stdbool.h>


//	This region is for private local variables and the keys may
//	be used for any purpose.
//
//	Local variables are signed 32-bit values.
//
#define Region_Base__Local_Variables									    					    1
#define Region_Size__Local_Variables										  						199
#define Value_Bytes__Local_Variables													  		    4

//	The defines are for reference only. They are used for compressing
//	local variables in an AnyToken bin pattern file, and decompressed
//	values are 32 bits.
#define Subreg_Base__Local_One_Byte													  		      1
#define Subreg_Size__Local_One_Byte													  		    119
#define Value_Bytes__Local_One_Byte													  		      1
#define Subreg_Base__Local_Two_Byte													  		    120
#define Subreg_Size__Local_Two_Byte													  		     50
#define Value_Bytes__Local_Two_Byte													  		      2
#define Subreg_Base__Local_Four_Byte													  		  170
#define Subreg_Size__Local_Four_Byte													  		   20
#define Value_Bytes__Local_Four_Byte													  		    4
#define Subreg_Base__Local_Zero_Byte													  		  190
#define Subreg_Size__Local_Zero_Byte													  		   10
#define Value_Bytes__Local_Zero_Byte													  		    0


//	This region is for public indexed arrays of non-descript 8-bit inputs.
//	An example would be a graphics-based input device with arrays of user
//	controls.
//
//	Device firmware maps the keys to specific inputs.
//	Although 8 bits are specified, the input is typically boolean.
//
#define Region_Base__Indexed_One_Byte_Inputs												  200
#define Region_Size__Indexed_One_Byte_Inputs													300
#define Value_Bytes__Indexed_One_Byte_Input													    1


//	This region is for public indexed arrays of non-descript 8-bit outputs.
//	An example would be an array of light heads used for patterns.
//
//	Device firmware maps the keys to specific outputs.
//
//	These outputs must be responsive to commands tokens.  Smart light heads
//	that have a component failure warning should issue output status tokens
//	only when a failure occurs, and no more often than once per second.
//
#define Region_Base__Indexed_One_Byte_Outputs			 						  		  500
#define Region_Size__Indexed_One_Byte_Outputs			 						  		  500
#define Value_Bytes__Indexed_One_Byte_Output									  		    1



//	This region is for public 8-bit named items.  The naming
//	comes from international standards committees or ESG internal.
//
//	For lights and speakers, the associated value is the relative
//	light intensity or acoustic intensity 0~100.
//
//	Organizing into subregions below facilitates tests for certain
//	categories, and provides a clearer picture to third-party OEMs
//	who may use such keys for direct asset control.
//
#define Region_Base__Named_One_Byte																	 1000
#define Region_Size__Named_One_Byte				 													 4000
#define Value_Bytes__Named_One_Byte		  														    1
	#define Subreg_Base__Named_Lights																	 1000
	#define Subreg_Size__Named_Lights																		400
	#define Subreg_Base__Named_Sounds																	 1400
	#define Subreg_Size__Named_Sounds																		400
	#define Subreg_Base__Named_Messages																 1800
	#define Subreg_Size__Named_Messages																  200
	#define Subreg_Base__Named_OBD2_One_Byte													 2000
	#define Subreg_Size__Named_OBD2_One_Byte													 1000
	#define Region_Base__Named_Misc_One_Byte													 3000
	#define Region_Size__Named_Misc_One_Byte													 2000


//	This region is for public 16-bit named items.  The naming
//	comes from international standards committees or ESG internal.
//
#define Region_Base__Named_Two_Byte				 													 5000
#define Region_Size__Named_Two_Byte				 													 2000
#define Value_Bytes__Named_Two_Byte		  														    2


//	This region is for public signed 32-bit named items.  The naming
//	comes from international standards committees or ESG internal.
//
#define Region_Base__Named_Four_Byte			 													 7000
#define Region_Size__Named_Four_Byte			 													 1000
#define Value_Bytes__Named_Four_Byte		 														    4
	#define Subreg_Base__Named_Four_Byte_C3Net												 7950
	#define Subreg_Size__Named_Four_Byte_C3Net													 50


//	This region is for public named zero-byte items.  The naming
//	comes from international standards committees or ESG internal.
//
#define Region_Base__Named_Zero_Byte			 													 8000
#define Region_Size__Named_Zero_Byte			 													  150
#define Value_Bytes__Named_Zero_Byte		 														    0


//	This region is for public indexed sequencers.
//	(intensity << 16) | pattern_enumeration
//
#define Region_Base__Indexed_Sequencer_Three_Byte					 		  		 8150
#define Region_Size__Indexed_Sequencer_Three_Byte							  		   10
#define Value_Bytes__Indexed_Sequencer_Three_Byte							  		    1


//	This region is for public named FTP requests.
//
#define Region_Base__FTP_Requests																		 8160
#define Region_Size__FTP_Requests																		   10

//	This region is for public named FTP responses.
//
#define Region_Base__FTP_Responses																	 8170
#define Region_Size__FTP_Response																		   22




/**
  * @brief  Returns a value indicating whether the given key is a local variable key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsLocalVariable(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is an indexed one-byte input key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsIndexedOneByteInput(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is an indexed one-byte output key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsIndexedOneByteOutput(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is a named one-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsNamedOneByte(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is a named two-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsNamedTwoByte(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is a named four-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsNamedFourByte(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is a named zero-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsNamedZeroByte(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is an ftp request key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsFtpRequest(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is an fpt response key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsFtpResponse(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is within
	*					the area of tokens that includes zero through four bytes.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsZeroThroughFourByte(uint16_t key);

/**
  * @brief  Returns a value indicating whether the given key is within
	*					the area of tokens that includes named light and sound patterns.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
extern bool Key_IsNamedPattern(uint16_t key);


#endif  //  __MATRIX_TOKEN_REGIONS_H


