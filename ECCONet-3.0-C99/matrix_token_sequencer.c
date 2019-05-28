/**
  ******************************************************************************
  * @file    		matrix_token_sequencer.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		2.0.0
  * @date    		Oct 2017
  * @brief   		A set of token sequencers.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  * OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "matrix.h"
#include "matrix_flash_drive.h"
#include "matrix_codec.h"
#include "matrix_token_sequencer.h"


//	private methods
static uint8_t *GetPattern(uint16_t patternEnum, uint16_t *stepCount);
static uint8_t *GetAllOffStep(uint8_t *patternPosition);
static uint16_t GetRootPatternEnum(MATRIX_TOKEN_SEQUENCER *ts);

static bool IsPatternFilePointerValid(void);

static int Start(uint16_t sequencerIndex, uint16_t patternEnum, uint8_t numPatternRepeats, bool isRoot);
static void NextStep(uint16_t sequencerIndex);
static void EndCurrentPattern(uint16_t sequencerIndex);
static void Stop(uint16_t sequencerIndex);

static void SendToken(TOKEN *token);
static void SendDefaultStateToken(TOKEN *token);
static void SendCommonKeyToken(TOKEN *token);


//	token sequencer controller data object
MATRIX_TOKEN_SEQUENCER_CONTROLLER TokenSequencerController;



/**
  * @brief  Returns a value indicating whether the sequencer is running.
  * @param  sequencerIndex: The token sequencer index 0~2.
  * @retval True if the sequencer is running.
  */
bool Matrix_IsTokenSequencerRunning(uint16_t sequencerIndex)
{
	if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
		return false;
	return (0 <= TokenSequencerController.Sequencers[sequencerIndex].patternStackIndex);
}

/**
  * @brief  Returns the number of available patterns.
  * @param  None.
  * @retval The number of available patterns, or zero on error.
  */
uint16_t Matrix_GetTokenSequencerNumPatterns(void)
{
	if (IsPatternFilePointerValid())
		return ((uint16_t)TokenSequencerController.patternData[4] << 8) | TokenSequencerController.patternData[5];
	return 0;
}



//////////////////////////////////////////////////////////////////////
//
//	Internal library methods

/**
  * @brief  Resets the token sequencers.
  * @param  None.
  * @retval None
  */
void MatrixTokenSequencerController_Reset(void)
{
	MATRIX_TOKEN_SEQUENCER *ts;
	FLASH_DRIVE_FILE file;
	uint16_t index, checksum;

			
	//	try to get pattern file
	TokenSequencerController.patternData = NULL;
	TokenSequencerController.patternDataSize = 0;
	if (0 == FlashDrive_GetFile(MATRIX_TOKEN_PATTERN_VOLUME_INDEX,
		MATRIX_TOKEN_PATTERN_FILE_NAME, &file, NULL))
	{
		//	validate the pattern data
		if (FlashDrive_CheckFileIntegrity(&file, &checksum))
		{
			//	set the pattern data location and data size
			TokenSequencerController.patternData = (uint8_t *)file.dataLocation;
			TokenSequencerController.patternDataSize = file.dataSize;
		}
	}		
	
	for (index = 0; index < MTS_NUM_TOKEN_SEQUENCERS; ++index)
	{
		//	reset sequencer
		ts = &TokenSequencerController.Sequencers[index];
		ts->patternStackIndex = -1;
		ts->outputIntensity = 100;
		ts->syncRangeBottom = SYNC_RANGE_EXACT;
		ts->syncRangeTop = SYNC_RANGE_EXACT;
		ts->commonKey = KeyNull;
	}
}

/**
  * @brief  Clocks the token sequencers.
	* @param  None.
  * @retval None.
  */
void MatrixTokenSequencerController_Clock(void)
{
	MATRIX_TOKEN_SEQUENCER *ts;
	uint16_t index;

	//	validate file location and size
	if ((NULL == TokenSequencerController.patternData) || (0 == TokenSequencerController.patternDataSize))
		return;
	
	//	validate file key
	if (MATRIX_TOKEN_PATTERN_FILE_KEY != *(uint32_t *)TokenSequencerController.patternData)
	{
		MatrixTokenSequencerController_Reset();
		return;
	}
	
	//	for all token sequencers
	for (index = 0; index < MTS_NUM_TOKEN_SEQUENCERS; ++index)
	{
		//	get sequencer
		ts = &TokenSequencerController.Sequencers[index];
		
		//	if token sequencer running and time for next set of tokens, then next pattern step
		if ((0 <= ts->patternStackIndex) && (0 <= (int32_t)(Matrix.systemTime - ts->stepTime)))
			NextStep(index);
	}
}

/**
  * @brief  Handles incoming token messages.
	* @param  token: A pointer to a token.
  * @retval None
  */
void MatrixTokenSequencerController_TokenIn(TOKEN *token)
{
	MATRIX_TOKEN_SEQUENCER *ts;
	TOKEN tk;
	uint16_t sequencerIndex;

	//	validate token
	if (NULL == token)
		return;
	
	//	get token without prefix
	tk = *token;
	tk.key = Key_WithoutPrefix(tk.key);
	
	//	if an indexed sequencer token 
	if ((KeyIndexedSequencer <= tk.key) &&
		 ((KeyIndexedSequencer + Region_Size__Indexed_Sequencer_Three_Byte) > tk.key))
	{
		//	get sequencer index and data object
		sequencerIndex = tk.key - KeyIndexedSequencer;
		if (MTS_NUM_TOKEN_SEQUENCERS > sequencerIndex)
		{
			ts = &TokenSequencerController.Sequencers[sequencerIndex];

			//	set the output intensity
			ts->outputIntensity = (tk.value >> 16) & 0xff;
			
			//	if stopping a pattern
			tk.value &= 0xffff;
			if (Pattern_Stop == tk.value)
			{
				//	stop any existing pattern stack
				Stop(sequencerIndex);
			}
			else  //	else starting a pattern
			{
				//	validate sequencer pattern pointer
				if (IsPatternFilePointerValid())
				{
					//	if not already running the requested pattern
					if (GetRootPatternEnum(ts) != tk.value)
					{
						//	stop any existing pattern stack
						Stop(sequencerIndex);

						//	start the pattern
						Start(sequencerIndex, tk.value, 0, true);
					}
				}
			}
		}
	}
	else  //	all other tokens
	{
		switch (tk.key)
		{
			case KeyTokenSequencerSyncRange:
			{
				//	get sequencer index and data object
				sequencerIndex = token->address - MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
				if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
					return;
				ts = &TokenSequencerController.Sequencers[sequencerIndex];
				ts->syncRangeBottom = tk.value & 0xffff;
				ts->syncRangeTop = tk.value >> 16;
				break;
			}
			
			case KeyTokenSequencerSync:
			{
				//	if token address <= this device address, then skip
				if (token->address <= Matrix_GetCanAddress())
					break;

				//	strip off enumeration prefix
				tk.value &= ~(PatternEnumPrefixMask << 8);
				
				//	for all token sequencers
				for (sequencerIndex = 0; sequencerIndex < MTS_NUM_TOKEN_SEQUENCERS; ++sequencerIndex)
				{
					//	get sequencer
					ts = &TokenSequencerController.Sequencers[sequencerIndex];
					
					//	if sequencer running and sequencer address is higher than sync token address
					if (0 <= ts->patternStackIndex
						&& (MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS + sequencerIndex) > token->address)
					{
						//	if sequencer syncing
						if (SYNC_RANGE_NONE != ts->syncRangeTop)
						{
							//	if sequencer syncing to a range and network sync is in range
							//	or sequencer only syncing to exact match and network sync matches running pattern
							//	then restart the root pattern
							if (((tk.value >= ts->syncRangeBottom) && (tk.value <= ts->syncRangeTop))
								|| ((SYNC_RANGE_EXACT == ts->syncRangeBottom) && (tk.value == GetRootPatternEnum(ts))))
							{
								//	restart root pattern
								ts->patternStackIndex = 0;
								ts->patternStack[0].currentPosition =	ts->patternStack[0].firstStepPosition;
								ts->stepTime = Matrix.systemTime;
								NextStep(sequencerIndex);
							}
						}
					}
				}
				break;
			}
			
			case KeyIndexedTokenSequencerWithPattern:
			{
				//	set the sequencer intensity
				tk.key = KeyTokenSequencerIntensity;
				tk.value = (token->value >> 8) & 0xff;
				tk.address = (token->value & 0xff) + MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
				tk.flags = 0;
				MatrixTokenSequencerController_TokenIn(&tk);
				
				//	set the sequencer pattern
				tk.key = KeyTokenSequencerPattern;
				tk.value  = token->value >> 16;
				tk.address = (token->value & 0xff) + MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
				tk.flags = 0;
				MatrixTokenSequencerController_TokenIn(&tk);
				break;
			}
			
			case KeyTokenSequencerPattern:
			{
				//	get sequencer index and data object
				sequencerIndex = token->address - MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
				if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
					break;
				ts = &TokenSequencerController.Sequencers[sequencerIndex];

				//	if stopping a pattern
				if (Pattern_Stop == (token->value & 0xffff))
				{
					//	stop any existing pattern stack
					Stop(sequencerIndex);
				}
				else  //	else starting a pattern
				{
					//	validate sequencer pattern pointer
					if (!IsPatternFilePointerValid())
						break;
					
					//	if not already running the requested pattern
					if (GetRootPatternEnum(ts) != (token->value & 0xffff))
					{
						//	stop any existing pattern stack
						Stop(sequencerIndex);

						//	start the pattern
						Start(sequencerIndex, token->value & 0xffff, 0, true);
					}
				}
				break;
			}

			case KeyTokenSequencerIntensity:
			{			
				//	get sequencer index and data object
				sequencerIndex = token->address - MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
				if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
					break;
				ts = &TokenSequencerController.Sequencers[sequencerIndex];

				//	scale intensity up for quick scaling during sequencing
				//ts->outputIntensity = (100 < token->value) ? 256 : (token->value * 256) / 100;
				ts->outputIntensity = token->value;
				break;
			}
			
			default:
				break;
		}
	}
}


//////////////////////////////////////////////////////////////////////
//
//	Internal library methods

/**
  * @brief  Returns a value indicating whether the pattern file pointer is valid.
  * @param  None.
  * @retval Returns a value indicating whether the pattern file pointer is valid.
  */
static bool IsPatternFilePointerValid(void)
{
	return ((NULL != TokenSequencerController.patternData)
		&& (0 != TokenSequencerController.patternDataSize)
		&& (MATRIX_TOKEN_PATTERN_FILE_KEY == *(uint32_t *)TokenSequencerController.patternData));
}

/**
  * @brief  Get pointer to the all-off step.
  * @param  patternPosition: The pattern position.
  * @retval Returns pointer to all-off step, or null if not found.
  */
static uint8_t *GetAllOffStep(uint8_t *patternPosition)
{
	//	validate pattern position
	if (NULL == patternPosition)
		return NULL;
	
	//	advance past pattern header with repeats and pattern enum
	patternPosition += 3;
	
	//	advance past step prefix
	if (PatternPrefix_PatternStepWithAllOff == (*patternPosition & PatternPrefix_Mask))
		return patternPosition + 1;
	else if (PatternPrefix_PatternStepWithPeriod	== (*patternPosition & PatternPrefix_Mask))
		return patternPosition + 2;
	return NULL;
}

/**
  * @brief  Sends a pattern token to the application.
  * @param  token: The token to send.
  * @retval None.
  */
static void SendToken(TOKEN *token)
{
	uint16_t sequencerIndex;
	
	//	validate token
	if (NULL == token)
		return;

	//	multiply token value by overall expression intensity
	sequencerIndex = token->address - MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
	if (MTS_NUM_TOKEN_SEQUENCERS > sequencerIndex)
		token->value =
		  (token->value * TokenSequencerController.Sequencers[sequencerIndex].outputIntensity) / 100;
	
	//	clear flags
	token->flags = 0;

	//	send token to application
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->tokenCallback))
		Matrix.appInterface->tokenCallback(token);
}

/**
  * @brief  Sends a default state token to the application.
  * @param  token: The token to send.
  * @retval None.
  */
static void SendDefaultStateToken(TOKEN *token)
{
	uint16_t sequencerIndex;
	
	//	validate token
	if (NULL == token)
		return;

	//	multiply token value by overall expression intensity
	sequencerIndex = token->address - MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
	if (MTS_NUM_TOKEN_SEQUENCERS > sequencerIndex)
		token->value =
		  (token->value * TokenSequencerController.Sequencers[sequencerIndex].outputIntensity) / 100;
	
	//	set default state value flag
	token->flags = TF_DEFAULT_STATE;

	//	send token to application
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->tokenCallback))
		Matrix.appInterface->tokenCallback(token);
}

/**
  * @brief  Sends a step method pattern token to the application.
  * @param  token: The token to send.
  * @retval None.
  */
static void SendCommonKeyToken(TOKEN *token)
{
	uint16_t sequencerIndex;
	
	//	if have token and app token callback
	if ((NULL != token) && (NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->tokenCallback))
	{
		//	add output intensity to bits 16-22
		sequencerIndex = token->address - MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS;
		if (MTS_NUM_TOKEN_SEQUENCERS > sequencerIndex)
		{
			token->value &= ~0x007F0000;
			token->value |= (0x007F0000 &
				((uint32_t)TokenSequencerController.Sequencers[sequencerIndex].outputIntensity << 16));
		}
		
		//	clear flags
		token->flags = 0;

		//	send token to application
		Matrix.appInterface->tokenCallback(token);
	}
}

/**
  * @brief  Get the current root pattern enumeration.
  * @param  ts: The token sequencer data object.
  * @retval The current root pattern key, or KeyNull if no pattern running.
  */
static uint16_t GetRootPatternEnum(MATRIX_TOKEN_SEQUENCER *ts)
{
	//	validate sequencer, stack and pattern position
	if ((NULL == ts) || (0 > ts->patternStackIndex)
		|| (NULL == ts->patternStack[0].patternPosition))
		return Pattern_Stop;
	
	return ((uint16_t)(ts->patternStack[0].patternPosition[1]	& ~PatternEnumPrefixMask) << 8)
		| ts->patternStack[0].patternPosition[2];
}

/**
  * @brief  Starts a pattern with the given token.
  * @param  sequencerIndex: The token sequencer index.
  * @param  token: The pattern token.
  * @param  numPatternRepeats: The number of pattern repeats.
  * @param  isRoot: True if the requested pattern is a root pattern.
  * @retval Returns 0 on success, else -1.
  */
static int Start(uint16_t sequencerIndex, uint16_t patternEnum, uint8_t numPatternRepeats, bool isRoot)
{
	MATRIX_TOKEN_PATTERN *pattern;
	MATRIX_TOKEN_SEQUENCER *ts;
	uint8_t *patternPosition;
	uint16_t stepCount;


	//	get and validate sequencer
	if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
		return -1;
	ts = &TokenSequencerController.Sequencers[sequencerIndex];

	//	get and validate pattern position
	patternPosition = GetPattern(patternEnum, &stepCount);
	if (NULL == patternPosition)
		return -1;

	//	validate room on the pattern stack
	if ((MTS_PATTERN_STACK_SIZE - 1) <= ts->patternStackIndex)
		return -1;


	//	bump the stack index
	++ts->patternStackIndex;
	
	//	if a root pattern request, then set number of repeats from pattern start entry 
	//	enable sync if sync range is specified and pattern has more than one step entry
	//
	//	a nested (non-root) pattern entry number of repeats is passed to this method
	//
	if (isRoot)
	{
		numPatternRepeats = *patternPosition & 0x0f;
		ts->syncEnable = ((stepCount > 1) && (ts->syncRangeTop != SYNC_RANGE_NONE));
	}
	
	//	save common key
	//	determines whether an AnyToken pattern or SingleToken pattern format
	if (patternPosition[1] & PatternMode_StepDictionaryKey)
		ts->commonKey = KeyStepMethodDictionaryKey;
	else if (patternPosition[1] & PatternMode_LedMatrixKey)
		ts->commonKey = KeyLedMatrixMessage;
	else
		ts->commonKey = KeyNull;

	//	save pattern position and advance past pattern header
	pattern = &ts->patternStack[ts->patternStackIndex];
	pattern->patternPosition = patternPosition;
	patternPosition += 3;
	
	//	if pattern has an all-off step, then advance past the all-off tokens
	if (PatternPrefix_PatternStepWithAllOff == (*patternPosition & PatternPrefix_Mask))
	{
		//	bump past code
		++patternPosition;

		//	if sequencing a common key
		if (ts->commonKey != KeyNull)
			patternPosition += Key_ValueSize(ts->commonKey);
		else
			MatrixCodec_Decompress(&patternPosition,
				((TokenSequencerController.patternData + TokenSequencerController.patternDataSize) - patternPosition),
				0, NULL);
	}
	
	//	save first step position and number of repeats
	pattern->firstStepPosition = patternPosition;
	pattern->patternCounter = numPatternRepeats;

	//	clear repeated section
	pattern->repeatedSectionPosition = NULL;
	pattern->repeatedSectionCounter = 0;
	
	//	start sequencing and run first pattern step
	pattern->currentPosition = pattern->firstStepPosition;
	ts->stepTime = Matrix.systemTime;
	NextStep(sequencerIndex);

	//	return success
	return 0;
}

/**
  * @brief  Stops the current pattern, which may not be the root pattern.
  * @param  sequencerIndex: The sequencer index.
  * @param  buildSorted: True to build sorted all-off list.
  * @retval None.
  */
static void EndCurrentPattern(uint16_t sequencerIndex)
{
	MATRIX_TOKEN_SEQUENCER *ts;
	MATRIX_TOKEN_PATTERN *pattern;
	TOKEN token;
	uint32_t numBytes;


	//	get sequencer
	if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
		return;
	ts = &TokenSequencerController.Sequencers[sequencerIndex];

	//	get all-off step tokens position
	pattern = &ts->patternStack[ts->patternStackIndex];
	pattern->currentPosition = GetAllOffStep(pattern->patternPosition);
	
	//	if have tokens position
	if (NULL != pattern->currentPosition)
	{
		//	if sequencing a common key
		if (ts->commonKey != KeyNull)
		{
			//	set token with value
			token.address = MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS + sequencerIndex;
			token.key = ts->commonKey;
			token.value = 0;
			numBytes = Key_ValueSize(ts->commonKey);
			while (numBytes--)
			{
				token.value = (token.value << 8) | *pattern->currentPosition;
				pattern->currentPosition += 1;
			}
			SendCommonKeyToken(&token);
		}
		else  //  sequencing multiple keys
		{
			numBytes = (TokenSequencerController.patternData + TokenSequencerController.patternDataSize)
				- pattern->currentPosition;
			MatrixCodec_Decompress(&pattern->currentPosition, numBytes,
				MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS + sequencerIndex,
				SendDefaultStateToken);
		}
	}
	
	//	pop pattern off stack
	--ts->patternStackIndex;
}

/**
  * @brief  Stops all running patterns in the stack of the given sequencer.
  * @param  ts: The sequencer index.
  * @retval None.
  */
static void Stop(uint16_t sequencerIndex)
{
	if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
		return;
	while (0 <= TokenSequencerController.Sequencers[sequencerIndex].patternStackIndex)
		EndCurrentPattern(sequencerIndex);
}

/**
  * @brief  Handles the next expression step.
  * @param  sequencerIndex: The token sequencer index.
  * @retval None.
  */
static void NextStep(uint16_t sequencerIndex)
{
	MATRIX_TOKEN_SEQUENCER *ts;
	MATRIX_TOKEN_PATTERN *pattern;
	TOKEN token;
	uint32_t numBytes;
	
	//	get sequencer and verify sequencer is running
	if (MTS_NUM_TOKEN_SEQUENCERS <= sequencerIndex)
		return;
	ts = &TokenSequencerController.Sequencers[sequencerIndex];
	if (0 > ts->patternStackIndex)
		return;
	
	//	get pattern
	pattern = &ts->patternStack[ts->patternStackIndex];
	
	//	if at end of pattern
	numBytes = (TokenSequencerController.patternData + TokenSequencerController.patternDataSize) - pattern->currentPosition;
	if ((0 == numBytes)
		|| (PatternPrefix_PatternWithRepeats == (*pattern->currentPosition & PatternPrefix_Mask)))
	{
		//	if infinite repeat or counting down, then go back to first step of pattern
		if ((0 == pattern->patternCounter) || (0 != --pattern->patternCounter)) 
			pattern->currentPosition = pattern->firstStepPosition;
		else
		{
			//	stop the current pattern and get previous pattern 
			EndCurrentPattern(sequencerIndex);
			if (0 > ts->patternStackIndex)
				return;
			pattern = &ts->patternStack[ts->patternStackIndex];
		}
	}
	
	//	if at start of root pattern and syncing, then send sync
	if ((pattern->currentPosition == pattern->firstStepPosition)
		&& (0 == ts->patternStackIndex) && ts->syncEnable)
	{
		//	send sync message to local sequencers, equation processor and CAN bus
		token.address = MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS + sequencerIndex;
		token.key = KeyTokenSequencerSync | (KeyPrefix_OutputStatus << 8);
		token.value = GetRootPatternEnum(ts);
		MatrixTokenSequencerController_TokenIn(&token);
		MatrixTimeLogic_TokenIn(&token);
		Matrix_SendSync(&token);
	}

	//	if a section start with repeats
	if (PatternPrefix_PatternSectionStartWithRepeats
		== (*pattern->currentPosition & PatternPrefix_Mask))
	{
		//	get repeat count and bump to next token
		pattern->repeatedSectionCounter	= (*pattern->currentPosition & ~PatternPrefix_Mask);
		pattern->currentPosition += 1;
		pattern->repeatedSectionPosition = pattern->currentPosition;
	}
	//	else if a section end
	else if (PatternPrefix_PatternSectionEnd == (*pattern->currentPosition & PatternPrefix_Mask))
	{
		if ((0 != pattern->repeatedSectionCounter) && (0 != --pattern->repeatedSectionCounter))
			pattern->currentPosition = pattern->repeatedSectionPosition;
		else
			pattern->currentPosition += 1;
	}
	
	//	if a pattern step with period
	if (PatternPrefix_PatternStepWithPeriod
		== (*pattern->currentPosition & PatternPrefix_Mask))
	{
		//	get next step time
		ts->stepTime += ((((uint16_t)pattern->currentPosition[0] & ~PatternPrefix_Mask) << 8)
			| pattern->currentPosition[1]);
		pattern->currentPosition += 2;

		//	if sequencing a common key
		if (ts->commonKey != KeyNull)
		{
			//	set token with value
			token.address = MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS + sequencerIndex;
			token.key = ts->commonKey;
			token.value = 0;
			numBytes = Key_ValueSize(ts->commonKey);
			while (numBytes--)
			{
				token.value = (token.value << 8) | *pattern->currentPosition;
				pattern->currentPosition += 1;
			}
			SendCommonKeyToken(&token);
		}
		else  //  sequencing multiple key
		{
			//	decompress token section
			numBytes = (TokenSequencerController.patternData + TokenSequencerController.patternDataSize) - pattern->currentPosition;
			MatrixCodec_Decompress(&pattern->currentPosition, numBytes,
				MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS + sequencerIndex, SendToken);
		}
	}
	//	else if a nested pattern
	else if (PatternPrefix_PatternStepWithRepeatsOfNestedPattern
		== (*pattern->currentPosition & PatternPrefix_Mask))
	{
		//	numBytes is used here for number of repeats
		numBytes = *pattern->currentPosition & ~PatternPrefix_Mask;

		//	build token
		token.address = MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS + sequencerIndex;
		token.key = KeyTokenSequencerPattern;
		token.value = ((uint16_t)pattern->currentPosition[1] << 8) | pattern->currentPosition[2];

		//	advance past nested pattern token
		pattern->currentPosition += 3;

		//	try to nest pattern
		Start(sequencerIndex, token.value, numBytes, false);
	}
}

/**
  * @brief  Gets a pointer to a pattern.
	* @param  patternEnum: A pattern enumeration.
	* @param  stepCount: A pointer to a step count value.
  * @retval Returns a pointer to the pattern if found, else null.
  */
static uint8_t *GetPattern(uint16_t patternEnum, uint16_t *stepCount)
{
	uint8_t *currentPosition, *matchPosition, *lastPosition;
	uint16_t patEnum;
	uint16_t commonKey = KeyNull;

	//	validate inputs
	if (!IsPatternFilePointerValid())
		return NULL;
	
	//	get position after file key and num patterns and get last position
	currentPosition = TokenSequencerController.patternData + 6;
	lastPosition = TokenSequencerController.patternData + TokenSequencerController.patternDataSize;
	
	//	while pattern table to analyze
	matchPosition = NULL;
	while (currentPosition < lastPosition)
	{
		//	switch on table entry code
		switch (*currentPosition & PatternPrefix_Mask)
		{
			//	if a pattern start
			case PatternPrefix_PatternWithRepeats:
				//	if match position not null, then have found pattern and counted all pattern steps, so done
				if (matchPosition != NULL)
					return matchPosition;
				
				//	get the current pattern enumeration
				patEnum = ((uint16_t)(currentPosition[1] & ~PatternEnumPrefixMask) << 8) | currentPosition[2]; 
				
				//	clear the pattern step count
				*stepCount = 0;
				
				//	if enumeration is null, then at end of patterns
				if (Pattern_Stop == patEnum)
					return NULL;
				
				//	else if enumeration matches requested pattern, then set match position
				else if (patEnum == patternEnum)
					matchPosition = currentPosition;

				//	get the pattern mode and set the common key
				if (currentPosition[1] & PatternMode_StepDictionaryKey)
					commonKey = KeyStepMethodDictionaryKey;
				else if (currentPosition[1] & PatternMode_LedMatrixKey)
					commonKey = KeyLedMatrixMessage;
				else
					commonKey = KeyNull;

				//	advance past pattern start and pattern enum
				currentPosition += 3;
				break;

				
			//	if the all-off/priority-reset pattern step, then advance past all-off tokens
			case PatternPrefix_PatternStepWithAllOff:
				//	advance past prefix
				currentPosition += 1;
				
				//	advance past tokens
				if (commonKey != KeyNull)
					currentPosition += Key_ValueSize(commonKey);
				else
					MatrixCodec_Decompress(&currentPosition, lastPosition - currentPosition, 0, NULL);
				break;
				
		
			//	if a section start with repeats
			case PatternPrefix_PatternSectionStartWithRepeats:
				currentPosition += 1;
				break;

			
			//	if a repeat section end
			case PatternPrefix_PatternSectionEnd:
				currentPosition += 1;
				break;

			
			//	if a pattern step with period
			case PatternPrefix_PatternStepWithPeriod:
				//	bump the step count
				*stepCount = *stepCount + 1;
				
				//	advance past prefix and time period
				currentPosition += 2;
					
				//	advance past tokens
				if (commonKey != KeyNull)
					currentPosition += Key_ValueSize(commonKey);
				else
					MatrixCodec_Decompress(&currentPosition, lastPosition - currentPosition, 0, NULL);
				break;
		
				
			//	if a pattern step with repeats of nested patten
			case PatternPrefix_PatternStepWithRepeatsOfNestedPattern:
				currentPosition += 3;
				break;
			
			
			//	if not a valid pattern table code, assume table is corrupt and return null
			default:
				*stepCount = 0;
				return NULL;
			
		}  //  case	
	}  //  while
	
	
	//	the pattern compiler always adds a null pattern enumeration at end of table,
	//	so if search runs past end of table assume table is corrupt and return null
	*stepCount = 0;
	return NULL;
}
