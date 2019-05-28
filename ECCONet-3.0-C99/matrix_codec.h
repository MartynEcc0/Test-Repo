/**
  ******************************************************************************
  * @file    		matrix_code.h
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

#ifndef __MATRIX_CODEC_H
#define __MATRIX_CODEC_H


#include <stdint.h>
#include "matrix_time_logic.h"
#include "matrix_tokens.h"


/**
  * @brief  The compressor byte synk prototype.
  */
typedef void (*MATRIX_CODEC_BYTE_SINK)(uint8_t byte);

/**
  * @brief  The decompressor token synk prototype.
  * @param  token: A token from the compressed byte stream.
  * @retval None.
  */
typedef void (*MATRIX_CODEC_TOKEN_SINK)(TOKEN *token);


/**
  * @brief  Converts one or more tokens into a compressed byte stream.
	*					Note: Sorts the tokens in place, so token(s) must not be const array. 
	*
  * @param  token: A pointer to an array of one or more tokens.
  * @param  numToken: The number of tokens to process.
  * @param  byteSink: A callback method to receive the bytes.
  * @retval Returns 0 on success, else -1.
  */
extern int MatrixCodec_Compress(MTL_TOKEN *token, uint16_t numTokens, MATRIX_CODEC_BYTE_SINK byteSink);

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
extern int MatrixCodec_Decompress(uint8_t **byte, uint16_t numBytes, uint8_t address,
	MATRIX_CODEC_TOKEN_SINK tokenSink);



#endif  //  __MATRIX_CODEC_H
