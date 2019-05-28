/**
  ******************************************************************************
  * @file       matrix_time_logic.c
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


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "matrix_flash_drive.h"
#include "matrix.h"
#include "matrix_time_logic.h"

//	external methods
extern void MTL_PopulateTokenTable(uint8_t *bytecode, uint32_t bytecodeSize);
extern int  MTL_CompareTokens(const void *t1, const void *t2);
extern int  MTL_PerformCalculation(uint8_t **ptrRef, uint8_t *lastPtr, int32_t *out_result, uint8_t **outFirstToken);
extern int  MTL_ProcessOutputOptions(uint8_t **ptrRef, uint8_t *lastPtr, int32_t calculatedValue, uint8_t *firstToken);



/**
  * @brief  The time logic data object.
	*/
MATRIX_TIME_LOGIC_OBJECT MatrixTimeLogic;    

/**
  * @brief  Resets the time logic processor.
	* @param  None.
  * @retval None
  */
void MatrixTimeLogic_Reset(char *equationFileName)
{
	FLASH_DRIVE_FILE file;
	uint16_t checksum;
	
	//	save the file name
	strncpy(MatrixTimeLogic.fileName, equationFileName, MATRIX_FILE_NAME_LENGTH);
	MatrixTimeLogic.fileName[MATRIX_FILE_NAME_LENGTH] = 0;
	
	//	reset the state
	MatrixTimeLogic.fileLocation = NULL;
	MatrixTimeLogic.fileSize = 0;
	MatrixTimeLogic.equationLocation = 0;

	//	try to get file
	if (0 == FlashDrive_GetFile(MATRIX_TIME_LOGIC_FILE_VOLUME_INDEX,
		equationFileName, &file, NULL))
	{
		//	check file integrity
		if (FlashDrive_CheckFileIntegrity(&file, &checksum))
		{
			//	save the file location and size
			MatrixTimeLogic.fileLocation = (uint8_t *)file.dataLocation;
			MatrixTimeLogic.fileSize = file.dataSize;
		}
	}
			
	//	populate the token table
	MTL_PopulateTokenTable((uint8_t *)MatrixTimeLogic.fileLocation,	MatrixTimeLogic.fileSize);
}

/**
  * @brief  Clocks the time logic processor.
	*					This method supports cooperative task scheduling.
	* @param  None.
  * @retval None
  */
void MatrixTimeLogic_Clock(void)
{
	uint8_t *lastLocation, *currentLocation, *firstToken;
	int32_t result;
	int status;

	//	validate file location and size
	if ((NULL == MatrixTimeLogic.fileLocation) || (0 == MatrixTimeLogic.fileSize))
		return;
	
	//	validate file key
	if (MATRIX_TIME_LOGIC_FILE_KEY != *(uint32_t *)MatrixTimeLogic.fileLocation)
	{
		MatrixTimeLogic_Reset(MatrixTimeLogic.fileName);
		return;
	}
	
	//	if initial constants, then skip them
	currentLocation = MatrixTimeLogic.fileLocation + 4;
	if ((MatrixTimeLogic.fileLocation[4] == 0xca) && (MatrixTimeLogic.fileLocation[5] == 0xfe))
		currentLocation += (4 + MatrixTimeLogic.fileLocation[6] + (MatrixTimeLogic.fileLocation[7] << 8));

	//	process all priority equations
	lastLocation = MatrixTimeLogic.fileLocation + MatrixTimeLogic.fileSize; 
	while ((currentLocation < lastLocation) && (PriorityEquationStart == *currentLocation)) 
	{
		//	perform calculation and process output options
		result = 0;
		if (0 == (status = MTL_PerformCalculation(&currentLocation, lastLocation, &result, &firstToken)))
			status = MTL_ProcessOutputOptions(&currentLocation, lastLocation, result, firstToken);
		
		//	if calculation or output error, reset time-logic
		if (0 != status)
		{
			MatrixTimeLogic_Reset(MatrixTimeLogic.fileName);
			return;
		}
	}
	
	//	if no remaining non-priority equations, then done
	if (currentLocation >= lastLocation)
		return;

	
	//	bounds-check and wrap the normal priority equation location
	if ((MatrixTimeLogic.equationLocation < currentLocation)
		|| (MatrixTimeLogic.equationLocation >= lastLocation))
	{
		//	point to first normal priority equation
		MatrixTimeLogic.equationLocation = currentLocation;
	}
	
	//	for all normal-priority equations in a succession
	while (MatrixTimeLogic.equationLocation < lastLocation) 
	{
		//	perform calculation and process output options
		result = 0;
		if (0 == (status = MTL_PerformCalculation(&MatrixTimeLogic.equationLocation, lastLocation, &result, &firstToken)))
			status = MTL_ProcessOutputOptions(&MatrixTimeLogic.equationLocation, lastLocation, result, firstToken);
		
		//	if calculation or output error, reset time-logic
		if (0 != status)
		{
			MatrixTimeLogic_Reset(MatrixTimeLogic.fileName);
			return;
		}
		
		//	if no more successive equations, then done
		if ((MatrixTimeLogic.equationLocation >= lastLocation)
			|| (SuccessiveEquationStart != *MatrixTimeLogic.equationLocation))
			break;
	}
}

/**
  * @brief  Receives a new token.
  * @param  token: A pointer to a token to process.
  * @retval None.
  */
void MatrixTimeLogic_TokenIn(TOKEN *token)
{
	MTL_TOKEN idToken, *tableToken;
	TOKEN appToken;
	uint16_t i;

	//	table variable modification qualifiers
	//
	//	corresponds to an		is an input	|				should
	//	equation output?		status?   	| 	update variable?
	//=======================================================
	//			No								x					|		    Yes
	//			Yes								No				|				No
	//			x 								Yes								Yes
	
	//	copy token key
	idToken.token.key = token->key;
	
	//	cover both scenarios for when the address is specified for static adress routing
	//	or is "don't care" and set to zero in the variable
	for (i = 0; i < 2; i++)
	{
		//	set address and product type
		idToken.token.address = (i & 1) ? token->address : 0;

		//	try to get token from table	
		if (NULL != (tableToken = bsearch(&idToken, &MatrixTimeLogic_TokenTable.tokens,
			MatrixTimeLogic_TokenTable.numTokens, sizeof(MTL_TOKEN), MTL_CompareTokens)))
		{
			//	if token has local translation
			if (tableToken->mappedTokenKey != KeyNull)
			{
				appToken = *token;
				appToken.key = tableToken->mappedTokenKey;
				Matrix.appInterface->tokenCallback(&appToken);
			}
			
			//	if not an equation output variable or is an input status
			if ((0 == (tableToken->token.flags & MtlFlagsIsEquationOutput))
				|| (Key_IsInputStatus(token->key)))
			{
				//	update the token value
				tableToken->token.value = token->value;
				
				//	set the token received flag
				tableToken->token.flags |= MtlFlagsTokenReceived;
			}
		}
	}
}

/**
  * @brief  Gets a pointer to the current equation file.
	* @param  None.
  * @retval A pointer to the current equation file, or null if none found.
  */
const uint8_t *Matrix_GetCurrentEquationFile(void)
{
	//	validate file location and size
	if ((NULL == MatrixTimeLogic.fileLocation) || (0 == MatrixTimeLogic.fileSize))
		return NULL;
	
	//	validate file key
	if (MATRIX_TIME_LOGIC_FILE_KEY != *(uint32_t *)MatrixTimeLogic.fileLocation)
	{
		MatrixTimeLogic_Reset(MatrixTimeLogic.fileName);
		return NULL;
	}
	
	//	validate initial constants
	if ((MatrixTimeLogic.fileLocation[4] != 0xca) || (MatrixTimeLogic.fileLocation[5] != 0xfe))
		return NULL;

	//	return the constants location
	return MatrixTimeLogic.fileLocation;
}

