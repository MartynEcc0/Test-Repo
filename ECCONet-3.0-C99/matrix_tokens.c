/**
  ******************************************************************************
  * @file    		matrix_tokens.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		ESG Matrix token key utilities.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  *	OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#include "matrix_tokens.h"

/**
  * @brief  Key prefix setter.
	* @param  prefix: The key prefix to set.
	* @param  key: The key to modify.
  * @retval The modified key.
  */
uint16_t Key_SetPrefix(uint8_t prefix, uint16_t key)
{
	return (key & ~KeyPrefix_Mask) | (prefix << 8);
}

/**
  * @brief  Key prefix getter.
	* @param  key: The key.
  * @retval The key prefix.
  */
uint8_t Key_GetPrefix(uint16_t key)
{
	return (uint8_t)(uint16_t)((key >> 8) & KeyPrefix_Mask);
}

/**
  * @brief  Returns the key without the prefix.
	* @param  key: The key.
  * @retval The key without the prefix.
  */
uint16_t Key_WithoutPrefix(uint16_t key)
{
	return key & ~(KeyPrefix_Mask << 8);
}

/**
  * @brief  Returns a value indicating whether is the key is an input status.
	* @param  key: The key.
  * @retval A value indicating whether is the key is an input status.
  */
bool Key_IsInputStatus(uint16_t key)	
{
	return (key & (KeyPrefix_Mask << 8)) == (KeyPrefix_InputStatus << 8);
}

/**
  * @brief  Returns a value indicating whether is the key is an output status.
	* @param  key: The key.
  * @retval A value indicating whether is the key is an output status.
  */
bool Key_IsOutputStatus(uint16_t key)	
{
	return (key & (KeyPrefix_Mask << 8)) == (KeyPrefix_OutputStatus << 8);
}

/**
  * @brief  Returns a value indicating whether is the key is a command.
	* @param  key: The key.
  * @retval A value indicating whether is the key is an command.
  */
bool Key_IsCommand(uint16_t key)	
{
	return (key & (KeyPrefix_Mask << 8)) == (KeyPrefix_Command << 8);
}

/**
  * @brief  Returns a value indicating whether the given key is a local variable key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsLocalVariable(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Local_Variables) &&
		(key < (Region_Base__Local_Variables + Region_Size__Local_Variables)));
}

/**
  * @brief  Returns a value indicating whether the given key is an indexed one-byte input key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsIndexedOneByteInput(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Indexed_One_Byte_Inputs) &&
	 (key < (Region_Base__Indexed_One_Byte_Inputs + Region_Size__Indexed_One_Byte_Inputs)));
}

/**
  * @brief  Returns a value indicating whether the given key is an indexed one-byte output key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsIndexedOneByteOutput(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Indexed_One_Byte_Outputs) &&
	 (key < (Region_Base__Indexed_One_Byte_Outputs + Region_Size__Indexed_One_Byte_Outputs)));
}

/**
  * @brief  Returns a value indicating whether the given key is a named one-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsNamedOneByte(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Named_One_Byte) &&
	 (key < (Region_Base__Named_One_Byte + Region_Size__Named_One_Byte)));
}

/**
  * @brief  Returns a value indicating whether the given key is a named two-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsNamedTwoByte(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Named_Two_Byte) &&
	 (key < (Region_Base__Named_Two_Byte + Region_Size__Named_Two_Byte)));
}

/**
  * @brief  Returns a value indicating whether the given key is a named four-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsNamedFourByte(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Named_Four_Byte) &&
	 (key < (Region_Base__Named_Four_Byte + Region_Size__Named_Four_Byte)));
}

/**
  * @brief  Returns a value indicating whether the given key is a zero-byte value key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsZeroByte(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Named_Zero_Byte) &&
	 (key < (Region_Base__Named_Zero_Byte + Region_Size__Named_Zero_Byte)));
}

/**
  * @brief  Returns a value indicating whether the given key is an ftp request key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsFtpRequest(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__FTP_Requests) &&
	 (key < (Region_Base__FTP_Requests + Region_Size__FTP_Requests)));
}

/**
  * @brief  Returns a value indicating whether the given key is an fpt response key.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsFtpResponse(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__FTP_Responses) &&
	 (key < (Region_Base__FTP_Responses + Region_Size__FTP_Response)));
}

/**
  * @brief  Returns a value indicating whether the given key is within
	*					the area of tokens that includes zero through four bytes.
	* @param  key: The key to check.
  * @retval True if the given key is within	the specified area.
  */
bool Key_IsZeroThroughFourByte(uint16_t key)
{
	key &= ~(KeyPrefix_Mask << 8);
	return ((key >= Region_Base__Indexed_One_Byte_Inputs) &&
		(key < (Region_Base__Named_Zero_Byte + Region_Size__Named_Zero_Byte)));
}


/**
  * @brief  Returns a value indicating a key value size.
	* @param  key: The key to check.
  * @retval The key value size.
  */
uint16_t Key_ValueSize(uint16_t key)
{
	key &= 0x1fff;
	
	//	mull key
	if (key == 0)
		return 0;

	//  local private variables
	else if (Region_Base__Indexed_One_Byte_Inputs > key)
	{
			if (Subreg_Base__Local_Two_Byte > key)
					return 1;
			else if (Subreg_Base__Local_Four_Byte > key)
					return 2;
			else if (Subreg_Base__Local_Zero_Byte > key)
					return 4;
			return 0;
	}

	//	global public variables
	else if (Region_Base__Named_Two_Byte > key)
		return 1;
	else if (Region_Base__Named_Four_Byte > key)
		return 2;
	else if (Region_Base__Named_Zero_Byte > key)
		return 4;
	else if (Region_Base__Indexed_Sequencer_Three_Byte > key)
		return 0;
	else if (Region_Base__FTP_Requests > key)
		return 3;
	return 0;
}

