/**
  ******************************************************************************
  * @file    		matrix_token_sequencer.h
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


#ifndef __MATRIX_TOKEN_SEQUENCER_H
#define __MATRIX_TOKEN_SEQUENCER_H


#include <stdint.h>
#include <stdbool.h>
#include "matrix_lib_interface.h"
#include "matrix_tokens.h"


//	the number of instanced token sequencers
#ifndef MTS_NUM_TOKEN_SEQUENCERS
#define MTS_NUM_TOKEN_SEQUENCERS  6 
#endif


//	the token sequencer pattern stack size
#define MTS_PATTERN_STACK_SIZE		3


//	The sync range defines a range of network sync tokens (pattern enumerations)
//	to which the token sequencer will sync, regardless of the local running pattern.
//	
//	The top and bottom range can be set in the behavioral file.
//
//	To ignore all network sync tokens, set top and bottom range to 0.
#define SYNC_RANGE_NONE		0
//
//	To only sync to a network sync token that matches the running pattern, set top and bottom to 8192.
#define SYNC_RANGE_EXACT	8192


/**
  * @brief  A token pattern data object.
  */
typedef struct
{
	//	pattern pointer
	uint8_t *patternPosition;
	
	//	pattern first step pointer
	uint8_t *firstStepPosition;
	
	//	pattern current step pointer
	uint8_t *currentPosition;
	
	//	repeated section pointer
	uint8_t *repeatedSectionPosition;

	//	pattern counter
	uint8_t patternCounter;
	
	//	repeated section counter
	uint8_t repeatedSectionCounter;
	
} MATRIX_TOKEN_PATTERN;

/**
  * @brief  A token sequencer data object.
  */
typedef struct
{
	//	the overall relative output intensity 0~100
	uint16_t outputIntensity;
	
	//	the pattern stack index
	int16_t patternStackIndex;

  //	the next step time
  uint32_t stepTime;

	//	the pattern stack
	MATRIX_TOKEN_PATTERN patternStack[MTS_PATTERN_STACK_SIZE];
	
	//	the bottom of the sync range
	uint16_t syncRangeBottom;
	
	//	the top of the sync range
	uint16_t syncRangeTop;
	
	//	the common token key, if used
	uint16_t commonKey;
	
	//	the sync enable flag
	uint16_t syncEnable;
	
} MATRIX_TOKEN_SEQUENCER;

/**
  * @brief  The token sequencer controller data object.
  */
typedef struct
{
	//	the array of pattern sequencers
	MATRIX_TOKEN_SEQUENCER Sequencers[MTS_NUM_TOKEN_SEQUENCERS];

	//	a pointer to the pattern data
	uint8_t *patternData;
	
	//	the pattern data size
	uint32_t patternDataSize;
	

} MATRIX_TOKEN_SEQUENCER_CONTROLLER;
//extern MATRIX_TOKEN_SEQUENCER_CONTROLLER TokenSequencerController;


/**
  * @brief  Resets the token sequencers.
  * @param  None.
  * @retval None
  */
extern void MatrixTokenSequencerController_Reset(void);

/**
  * @brief  Clocks the token sequencers.
	* @param  None.
  * @retval None.
  */
extern void MatrixTokenSequencerController_Clock(void);

/**
  * @brief  Handles incoming sync messages.
	* @param  token: A pointer to a sync token.
  * @retval None
  */
extern void MatrixTokenSequencerController_SyncIn(TOKEN *token);

/**
  * @brief  Starts and stops token sequencer.
	* @param  token: A pointer to a token.
  * @retval None
  */
extern void MatrixTokenSequencerController_TokenIn(TOKEN *token);

/**
  * @brief  Returns the number of available patterns.
  * @param  ls: The token sequencer data object.
  * @retval The number of available patterns, or zero on error.
  */
extern uint16_t MatrixTokenSequencer_GetNumPatterns(MATRIX_TOKEN_SEQUENCER *ts);


#endif  //  __MATRIX_TOKEN_SEQUENCER_H
