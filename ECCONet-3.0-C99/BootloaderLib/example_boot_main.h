/**
  ******************************************************************************
  * @file    		example_boot_main.h
  * @copyright      © 2015 ESG.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		June 2015
  * @brief   		Main program starting point for bootloader.
  ******************************************************************************
  * @attention
  *
  * Unless required by applicable law or agreed to in writing, software created
  * by Liquid Logic, LLC is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
  *	OR CONDITIONS OF ANY KIND, either express or implied.
  *
  ******************************************************************************
  */

#ifndef __BOOT_MAIN_H
#define __BOOT_MAIN_H

#include <stdint.h>
#include <stdbool.h>


//	processor
#define __PROCESSOR  __LPC11xx

//	the CAN message object indices
#define CAN_RX_MAILBOX_INDEX 	1
#define CAN_TX_MAILBOX_INDEX 	20



#endif  //  __BOOT_MAIN_H



