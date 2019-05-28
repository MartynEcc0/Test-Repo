/**
  ******************************************************************************
  * @file    		matrix_event_index.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		The Matrix event index manager.
	*							
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  * OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */


#include "matrix_event_index.h"


/**
  * @brief  The Matrix event index data object.
  */
MATRIX_EVENT_INDEX_OBJECT MatrixEventIndex;


/**
  * @brief  Resets the event index.
  * @param  None.
  * @retval None.
  */
void MatrixEventIndex_Reset(void)
{
	//	clear the index
	MatrixEventIndex.eventIndex = 0;
}

/**
  * @brief  Returns the event index.
	* @param  None.
  * @retval The event index.
  */
uint8_t Matrix_GetEventIndex(void)
{
	if (0 == MatrixEventIndex.eventIndex)
		++MatrixEventIndex.eventIndex;
	return MatrixEventIndex.eventIndex;
}

/**
  * @brief  Bumps the event index.
	*					This method is called by the matrix transmitter and should
	*					not be called by other modules.
	* @param  None.
  * @retval The event index.
  */
void Matrix_NextEventIndex(void)
{
	++MatrixEventIndex.eventIndex;
	if (0 == MatrixEventIndex.eventIndex)
		++MatrixEventIndex.eventIndex;
}

/**
  * @brief  Handles a new received event index.
	*					This method is called by the matrix receiver and should
	*					not be called by other modules.
	* @param  The event index.
  * @retval None.
  */
void Matrix_NewEventIndex(uint8_t index)
{
	int8_t result;
	
	//	incoming index of zero is ignored
	if (0 == index)
		return;
	
	//	if local index is zero (just booted) or incoming index is newer,
	//	then update local index
	result = index - MatrixEventIndex.eventIndex;
	if ((0 == MatrixEventIndex.eventIndex) || (result > 0))
		MatrixEventIndex.eventIndex = index;
}

/**
  * @brief  Returns a value indicating whether the given event index has expired.
	* @param  The event index.
  * @retval None.
  */
bool Matrix_IsEventIndexExpired(uint8_t index)
{
	//	incoming index of zero is never expired
	if (0 == index)
		return false;
	
	int8_t result = index - MatrixEventIndex.eventIndex;
	return (result < 0);
}

