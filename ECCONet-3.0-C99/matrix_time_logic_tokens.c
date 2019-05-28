/**
  ******************************************************************************
  * @file       matrix_time_logic_tokens.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author     M. Latham, Liquid Logic
  * @version    1.0.0
  * @date       Dec 2017
  * @brief      The matrix time-logic token table and methods.
  ******************************************************************************
  * @attention
  * Unless required by applicable law or agreed to in writing, software
  * created by Liquid Logic LLC is delivered "as is" without warranties
  * or conditions of any kind, either express or implied.
  *
  ******************************************************************************
  */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "matrix.h"
#include "matrix_time_logic.h"




/**
  * @brief  The time logic token table data object.
	*/
MATRIX_TIME_LOGIC_TOKEN_TABLE MatrixTimeLogic_TokenTable;


//	helper method for token compare
int MTL_CompareTokens(const void *t1, const void *t2)
{
	//	compare address and key fields
	uint32_t va = *(uint32_t *)t1 & 0xffffff00;
	uint32_t vb = *(uint32_t *)t2 & 0xffffff00;
	return (int)(va > vb) - (int)(va < vb);
}

/**
  * @brief  Finds a table token matching the given bytecode.
  * @param  bytecodeRef: Pointer to bytecode pointer at TokenKey code.  Advanced to the last token byte.
  * @retval A pointer to the token, or null if not found.
  */
MTL_TOKEN *MatrixTimeLogic_TokenTable_TokenFromBitcode(uint8_t **bytecodeRef)
{
	TOKEN token;
	uint8_t *ptr;
	
	//	get token, leaving pointer at next code
	ptr = *bytecodeRef;
	BitcodeUInt16Value(token.key, ptr);
	token.address = 0;
	if (TokenAddress == ptr[1])
	{
		++ptr;
		token.address = *++ptr;
	}
	*bytecodeRef = ptr;
	
	//	try to get token from table	
	return bsearch(&token, &MatrixTimeLogic_TokenTable.tokens,
		MatrixTimeLogic_TokenTable.numTokens, sizeof(MTL_TOKEN), MTL_CompareTokens);
}

/**
  * @brief  Populates the token table from the given bytecode.
  * @param  bytecode: A pointer to the bytecode.
  * @param  bytecodeSize: The logic file data size.
  * @retval None.
  */
void MTL_PopulateTokenTable(uint8_t *bytecode, uint32_t bytecodeSize)
{
	MTL_TOKEN token, prevToken, *tableToken, *lastTableToken;
	MTL_TOKEN *sortToken, *compareToken;
	uint8_t *lastPtr;
	
	//	clear table state and all entries
	MatrixTimeLogic_TokenTable.numTokens = 0;
	MatrixTimeLogic_TokenTable.tokenTableHasBroadcastTokens = false;
	memset(&MatrixTimeLogic_TokenTable.tokens, 0, sizeof(MTL_TOKEN) * MATRIX_TIME_LOGIC_TOKEN_TABLE_SIZE);
	
	//	validate inputs
	if ((NULL == bytecode) || (0 == bytecodeSize))
		return;

	//	clear prev working token
	memset(&prevToken, 0, sizeof(MTL_TOKEN));
	
	//	tabulate all tokens in bytecode
	lastPtr = bytecode + bytecodeSize;
	bytecode += 4;  //  skip security key, initial constants
	if ((bytecode[0] == 0xca) && (bytecode[1] == 0xfe))
		bytecode += (4 + bytecode[2] + (bytecode[3] << 8));
	while (++bytecode < lastPtr) 
	{
		switch (*bytecode)
		{
			//	clear previous token at start of each new equation
			case EquationStart:
			case PriorityEquationStart:
			case SuccessiveEquationStart:
				memset(&prevToken, 0, sizeof(MTL_TOKEN));
				break;
			
			//	skip constant values
			case ConstantValue:
				bytecode += 4;
				break;
			
			//	process tokens
			case TokenKey:
				
				//	clear bytecode token and set key
				memset(&token, 0, sizeof(MTL_TOKEN));
				token.token.key = (uint16_t)*++bytecode << 8;
				token.token.key |= *++bytecode;
			
				//	if bytecode token is preceeded by Lambda, 
				//	and bytecode token is not local var, and previous bytecode token is local var,
				//	then set mapped token key
				if ((*(bytecode - 3) == Lambda) && Key_IsLocalVariable(prevToken.token.key)
					&& !(Key_IsLocalVariable(token.token.key)))
						token.mappedTokenKey = prevToken.token.key;
				
				//	if bytecode token address given, then set it
				if (*(bytecode + 1) == TokenAddress)
				{
					++bytecode;
					token.token.address = *++bytecode;
				}

				//	get pointer to table token with same key and address as bytecode token
				//	if none found then pointer will point to next unused pre-cleared token in table
				tableToken = MatrixTimeLogic_TokenTable.tokens;
				lastTableToken = tableToken + MatrixTimeLogic_TokenTable.numTokens;
				while (tableToken < lastTableToken)
				{
					if (*(uint32_t *)&token == (*(uint32_t *)tableToken & 0xffffff00))
						break;
					++tableToken;
				}
						
				//	if new token in table
				if (tableToken == lastTableToken)
				{
					//	set table token key and address fields
					tableToken->token.key = token.token.key;
					tableToken->token.address = token.token.address;
					
					//	bump table token size
					++MatrixTimeLogic_TokenTable.numTokens;
				}
					
				//	if bytecode token is equation result
				if (EquationEnd == *(bytecode + 1))
				{
					//	flag table token as equation output
					tableToken->token.flags |= MtlFlagsIsEquationOutput;
					
					//	if bytecode token is a global input status or output status,
					//	then set table token broadcast flag and table has broadcast tokens status
					if (!Key_IsLocalVariable(tableToken->token.key)
						&& (Key_IsInputStatus(tableToken->token.key) || Key_IsOutputStatus(tableToken->token.key)))
					{
						tableToken->token.flags |= MtlFlagsShouldBroadcast;
						MatrixTimeLogic_TokenTable.tokenTableHasBroadcastTokens = true;
					}

					//	if bytecode token has Lambda mapping, then set it in the table token
					if (KeyNull != token.mappedTokenKey)
						tableToken->mappedTokenKey = token.mappedTokenKey;
				}
					
				//	save bytecode token to previous token
				prevToken = token;
				break;
				
			//	all other codes are single-byte without variable
			default:
				break;
		}

		//	if token table is full (designer should not let happen) then done
		if (MatrixTimeLogic_TokenTable.numTokens >= MATRIX_TIME_LOGIC_TOKEN_TABLE_SIZE)
			break;

	}  //  while
	
	//	sort the tokens in the table
	tableToken = MatrixTimeLogic_TokenTable.tokens;
	lastTableToken = tableToken + MatrixTimeLogic_TokenTable.numTokens;
	sortToken = tableToken;
	while (++sortToken < lastTableToken)
	{
		compareToken = sortToken;
		while (--compareToken >= tableToken)
			if ((*(uint32_t *)compareToken & 0xffffff00) <= (*(uint32_t *)sortToken & 0xffffff00))
				break;
		if (++compareToken < sortToken)
		{
			token = *sortToken;
			memmove(compareToken + 1, compareToken,
				(uintptr_t)sortToken - (uintptr_t)compareToken);
			*compareToken = token;
		}		
	}
}			
