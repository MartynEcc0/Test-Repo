/**
  ******************************************************************************
  * @file    		matrix_event_index.h
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

#ifndef __MATRIX_EVENT_INDEX_H
#define __MATRIX_EVENT_INDEX_H


#include <stdint.h>
#include <stdbool.h>


/**
  * @brief  The Matrix event index data object.
  */
typedef struct
{
	//	event index for determining the event order
	uint16_t eventIndex;
	
} MATRIX_EVENT_INDEX_OBJECT;
//extern MATRIX_EVENT_INDEX_OBJECT MatrixEventIndex;


/**
  * @brief  Resets the event index.
  * @param  None.
  * @retval None.
  */
extern void MatrixEventIndex_Reset(void);

/**
  * @brief  Returns the event index.
	*					This method is called by the matrix transmitter and should
	*					not normally be called by other modules.
	* @param  None.
  * @retval The event index.
  */
extern uint8_t Matrix_GetEventIndex(void);

/**
  * @brief  Bumps the event index.
	*					This method is called by the matrix transmitter and should
	*					not normally be called by other modules.
	* @param  None.
  * @retval The event index.
  */
extern void Matrix_NextEventIndex(void);

/**
  * @brief  Handles a new received event index.
	*					This method is called by the matrix receiver and should
	*					not normally be called by other modules.
	* @param  The event index.
  * @retval None.
  */
extern void Matrix_NewEventIndex(uint8_t index);

/**
  * @brief  Returns a value indicating whether the given event index has expired.
	*					This method is called by the matrix receiver and should
	*					not normally be called by other modules.
	* @param  The event index.
  * @retval None.
  */
extern bool Matrix_IsEventIndexExpired(uint8_t index);


#endif  //  __MATRIX_EVENT_INDEX_H

