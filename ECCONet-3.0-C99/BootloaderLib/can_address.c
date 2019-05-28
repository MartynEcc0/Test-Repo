/**
  ******************************************************************************
  * @file    		can_address.c
  * @copyright  © 2017-2018 ECCO Safety Group.  All rights reserved.
  * @author  		M. Latham
  * @version 		1.0.0
  * @date    		Feb 2018
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

#include <string.h>
#include "ecconet.h"
#include "bootloader_interface.h"
#include "bootloader.h"
#include "encryption.h"
#include "transmitter.h"
#include "can_address.h"


//	Exclusive-OR value for creating device address.
#define DEVICE_ADDRESS_XOR_VALUE			0x64


//	private methods
static uint8_t GetNextProposedAddress(void);


/**
  * @brief  The Matrix CAN address data object.
  */
CAN_ADDRESS_OBJECT CanAddress;


/**
  * @brief  Resets and configures the Matrix CAN address mechanism.
  * @param  None.
  * @retval None.
  */
void CanAddress_Reset(void)
{
	//TOKEN token;
	
	//	get CAN address and mode
	if (NULL != Bootloader.appInterface->canAddressStruct)
	{
		CanAddress.addressMode.address = Bootloader.appInterface->canAddressStruct->address;
		CanAddress.addressMode.isStatic = Bootloader.appInterface->canAddressStruct->isStatic;
	}
	else  //  app did not provide
	{
		CanAddress.addressMode.address = 0;
		CanAddress.addressMode.isStatic = 0;
	}
	
	//	init self-address mechanism
	CanAddress.addressOffset = 0;
	CanAddress.xorIndex = 0;
	CanAddress.proposedAddress = 0;

#if 0  //  beacon takes care of this	
	//	if address is static, then send message to system
	if (CanAddress.addressMode.isStatic)
	{
		//	notify other devices in the system that the address is in use
		token.key = KeyResponseAddressInUse;
		token.value = CanAddress.addressMode.address;
		token.address = 0;
		Transmitter_SendToken(&token, 1); 
	}
#endif
}

/**
  * @brief  Clocks the Matrix CAN address mechanism.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
void CanAddress_Clock(void)
{
	TOKEN token;
	
	//	if CAN address is not valid
	if (!Bootloader_IsCanAddressValid())
	{
		//	if have not proposed an address
		if (0 == CanAddress.proposedAddress)
		{
			//	get and send next proposed address
			CanAddress.proposedAddress = GetNextProposedAddress();
			token.key = KeyRequestAddress;
			token.value = CanAddress.proposedAddress;
			token.address = 0;
			Transmitter_SendToken(&token, 1);
			
			//	claim address in 100 mS
			CanAddress.requestTime = Bootloader.systemTime + 100;
		}
	
		//	else if proposed an address and did not received an address-in-use message
		//	before the 100 mS timer expired
		else if (IsBootloaderTimerExpired(CanAddress.requestTime))
		{
			//	adopt address
			CanAddress.addressMode.address = CanAddress.proposedAddress;
			CanAddress.proposedAddress = 0;

			//	notify other devices in the system that the address is in use
			token.key = KeyResponseAddressInUse;
			token.value = CanAddress.addressMode.address;
			token.address = 0;
			Transmitter_SendToken(&token, 1);
			
			//	start the beacon timer
			Bootloader.nextBeaconTime = Bootloader.systemTime + 1200;
		}
	}
}

/**
  * @brief  Checks incoming tokens and senders for self-address mechanism.
	* @param  token: A message token.
  * @retval None.
  */
void CanAddress_TokenIn(TOKEN *token)
{
	TOKEN tk;
	
	//	if received an address-in-use message while proposing the same address
	//	
	//	or if received any message from a sender whose CAN address matches this device's
	//	working address and this device's address is not static
	//	
	//	then in either case restart the self-addressing mechanism
	//	
	if (((token->key == KeyResponseAddressInUse) && (token->value == CanAddress.proposedAddress)) 
		|| ((0 != CanAddress.addressMode.address) && (CanAddress.addressMode.address == token->address)
		&&	!CanAddress.addressMode.isStatic))
	{
		CanAddress.addressMode.address = 0;
		CanAddress.proposedAddress = 0;
	}
	
	//	if another devide is proposing to use an address that matches
	//	this device's address, then send out an address-in-use response
	else if ((token->key == KeyRequestAddress) &&
		(token->value == CanAddress.addressMode.address))
	{
		tk.address = 0;
		tk.key = KeyResponseAddressInUse;
		tk.value = CanAddress.addressMode.address;
		Transmitter_SendToken(&tk, 1);
	}
}


/**
  * @brief  Gets the device CAN address.
	* @param  None.
  * @retval The device CAN address.
  */
uint8_t Bootloader_GetCanAddress(void)
{
	return CanAddress.addressMode.address;
}

/**
  * @brief  Returns a value indicating whether this device CAN address is valid.
	*					An address is valid if it static or is in the range 1 to 120.
	* @param  None.
  * @retval True if the address is static.
  */
bool Bootloader_IsCanAddressValid(void)
{
	return (((CanAddress.addressMode.address >= 1) && (CanAddress.addressMode.address <= 120))
		|| (0 != CanAddress.addressMode.isStatic));
}


//	private methods..................................................


/**
  * @brief  Gets the next address to propose.
	* @param  None.
  * @retval The next address to propose.
  */
static uint8_t GetNextProposedAddress(void)
{
	uint8_t *pGuid;
	uint16_t i;
	uint32_t address, xorValue;

	//	number of address bits and mask
	const uint32_t numBits = ENET_CAN_ID_ADDRESS_BIT_WIDTH;
	const uint32_t mask = ENET_CAN_ID_ADDRESS_MASK;
	
	do
	{
		//	clear address
		address = 0;

		//	get the current xor value
		xorValue = ((uint32_t)DEVICE_ADDRESS_XOR_VALUE >> CanAddress.xorIndex) |
			(((uint32_t)DEVICE_ADDRESS_XOR_VALUE << (numBits - CanAddress.xorIndex)) & mask);

		//	run algorithm
		pGuid = (uint8_t *)Encryption.deviceGuid;
		for (i = 0; i < 16; i++)
			address += (*pGuid++ ^ xorValue); 
		address += CanAddress.addressOffset;
		address &= mask;
		
		//	bump the xor index, and if rolling over, then bump the address offset
		if (++CanAddress.xorIndex >= numBits)
		{
			CanAddress.xorIndex = 0;
			++CanAddress.addressOffset;
			CanAddress.addressOffset &= mask;
		}
	}	while (!address || (address > 120));

	//	return the address
	return address;
}
