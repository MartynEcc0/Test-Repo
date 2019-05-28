/**
  ******************************************************************************
  * @file       matrix_time_logic.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author     M. Latham, Liquid Logic
  * @version    1.0.0
  * @date       April 2017
	*
  * @brief      The time-logic processer takes tokens as inputs, processes them
	*							via human-readable math and logic equations, and then generates
	*							value-based and time-based output tokens based on text readable
	*							output options.
	*
  ******************************************************************************
  * @attention
  * Unless required by applicable law or agreed to in writing, software
  * created by Liquid Logic LLC is delivered "as is" without warranties
  * or conditions of any kind, either express or implied.
  *
  ******************************************************************************
  */

#ifndef __MATRIX_TIME_LOGIC_H
#define __MATRIX_TIME_LOGIC_H


#include <stdint.h>
#include <stdbool.h>
#include "matrix_config.h"
#include "matrix_tokens.h"
#include "matrix_file.h"

//	parser no more characters code
//	either by a null string char or end of file
#define MTL_NO_MORE_CHARS					0
//#define MTL_EQUATION_START				'$'
#define MTL_EQUATION_END					';'
#define MTL_OPTION								'-'
#define MTL_OUTPUT								'@'


//	Lexical token codes, used to represent identifiers, keywords, literals, operators, and punctuators.
//	These match the enumeration of the .Net bitcode compiler.
typedef enum LexicalCodes
{
		None,
		ConstantValue,	//	const value in equation
		Value8,     		//  const 8-bit unsigned value before equations start
		Value16,    		//  const 16-bit unsigned value before equations start
		Value32,    		//  const 32-bit unsigned value before equations start
		String,					//	string for use in bytecode compiler

		//  equation and token delineation
		EquationStart,
		PriorityEquationStart,
		SuccessiveEquationStart,
		EquationEnd,
		Equals,
		Lambda,
		TokenKey,
		TokenKeyClose,
		TokenAddress,

		//  math and logic operators
		OperatorLogicalNot,
		OperatorBitwiseInvert,
		OperatorMultiply,
		OperatorDivide,
		OperatorModulus,
		OperatorAdd,
		OperatorSubtract,
		OperatorShiftLeft,
		OperatorShiftRight,
		OperatorIsLessThan,
		OperatorIsLessThanOrEqual,
		OperatorIsGreaterThan,
		OperatorIsGreaterThanOrEqual,
		OperatorIsEqual,
		OperatorIsNotEqual,
		OperatorBitwiseAnd,
		OperatorBitwiseXor,
		OperatorBitwiseOr,
		OperatorLogicalAnd,
		OperatorLogicalOr,
		OperatorConditionalQuestion,
		OperatorConditionalSeparator,
		OperatorOpenParentheses,
		OperatorCloseParentheses,

		//  logic functions
		OutputLogicActivityMonitor,
		OutputLogicRisingEdgeUpCounter,
		OutputLogicFallingEdgeUpCounter,
		OutputLogicRisingEdgeToggle,
		OutputLogicFallingEdgeToggle,
		OutputLogicRisingEdgeSkipToggle,
		OutputLogicFallingEdgeSkipToggle,
		OutputLogicRisingEdgeVariableClear,
		OutputLogicFallingEdgeVariableClear,
		OutputLogicRisingEdgeDelay,
		OutputLogicFallingEdgeDelay,

		//  token output options
		OutputSendTokenOnChange,
		OutputSendTokenOnOutputRisingEdge,
		OutputSendTokenOnOutputFallingEdge,
		OutputSendTokenOnOutputRisingByValue,
		OutputSendTokenOnOutputFallingByValue,
		
} LEXICAL_CODES;


//	flags for special functions
typedef enum
{
	MtlFlagsInputBitstate = 0x01,
	MtlFlagsSkipToggle = 0x02,
	MtlFlagsIsEquationOutput = 0x04,
	MtlFlagsShouldBroadcast = 0x08,
	MtlFlagsTokenReceived = 0x10,
	
} MTL_FLAGS;


/**
  * @brief  The time logic token data object.
	*/
typedef struct
{
	//	the token
	TOKEN token;
	
	//	timestamp used for timed logic output options
	//	maximum 60 seconds
	uint16_t timestamp;
	
	//	a mapped token key
	//	if not a global mapped to a local, then this value is KeyNull
	uint16_t mappedTokenKey;
	
} MTL_TOKEN;


/**
  * @brief  The time logic processor data object.
	*/
typedef struct
{
	//	equations file name
	char fileName[MATRIX_FILE_NAME_LENGTH + 1];

	//	equations file location
	uint8_t *fileLocation;

	//	equations file size
	uint32_t fileSize;
	
	//	current equation location
	uint8_t *equationLocation;
	
} MATRIX_TIME_LOGIC_OBJECT;
extern MATRIX_TIME_LOGIC_OBJECT MatrixTimeLogic;

/**
  * @brief  The time logic token table data object.
	*/
typedef struct
{
	//	the table of all tokens named in the equations
	MTL_TOKEN tokens[MATRIX_TIME_LOGIC_TOKEN_TABLE_SIZE];
	
	//	the number of tokens in the table
	uint16_t numTokens;
	
	//	indicates whether table contains broadcast tokens
	bool tokenTableHasBroadcastTokens;

} MATRIX_TIME_LOGIC_TOKEN_TABLE;
extern MATRIX_TIME_LOGIC_TOKEN_TABLE MatrixTimeLogic_TokenTable;

//	macro to get bitcode Int32 value
#define BitcodeInt32Value(x, y) {			\
	(x) = (int32_t)*++(y) << 24;				\
	(x) |= (int32_t)*++(y) << 16;				\
	(x) |= (int32_t)*++(y) << 8;				\
	(x) |= *++(y); }

//	macro to get bitcode UInt16 value
#define BitcodeUInt16Value(x, y) {		\
	(x) = (uint16_t)*++(y) << 8;				\
	(x) |= *++(y); }



/**
  * @brief  Resets the time logic processor.
	* @param  equationFileName: The name of the current equations file (may change per user profile).
  * @retval None
  */
extern void MatrixTimeLogic_Reset(char *equationFileName);

/**
  * @brief  Clocks the time logic processor.
	*					This method supports cooperative task scheduling.
	* @param  None.
  * @retval None
  */
extern void MatrixTimeLogic_Clock(void);

/**
  * @brief  Receives a new token and re-evaluates the equations.
  * @param  token: A pointer to a token to process.
  * @retval None.
  */
extern void MatrixTimeLogic_TokenIn(TOKEN *token);


#endif  //  __MATRIX_TIME_LOGIC_H

