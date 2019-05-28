/**
  ******************************************************************************
  * @file    		bootloader.h
  * @copyright  © 2018 ECCO Group.  All rights reserved.
  * @author  		M. Latham, Liquid Logic, LLC
  * @version 		1.0.0
  * @date    		Feb 2018
  * @brief   		The bootloader main.
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

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bootloader_interface.h"


//	flash write message
//
//	item    bytes     function
//  ======================================
//  1       1         event index					
//  2       2         token
//  3       4         access code
//  4       31        model name
//  5       4         data location
//  6       2         data size
//  7       256 max   data
//  8       2         data checksum
//  9       2         message checksum



#define IsBootloaderTimerExpired(x) (0 <= (int32_t)(Bootloader.systemTime - (x)))


//	some handy macros
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? a : b)
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? a : b)
#endif

#ifndef ABS
#define ABS(a) (((a) < 0) ? -(a) : a)
#endif


/**
  * @brief  The bootloader data object.
  */
typedef struct
{
	//	system time provided by app on clock calls
	uint32_t systemTime;
	
	//	output beacon timer
	uint32_t nextBeaconTime;
	
	//	app interface structure
	const BOOTLOADER_INTERFACE_TABLE *appInterface;
	
	//	atomic flag for process lock
	uint8_t busy;
	
	
} BOOTLOADER_OBJECT;
extern BOOTLOADER_OBJECT Bootloader;




#endif  //  __BOOTLOADER_H

