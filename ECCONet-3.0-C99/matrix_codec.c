/**
  ******************************************************************************
  * @file    		matrix_transmitter.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		Converts tokens to compressed byte streams and vice versa.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  * OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include "matrix_config.h"
#include "matrix_codec.h"


//	private methods
static void OutputTokenKey(uint16_t key, MATRIX_CODEC_BYTE_SINK byteSink);
static void OutputTokenValue(uint32_t value, uint16_t valueSize, MATRIX_CODEC_BYTE_SINK byteSink);
static void OutputToken(MTL_TOKEN *token, MATRIX_CODEC_BYTE_SINK byteSink);



/**
  * @brief  Converts the time logic processor tokens into a byte stream.
	*					Note: Sorts the tokens in place, so token(s) must not be const array. 
	*
  * @param  token: A pointer to an array of one or more tokens.
  * @param  numToken: The number of tokens to process.
  * @param  byteSink: A callback method to receive the bytes.
  * @retval Returns 0 on success, else -1.
  */
int MatrixCodec_Compress(MTL_TOKEN *token, uint16_t numTokens, MATRIX_CODEC_BYTE_SINK byteSink)
{
	//MTL_TOKEN *sortToken, *compareToken, *lastToken, tempToken;
	MTL_TOKEN *compareToken, *lastToken;
	uint8_t byte;
	uint32_t key, valueSize, bit;
	uint32_t numBinaryRepeats, numAnalogRepeats;
	int32_t value;
	
	//	validate inputs
	if ((NULL == token) || (0 == numTokens) || (NULL == byteSink))
		return -1;

	//	pointer to last token + 1
	lastToken = token + numTokens;
	
#if 0  // ML already sorted
	//	sort the tokens by key
	sortToken = token;
	while (++sortToken < lastToken)
	{
		compareToken = sortToken;
		while (--compareToken >= token)
			if (compareToken->key <= sortToken->key)
				break;
		if (++compareToken < sortToken)
		{
			tempToken = *sortToken;
			memmove(compareToken + 1, compareToken,
				(uintptr_t)sortToken - (uintptr_t)compareToken);
			*compareToken = tempToken;
		}		
	}
#endif
	
	//	for all tokens
	while (token < lastToken)
	{
		//	if token not flagged for broadcast, continue to next one
		if (!(token->token.flags & MtlFlagsShouldBroadcast))
		{
			++token;
			continue;
		}
		
		//	if no value associated with token,
		//	then just send it without compression
		valueSize = Key_ValueSize(token->token.key);
		if (0 == valueSize)
		{
			OutputToken(token, byteSink);
			++token;
			continue;
		}
		
		//	check for compressible series starting with current token
		numAnalogRepeats = 0;
		numBinaryRepeats = 0;
		value = token->token.value;
		key = token->token.key + 1;
		compareToken = token;
		while (numAnalogRepeats < (MATRIX_MESSAGE_MAX_TOKEN_REPEATS - 1))
		{
			//	get next status token
			do
				++compareToken;
			while ((compareToken < lastToken) && !(compareToken->token.flags & MtlFlagsShouldBroadcast));
			
			//	if not a next token in the key series, then done
			if ((compareToken >= lastToken) || (compareToken->token.key != key) ||
				(Key_ValueSize(compareToken->token.key) != valueSize))
				break;
			
			//	get first non-zero value for binary repeat
			if ((0 == value) && (0 != compareToken->token.value))
				value = compareToken->token.value;
			
			//	check for non-zero value match for binary repeat
			if ((0 == compareToken->token.value) || (compareToken->token.value == value))
				++numBinaryRepeats;
			else
				numBinaryRepeats = MATRIX_MESSAGE_MAX_TOKEN_REPEATS;
			
			//	bump params
			++key;
			++numAnalogRepeats;
		}
		
		//	if a binary series found
		if (numBinaryRepeats && (numBinaryRepeats < MATRIX_MESSAGE_MAX_TOKEN_REPEATS))
		{
			//	send repeat, first key in sequence and the common non-zero value
			byteSink(numBinaryRepeats | KeyPrefix_BinaryRepeat);
			OutputTokenKey(token->token.key, byteSink);
			OutputTokenValue(value, valueSize, byteSink); 
		
			//	for tokens in series
			bit = 0;
			byte = 0;
			++numBinaryRepeats;
			while (numBinaryRepeats--)
			{
				//	set non-zero value flag and bump token
				if (0 != token->token.value)
					byte |= (1 << bit);
				if (++bit >= 8)
				{
					byteSink(byte);
					bit = 0;
					byte = 0;
				}
				//	get next status token
				do
					++token;
				while ((token < lastToken) && !(token->token.flags & MtlFlagsShouldBroadcast));
			}
			
			//	if partially full byte, then send it
			if (bit)
				byteSink(byte);
		}
		
		//	else if an analog series found
		else if (numAnalogRepeats)
		{
			//	send repeat and base token
			byteSink(numAnalogRepeats | KeyPrefix_AnalogRepeat);
			OutputToken(token, byteSink);
			do
				++token;
			while ((token < lastToken) && !(token->token.flags & MtlFlagsShouldBroadcast));
			
			//	for all others in series, send token value
			while (numAnalogRepeats--)
			{
				//	set value
				OutputTokenValue(token->token.value, valueSize, byteSink);

				//	get next status token
				do
					++token;
				while ((token < lastToken) && !(token->token.flags & MtlFlagsShouldBroadcast));
			}
		}
		
		//	else no compressible series starting with current token
		else
		{
			//	send token and bump token index
			OutputToken(token, byteSink);
			++token;
		}
	}
	return 0;
}


//	macro for to check bytes during decompression for buffer overrun
#define CheckStreamPointer() { if (*byte >= lastByte)	return -1; }

/**
  * @brief  Decompresses a byte stream into one or more tokens.
  *         On completion the stream pointer points to the byte
  *         following the compressed data.
  *
  * @param  byte: A pointer to a byte stream pointer.
  * @param  numBytes: The maximum number of bytes to process.
	*	@param	address: The CAN address of the byte stream source.
  * @param  tokenSink: A callback method to receive the tokens.
  * @retval Returns zero on success, else -1 on error.
  */
int MatrixCodec_Decompress(uint8_t **byte, uint16_t numBytes, uint8_t address,
	MATRIX_CODEC_TOKEN_SINK tokenSink)
{
	TOKEN token;
	uint8_t *lastByte;
	uint16_t bitFlags, bitIndex, n;
	uint16_t tokenType, key, valueSize;
	uint32_t value;
	uint16_t numRepeats = 1; 
	
	//	validate inputs
	if ((NULL == byte) || (0 == numBytes))
		return -1;

	//	for all bytes
	lastByte = *byte + numBytes;
	while (*byte < lastByte)
	{
		//	if token is not standard or repeat, then done
		tokenType = **byte & KeyPrefix_Mask;
		if (KeyPrefix_AnalogRepeat < tokenType) 
			return 0;
		
		//	if token type is a binary or analog repeat, then get number of repeats
		if ((KeyPrefix_BinaryRepeat == tokenType) || (KeyPrefix_AnalogRepeat == tokenType))
		{
			numRepeats = (**byte & (MATRIX_MESSAGE_MAX_TOKEN_REPEATS - 1)) + 1;
			++*byte;
		}
		
		//	token key and value size
		CheckStreamPointer();
		key = **byte;
		++*byte;
		CheckStreamPointer();
		key = (key << 8) | **byte;
		++*byte;
		valueSize = Key_ValueSize(key);
		
		//	if an analog repeat
		if (tokenType == KeyPrefix_AnalogRepeat)
		{
			//	for all tokens in series
			while(numRepeats--)
			{
				//	output token
				token.value = 0;
				for (n = 0; n < valueSize; ++n)
				{
					CheckStreamPointer();
					token.value <<= 8;
					token.value |= **byte;
					++*byte;
				}
				if (NULL != tokenSink)
				{
					token.key = key++;
					token.address = address;
					tokenSink(&token);
				}
			}
		}
		
		//	else if a binary repeat
		else if (tokenType == KeyPrefix_BinaryRepeat)
		{
			//	get common non-zero value
			value = 0;
			for (n = 0; n < valueSize; ++n)
			{
				CheckStreamPointer();
				value <<= 8;
				value |= **byte;
				++*byte;
			}

			//	for all tokens in series
			bitIndex = 8;
			bitFlags = 0;
			while(numRepeats--)
			{
				bitFlags >>= 1;
				if (++bitIndex >= 8)
				{
					CheckStreamPointer();
					bitIndex = 0;
					bitFlags = **byte;
					++*byte;
				}
				if (NULL != tokenSink)
				{
					token.key = key++;
					token.value = (bitFlags & 1) ? value : 0; 
					token.address = address;
					tokenSink(&token);
				}
			}			
		}
		
		//	else a single token
		else
		{
			token.value = 0;
			for (n = 0; n < valueSize; ++n)
			{
				CheckStreamPointer();
				token.value <<= 8;
				token.value |= **byte;
				++*byte;
			}
			if (NULL != tokenSink)
			{
				token.key = key;
				token.address = address;
				tokenSink(&token);
			}
		}
	}
	return 0;
}


//	private methods...........................................................

/**
  * @brief  Outputs a token key.
	* @param  key: A token key to output.
  * @param  byteSink: A callback method to receive the bytes.
  * @retval None.
  */
static void OutputTokenKey(uint16_t key, MATRIX_CODEC_BYTE_SINK byteSink)
{
	byteSink(key >> 8);
	byteSink(key);
}

/**
  * @brief  Outputs a token value.
	* @param  value: A token value to output.
	* @param  valueSize: The value size in bytes.
  * @param  byteSink: A callback method to receive the bytes.
  * @retval None.
  */
static void OutputTokenValue(uint32_t value, uint16_t valueSize, MATRIX_CODEC_BYTE_SINK byteSink)
{
	while (valueSize--)
		byteSink(value >> (8 * valueSize));
}

/**
  * @brief  Adds a token to the transmit fifo.
	* @param  tokens: A pointer to a token to add.
  * @param  byteSink: A callback method to receive the bytes.
  * @retval None.
  */
static void OutputToken(MTL_TOKEN *token, MATRIX_CODEC_BYTE_SINK byteSink)
{
	uint16_t valueSize;

	byteSink(token->token.key >> 8);
	byteSink(token->token.key);
	valueSize = Key_ValueSize(token->token.key);
	while (valueSize--)
		byteSink(token->token.value >> (8 * valueSize));
}

