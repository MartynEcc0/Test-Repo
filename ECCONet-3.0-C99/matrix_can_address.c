/**
  ******************************************************************************
  * @file    		matrix.c
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

#include <string.h>
#include "matrix.h"
#include "matrix_config.h"
#include "matrix_flash_drive.h"
#include "matrix_can_address.h"

//	private methods
extern int Matrix_PrivateSendCanToken(TOKEN *token);
uint8_t MatrixCanAddress_GetNextProposedCanAddress(void);
int MatrixCanAddress_SendAddressNegotiationToken(TOKEN *token);


/**
  * @brief  The Matrix CAN address data object.
  */
MATRIX_CAN_ADDRESS_OBJECT MatrixCanAddress;


/**
  * @brief  Resets and configures the Matrix CAN address mechanism.
  * @param  None.
  * @retval None.
  */
void MatrixCanAddress_Reset(void)
{
	TOKEN token;
	uint32_t timestamp;
	
	//	The device CAN address may be programmed as a static via GUI
	//	and stored in the flash file system.
	//	
	//	If the CAN address file does not exist, then set the address to zero
	//	and isStatic to false in order to generate a self-assigned address.
	//
	if (0 != FlashDrive_ReadFile(MATRIX_CAN_ADDRESS_FILE_VOLUME_INDEX, MATRIX_CAN_ADDRESS_FILE_NAME,
		&MatrixCanAddress.file.address, sizeof(MATRIX_CAN_ADDRESS_FILE_OBJECT), &timestamp))
	{
		MatrixCanAddress.file.address = 0;
		MatrixCanAddress.file.isStatic = 0;
	}

	//	force static test	
	//MatrixCanAddress.file.address = 0x31;
	//MatrixCanAddress.file.isStatic = 1;
	
	//	init self-address mechanism
	MatrixCanAddress.addressOffset = 0;
	MatrixCanAddress.xorIndex = 0;
	MatrixCanAddress.proposedAddress = 0;
	
	//	if address is static, then send message to system
	if (MatrixCanAddress.file.isStatic)
	{
		//	notify other devices in the system that the address is in use
		token.key = KeyResponseAddressInUse;
		token.value = MatrixCanAddress.file.address;
		token.address = 0;
		Matrix_PrivateSendCanToken(&token);
	}
}

/**
  * @brief  Clocks the Matrix CAN address mechanism.
	*					This method supports cooperative task scheduling.
  * @param  None.
  * @retval None.
  */
void MatrixCanAddress_Clock(void)
{
	TOKEN token;
	
	//	if CAN address is not valid
	if (!Matrix_IsCanAddressValid())
	{
		//	if have not proposed an address
		if (0 == MatrixCanAddress.proposedAddress)
		{
			//	get and send next proposed address
			MatrixCanAddress.proposedAddress =
				MatrixCanAddress_GetNextProposedCanAddress();
			token.key = KeyRequestAddress;
			token.value = MatrixCanAddress.proposedAddress;
			token.address = 0;
			Matrix_PrivateSendCanToken(&token);
			
			//	claim address in 100 mS
			MatrixCanAddress.requestTime = Matrix.systemTime + 100;
		}
	
		//	else if proposed an address and did not received an address-in-use message
		//	before the 100 mS timer expired
		else if (IsMatrixTimerExpired(MatrixCanAddress.requestTime))
		{
			//	adopt address
			MatrixCanAddress.file.address = MatrixCanAddress.proposedAddress;
			MatrixCanAddress.proposedAddress = 0;

			//	notify other devices in the system that the address is in use
			token.key = KeyResponseAddressInUse;
			token.value = MatrixCanAddress.file.address;
			token.address = 0;
			Matrix_PrivateSendCanToken(&token);
			
			//	send the first status update in 1200 mS
			Matrix.nextStatusTime = Matrix.systemTime + 1200;
		}
	}
}

/**
  * @brief  Checks incoming tokens and senders for self-address mechanism.
	* @param  token: A message token.
  * @retval None.
  */
void MatrixCanAddress_CanTokenIn(TOKEN token)
{
	//	if received an address-in-use message while proposing the same address
	//	
	//	or if received any message from a sender whose CAN address matches this device's
	//	working address and this device's address is not static
	//	
	//	then in either case restart the self-addressing mechanism
	//	
	if (((token.key == KeyResponseAddressInUse) && (token.value == MatrixCanAddress.proposedAddress)) 
		|| ((0 != MatrixCanAddress.file.address) && (MatrixCanAddress.file.address == token.address)
		&&	!MatrixCanAddress.file.isStatic))
	{
		MatrixCanAddress.file.address = 0;
		MatrixCanAddress.proposedAddress = 0;
	}
	
	//	if another devide is proposing to use an address that matches
	//	this device's address, then send out an address-in-use response
	else if ((token.key == KeyRequestAddress) &&
		(token.value == MatrixCanAddress.file.address))
	{
		token.key = KeyResponseAddressInUse;
		token.value = MatrixCanAddress.file.address;
		token.address = 0;
		Matrix_PrivateSendCanToken(&token);
	}
}

/**
  * @brief  Gets the device CAN address.
	* @param  None.
  * @retval The device CAN address.
  */
uint8_t Matrix_GetCanAddress(void)
{
	return MatrixCanAddress.file.address;
}

/**
  * @brief  Returns a value indicating whether this device CAN address is static.
	* @param  None.
  * @retval True if the address is static.
  */
bool Matrix_IsCanAddressStatic(void)
{
	return (0 != MatrixCanAddress.file.isStatic);
}

/**
  * @brief  Returns a value indicating whether this device CAN address is valid.
	*					An address is valid if it static or is in the range 1 to 120.
	* @param  None.
  * @retval True if the address is static.
  */
bool Matrix_IsCanAddressValid(void)
{
	return (((MatrixCanAddress.file.address >= 1) && (MatrixCanAddress.file.address <= 120))
		|| (0 != MatrixCanAddress.file.isStatic));
}


//	private methods..................................................

/**
  * @brief  Gets the next address to propose.
	* @param  None.
  * @retval The next address to propose.
  */
uint8_t MatrixCanAddress_GetNextProposedCanAddress(void)
{
	uint8_t *pGuid;
	uint16_t i;
	uint32_t address, xorValue, guid[4];

	//	number of address bits and mask
	const uint32_t numBits = MATRIX_CAN_ID_ADDRESS_BIT_WIDTH;
	const uint32_t mask = MATRIX_CAN_ID_ADDRESS_MASK;
	
	//	get the GUID
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->get128BitGuid))
	{
		Matrix.appInterface->get128BitGuid(guid);
	}
	else  //  no guid available
	{
		guid[0] = 0xEE4CAD97;
		guid[1] = 0x331CE9EC;
		guid[2] = 0x9E957DBC;
		guid[3] = 0xA4A69FE5;
	}

	do
	{
		//	clear address
		address = 0;
		
		//	get the current xor value
		xorValue = ((uint32_t)DEVICE_ADDRESS_XOR_VALUE >> MatrixCanAddress.xorIndex) |
			(((uint32_t)DEVICE_ADDRESS_XOR_VALUE << (numBits - MatrixCanAddress.xorIndex)) & mask);

		//	run algorithm
		pGuid = (uint8_t *)guid;
		for (i = 0; i < 16; i++)
			address += (*pGuid++ ^ xorValue); 
		address += MatrixCanAddress.addressOffset;
		address &= mask;
		
		//	bump the xor index, and if rolling over, then bump the address offset
		if (++MatrixCanAddress.xorIndex >= numBits)
		{
			MatrixCanAddress.xorIndex = 0;
			++MatrixCanAddress.addressOffset;
			MatrixCanAddress.addressOffset &= mask;
		}
	}	while (!address || (address > 120));

	//	return the address
	return address;
}


#ifdef PREVIOUS_XOR_LOOP

		for (i = 0; i < 19; i++)
		{
			//	shift the 128-bit guid
			guid[0] = (guid[1] << (32 - numBits)) | (guid[0] >> numBits);
			guid[1] = (guid[2] << (32 - numBits)) | (guid[1] >> numBits);
			guid[2] = (guid[3] << (32 - numBits)) | (guid[2] >> numBits);
			guid[3] >>= numBits;
			
			//	add the lower 7 bits to the value accumulator
			address += ((guid[0] & mask) ^ xorValue);
		}

#endif  //


#ifdef UNUSED_CODE
/**
  * @brief  Sends a token over the CAN bus.
	* @param  token: The token to send.
	* @param  isNewEvent: True if sending a new event token.
  * @retval Returns zero on success, else -1.
  */
int MatrixCanAddress_SendAddressNegotiationToken(TOKEN *token)
{
	//	validate inputs
	if (NULL == token)
		return -1;
	
	//	initialize the transmitter
	MatrixTransmitter_StartMessageWithAddressAndKey(token->address, token->key);
	
	//	add the token to the transmit fifo
	MatrixTransmitter_AddToken(token);
	
	//	finish the message
	MatrixTransmitter_FinishMessage();
	
	return 0;
}
#endif


		
		
