/**
  ******************************************************************************
  * @file    		matrix.h
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		April 2017
  * @brief   		The Matrix library for communication between
	*							the CAN bus and the internal message system.
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

#ifndef __MATRIX_H
#define __MATRIX_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "matrix_config.h"
#include "matrix_lib_interface.h"
#include "matrix_transmitter.h"
#include "matrix_receiver.h"

//#define TEST_CALCS
#define IsMatrixTimerExpired(x) (0 <= (int32_t)(Matrix.systemTime - (x)))


/**
  * @brief  The Matrix interface data object.
  */
typedef struct
{
	//	system time provided by app on clock calls
	uint32_t systemTime;
	
	//	output status timer
	uint32_t nextStatusTime;
	
	//	app interface structure
	const MATRIX_INTERFACE_TABLE *appInterface;
	
	//	atomic flag for process lock
	uint8_t busy;
	
	
} MATRIX_OBJECT;
extern MATRIX_OBJECT Matrix;



#endif  //  __MATRIX_H

