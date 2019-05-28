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

#include <string.h>
#include "can_address.h"
#include "receiver.h"
#include "transmitter.h"
#include "encryption.h"
#include "bootloader.h"


/**
  * @brief  The bootloader data object.
  */
BOOTLOADER_OBJECT Bootloader;


/**
  * @brief  Resets and configures the bootloader.
	* @param  appInterface: The application interface table.
	* @param  systemTime: The 32-bit system time in milliseconds.
  */
void Bootloader_Reset(const BOOTLOADER_INTERFACE_TABLE *appInterface, uint32_t systemTime)
{
	//	save the application interface
	Bootloader.appInterface = appInterface;

	//	if have application interface
	if (NULL != Bootloader.appInterface)
	{
		//	set the system time and the next status time
		Bootloader.systemTime = systemTime;
		Bootloader.nextBeaconTime = systemTime + 1200;

		//	init Bootloader modules
		Encryption_Reset();
		Receiver_Reset();
		Transmitter_Reset();
		CanAddress_Reset();
	}
	
	//	clear the process lock
	Bootloader.busy = false;
}

/**
  * @brief  Clocks the bootloader.
	*					This method supports cooperative task scheduling (round-robin).
	* @param  systemTime: The 32-bit system time in milliseconds.
  * @retval None.
  */
void Bootloader_Clock(uint32_t systemTime)
{
	//	if app interface is null, then don't clock
	if (NULL == Bootloader.appInterface)
		return;
	
	//	check for clock process complete
	if (Bootloader.busy)
		return;
	Bootloader.busy = true;
	
	//	set the system time
	Bootloader.systemTime = systemTime;
	
	//	clock the modules
	Receiver_Clock();
	CanAddress_Clock();

	//	if time to send status and CAN address is valid 
	if (IsBootloaderTimerExpired(Bootloader.nextBeaconTime)
		&& Bootloader_IsCanAddressValid())
	{
		//	set next message time
		Bootloader.nextBeaconTime += (Bootloader_GetCanAddress() + (1000 - 60));
		
		//	send the beacon
		Transmitter_StartMessage(ECCONET_CAN_BROADCAST_ADDRESS);
		Transmitter_Finish();
	}
	
	//	clear busy status
	Bootloader.busy = false;
}

