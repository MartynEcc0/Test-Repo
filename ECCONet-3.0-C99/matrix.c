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
#include "matrix_token_sequencer.h"
#include "matrix_time_logic.h"
#include "matrix_codec.h"
#include "matrix_can_address.h"
#include "matrix_ftp_client.h"
#include "matrix_ftp_server.h"
#include "matrix_event_index.h"
#include "matrix_lib_interface.h"
#include "matrix.h"


//	private methods
extern MATRIX_RECEIVER MatrixReceiver;
int Matrix_PrivateSendCanToken(TOKEN *token);
void Matrix_SendUpdateCheckAck(uint32_t address);
void Matrix_DelayStatusUpdate15mS(void);


//	the matrix interface data object
MATRIX_OBJECT Matrix;

/**
  * @brief  Resets and configures the Matrix library.
	* @param  appInterface: The application interface table.
	* @param  systemTime: The 32-bit system time in milliseconds.
  */
void Matrix_Reset(const MATRIX_INTERFACE_TABLE *appInterface, uint32_t systemTime)
{
	//	save the application interface
	Matrix.appInterface = appInterface;
	
	//	set the system time and the next status time
	Matrix.systemTime = systemTime;
	Matrix.nextStatusTime = systemTime + 1200;

	//	init Matrix modules
	MatrixEventIndex_Reset();
	MatrixTimeLogic_Reset(MATRIX_TIME_LOGIC_FILE_NAME_0);
	MatrixReceiver_Reset();
	MatrixTransmitter_Reset();
	MatrixCanAddress_Reset();
	MatrixFTPClient_Reset();
	MatrixFTPServer_Reset();
	MatrixTokenSequencerController_Reset();
	
	//	clear the process lock
	Matrix.busy = false;
}

/**
  * @brief  Clocks the matrix library.
	*					This method supports cooperative task scheduling (round-robin).
	* @param  systemTime: The 32-bit system time in milliseconds.
  * @retval None.
  */
void Matrix_Clock(uint32_t systemTime)
{
	//	check for clock process complete
	if (Matrix.busy)
		return;
	Matrix.busy = true;
	
	//	set the system time
	Matrix.systemTime = systemTime;
	
	//	clock matrix modules
	MatrixReceiver_Clock();
	MatrixTransmitter_Clock();
	MatrixCanAddress_Clock();
	MatrixTimeLogic_Clock();
	MatrixFTPServer_Clock();
	MatrixFTPClient_Clock();
	MatrixTokenSequencerController_Clock();

	//	if time to send status and status tokens to send 
	//	and not running ftp and CAN address is valid
	if (IsMatrixTimerExpired(Matrix.nextStatusTime) /*&& MatrixTimeLogic.tokenTableHasBroadcastTokens*/ 
		&& (0 == MatrixReceiver.senderAddressFilter) && Matrix_IsCanAddressValid())
	{
		//	set next message time
		Matrix.nextStatusTime += (Matrix_GetCanAddress() + (1000 - 60));
		
		//	start the message
		MatrixTransmitter_StartMessage(CAN_BROADCAST_ADDRESS);
		
		//	compress the tokens
		MatrixCodec_Compress(MatrixTimeLogic_TokenTable.tokens, MatrixTimeLogic_TokenTable.numTokens,
			&MatrixTransmitter_AddByte);
		
		//	finish the message
		MatrixTransmitter_FinishMessage();		
	}
	
	//	clear busy status
	Matrix.busy = false;
}

/**
  * @brief  Handles incoming tokens from the application.
	* @param  token: A pointer to a token.
  * @retval None.
  */
void Matrix_TokenIn(TOKEN *token)
{
	//	if application is sending token to equation processor
	if (MATRIX_EQUATION_PROCESSOR_NETWORK_ADDRESS == token->address)
		MatrixTimeLogic_TokenIn(token);
	
	//	else if application is sending token to token sequencer
	else if ((MATRIX_TOKEN_SEQUENCER_0_NETWORK_ADDRESS <= token->address)
		&& (MATRIX_TOKEN_SEQUENCER_5_NETWORK_ADDRESS >= token->address))
		MatrixTokenSequencerController_TokenIn(token);

	//	else if sending public token and this device's CAN address is valid,
	//	then send token to CAN bus
	else if ((128 > token->address) && !Key_IsLocalVariable(token->key) && Matrix_IsCanAddressValid())
		Matrix_PrivateSendCanToken(token);
}


//	private methods....................................................................

/**
  * @brief  Sends a token over the CAN bus.
	* @param  token: The token to send.
  * @retval Returns zero on success, else -1.
  */
int Matrix_PrivateSendCanToken(TOKEN *token)
{
	int i;
	bool isInputEvent;
	
	//	validate token and device address
	if ((NULL == token) 
		|| (!Matrix_IsCanAddressValid() && (token->key != KeyRequestAddress)))
		return -1;

	//	get input event status
	isInputEvent = (KeyPrefix_InputStatus == Key_GetPrefix(token->key));

	//	if input event
	if (isInputEvent)
	{
		//	bump the event index and push status message up to 100 mS
		Matrix_NextEventIndex();
		Matrix_DelayStatusUpdate15mS();
	}
	
	//	send the token once, or three times for input events
	for (i = 0; i < (isInputEvent ? 3 : 1); ++i)
	{
		//	initialize the transmitter
		MatrixTransmitter_StartMessageWithAddressAndKey(token->address, token->key);
		
		//	add the token to the transmit fifo
		MatrixTransmitter_AddToken(token);
		
		//	finish the message
		MatrixTransmitter_FinishMessage();
	}
	return 0;
}

/**
  * @brief  Sends a sync token over the CAN bus.
	* @param  token: The token to send.
  * @retval Returns zero on success, else -1.
  */
int Matrix_SendSync(TOKEN *token)
{
	//	validate inputs
	if (NULL == token)
		return -1;
	
	//	initialize the transmitter
	//	all syncs are to broadcast address
	MatrixTransmitter_StartMessage(0);

	//	send the pattern enumeration with the pattern sync prefix
	MatrixTransmitter_AddByte(KeyPrefix_PatternSync
		| ((token->value >> 8) & ~KeyPrefix_PatternSync));
	MatrixTransmitter_AddByte(token->value);
	
	//	finish the message
	MatrixTransmitter_FinishMessage();
	
	return 0;
}

/**
  * @brief  Delays the status update by up to 15 mS from original next scheduled time.
  *         Note: This does *not* delay input or output events. 
	* @param  None.
  * @retval None.
  */
void Matrix_DelayStatusUpdate15mS(void)
{
	if (15 > (int32_t)(Matrix.nextStatusTime - Matrix.systemTime))
		Matrix.nextStatusTime += 15;
}

/**
  * @brief  Handles incoming non-ftp tokens from the matrix receiver.
	*					Note that this was written for a cooperative multitasking enviroment
	*					and thus TokenIn and ManageCanAddress are sequential, not concurrent.
	*
	* @param  token: The received token.
  * @retval None.
  */
void Matrix_PrivateReceiveCanToken(TOKEN *token)
{
	uint8_t prefix;
	
	//	manage address before handling other types of tokens
	MatrixCanAddress_CanTokenIn(*token);
	
	//	if this device's working CAN address is valid
	if (Matrix_IsCanAddressValid())
	{
		//	get key prefix
		prefix = Key_GetPrefix(token->key);
		
		//	all status tokens go to the time logic controller
		if ((KeyPrefix_InputStatus == prefix)
			|| (KeyPrefix_OutputStatus == prefix))
			MatrixTimeLogic_TokenIn(token);
		
		//	all command tokens go the sequencer 
		if (KeyPrefix_Command == prefix)
			MatrixTokenSequencerController_TokenIn(token);
		
		//	all tokens go to the application
		if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->tokenCallback))
			 Matrix.appInterface->tokenCallback(token);
	}
}



#ifdef CODEC_TEST

/**
  * @brief  Handles tokens from the matrix receiver.
	* @param  token: The received token.
	* @param  senderAddress: The CAN address of the sender.
  * @retval None.
  */
void Matrix_CanTokenInTest(TOKEN token, uint8_t senderAddress)
{

	if (Device_ReceiveIndex < (DEVICE_NUM_TOKENS * 3))
	{
		Device_Receive[Device_ReceiveIndex].Key = token.key;
		Device_Receive[Device_ReceiveIndex].Value = token.value;
		Device_Receive[Device_ReceiveIndex].SenderAddress = senderAddress;
	}
	++Device_ReceiveIndex;
	
	if (Device_ReceiveIndex >= (DEVICE_NUM_TOKENS * 3))
		Device_ReceiveIndex = 100;
}

typedef struct
{
	//	source address
	uint16_t SenderAddress;
	
	//	the universal light code, 16 bits on CAN bus
	uint16_t Key;
	
	//	the value associated with the code, 32 bits on CAN bus
	uint32_t Value;
	
} TEST_TOKEN;

#define DEVICE_NUM_TOKENS 10
const TOKEN Device_TransmitPresets[DEVICE_NUM_TOKENS] = 
{
	{ 0, 0, KeyBaseIndexedLight +  0,   0 },
	{ 0, 0, KeyBaseIndexedLight +  2,  30 },
	{ 0, 0, KeyBaseIndexedLight +  3,   0 },
	{ 0, 0, KeyBaseIndexedLight +  5,   0 },
	{ 0, 0, KeyBaseIndexedLight +  7,  30 },
	{ 0, 0, KeyBaseIndexedLight +  6,  30 },
	{ 0, 0, KeyBaseIndexedLight +  4,  30 },
	
	{ 0, 0, KeyLeftTurnLight,         100 },
	{ 0, 0, KeyTailLight,              90 },
	{ 0, 0, KeyStopLight,              80 },
};

TOKEN Device_Transmit[DEVICE_NUM_TOKENS];
TEST_TOKEN Device_Receive[DEVICE_NUM_TOKENS * 3];
uint32_t Device_ReceiveIndex;



void Matrix_Debug(uint16_t light)
{
	TOKEN token;
	
	token.address = 0;
	token.key = light;
	token.value = 10;
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->tokenCallback))
		 Matrix.appInterface->tokenCallback(&token);
}

#endif


#if 0	//<<--test 
	for (i = 0; i < DEVICE_NUM_TOKENS; i++)
		Device_Transmit[i] = Device_TransmitPresets[i];
	
	//	reset transmitter and receiver
	MatrixTransmitter_Reset();
	MatrixReceiver_Reset();
	
	//	test
	Device_ReceiveIndex = 0;
	
	Matrix.CAN_Address = 1;
	MatrixTransmitter_SendMessage(0, Device_Transmit, DEVICE_NUM_TOKENS, false);
	Matrix.CAN_Address = 40;
	MatrixTransmitter_SendMessage(0, Device_Transmit, DEVICE_NUM_TOKENS, false);
	Matrix.CAN_Address = 120;
	MatrixTransmitter_SendMessage(0, Device_Transmit, DEVICE_NUM_TOKENS, false);
#endif	




			
