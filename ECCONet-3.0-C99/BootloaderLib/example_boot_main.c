/**
  ******************************************************************************
  * @file    		example_boot_main.c
  * @copyright      © 2018 ECCO Safety Group.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		Feb 2018
  * @brief   		Main program starting point for bootloader.
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

#include <stdint.h>
#include <stdbool.h>
#include "can_api_small.h"
#include "flash_api_small.h"
#include "timebase_small.h"
#include "flash_image.h"
#include "bootloader_interface.h"
#include "boot_main.h"



//	FLASH API reserved bytes
uint32_t FlashAPI_Reserved[8] __attribute__((at(0x10001FE0)));
int main(void);

/**
  * @brief  The product.inf struct.
  */
const BOOTLOADER_PRODUCT_INFO_STRUCT ProductInfo =
{
	.modelName = "SIB",
	.manufacturerName = "Code 3",
	.hardwareRevision = "E",
	.appFirmwareRevision = "btldr",  //  indicate bootloader
	.bootloaderFirmwareRevision = "1.00",
	.baseLightheadEnumeration = "n/a",
	.maxLightheadEnumeration = "n/a",
};

/**
  * @brief  Bootloader library interface table.
  */
const BOOTLOADER_INTERFACE_TABLE BootloaderInterface_Table =
{
	.canAddressStruct = (void *)BOOT_CAN_ADDRESS_REGISTER,
	.productInfoStruct = &ProductInfo,
	.appFlashAddress = APP_FLASH_START_ADDRESS,
	.appFlashSize = APP_FLASH_SIZE,
	.sendCanFrame = CanApi_SendOn20,
	.flashWrite = FlashApi_WriteWithErase256,
	.reboot = main,
	.get128BitGuid = FlashApi_ReadUniqueID,
};

/**
  * @brief  Main bootloader application.
  *
  *         NOTE:  The normal (non-bootloader) app may set two general-purpose registers with the boot request key
  *                and the current ECCONet CAN address in static address mode, and then invoke this bootloader.
  *
  *                When the bootloader boots it checks these registers, and if set with the proper
  *                keys, remains in bootloader mode with the assigned static CAN address.
  *
  *                That allows the user to enter and stay in bootloader mode while the normal app is running.
  *
  * @param  None.
  * @retval	Never returns.
  */
int main(void)
{
	//	if warm boot from app
	if (*((uint32_t *)BOOT_CONTROL_REGISTER) == BOOT_CONTROL_KEY)
	{
		//	clear app request
		*((uint32_t *)BOOT_CONTROL_REGISTER) = 0;
	}
	else  //  cold boot
	{
		//	if the app CRC is good then proceed to application
		if (Bootloader_ComputeCRC32((uint8_t*)APP_FLASH_START_ADDRESS, APP_FLASH_SIZE - 4) ==
			*(volatile uint32_t *)(APP_FLASH_START_ADDRESS + APP_FLASH_SIZE - 4))
		{
			((void (*)(void))(APP_FLASH_START_ADDRESS + 1))();
		}
	
		//	else let the bootloader assign its own address
		*((uint32_t *)BOOT_CAN_ADDRESS_REGISTER) = 0;
	}
	
	//	reset the hardware
	Timebase_Reset();
	CanApi_Reset();

	//	intialize the bootloader lib
	Bootloader_Reset(&BootloaderInterface_Table, Timebase.Time);
		
	//	while power
	while(1)
	{
		//	clock the bootloader
		Bootloader_Clock(Timebase.Time);
	}
}

/**
  * @brief  CAN interrupt service routine.
  * @param  None.
  * @retval None.
  */
void CAN_IRQHandler(void)
{
	ENET_CAN_FRAME frame;

	//	<< YOUR CODE HERE >>


	//	send frame to the message handler
	Bootloader_ReceiveCanFrame(&frame);
}

