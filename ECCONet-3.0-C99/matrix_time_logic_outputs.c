/**
  ******************************************************************************
  * @file       matrix_time_logic_outputs.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author     M. Latham, Liquid Logic
  * @version    1.0.0
  * @date       April 2017
	*
  * @brief      Processes human-readable calculator output options to	generate
	*							addition logic, and value-based and time-based output tokens.
	*
  ******************************************************************************
  * @attention
  * Unless required by applicable law or agreed to in writing, software
  * created by Liquid Logic LLC is delivered "as is" without warranties
  * or conditions of any kind, either express or implied.
  *
  ******************************************************************************
  */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "matrix.h"
#include "matrix_token_sequencer.h"
#include "matrix_time_logic.h"



//	private methods
static void SendToken(TOKEN *token);
extern int Matrix_PrivateSendCanToken(TOKEN *token);
extern MTL_TOKEN *MatrixTimeLogic_TokenTable_TokenFromBitcode(uint8_t **bitcodeRef);




/**
  * @brief  Parses a logic file output options.
  * @param  ptr: A pointer to an equation equals operator.
  * @param  lastPtr: A pointer to the byte following the end of the file.
	* @param  calculatedValue: A new value for the output token.
	* @param  firstToken: A pointer to the first token in the left-hand expression, or NULL if none.
  * @retval Returns 0 on success, else -1.
  */
int MTL_ProcessOutputOptions(uint8_t **bitcodeRef, uint8_t *lastPtr, int32_t calculatedValue, uint8_t *firstToken)
{
	TOKEN token;
	MTL_TOKEN *tableToken, *clearToken;
	bool prevBitState, currentBitState, outputRisingEdge, outputFallingEdge;
	uint8_t *ptr;
	int32_t maxCount;
	
	//	validate inputs
	ptr = *bitcodeRef;
	if ((NULL == ptr) || (NULL == lastPtr))
		return -10;
	
	//	get local pointer and validate position in equation
	if ((Equals != *ptr) && (Lambda != *ptr))
		return -11;
	
	//	advance past equals or lambda and get right-hand output token from table
	++ptr;
	tableToken = MatrixTimeLogic_TokenTable_TokenFromBitcode(&ptr);
	if (NULL == tableToken)
		return -12;
	
	//	advance past equation end
	if (*++ptr != EquationEnd)
		return -13;

	//	preset token for send
	token.key = tableToken->token.key;
	token.address = tableToken->token.address;
	
	//	get previous and new bit state
	prevBitState = tableToken->token.flags & MtlFlagsInputBitstate;
	currentBitState = calculatedValue ? 1 : 0;
	outputRisingEdge = (!prevBitState && currentBitState);
	outputFallingEdge = (prevBitState && !currentBitState);
	
	//	if bit state changed, then timestamp the output token
	if (currentBitState != prevBitState)
		tableToken->timestamp = Matrix.systemTime; 
	
	
	//	for post-calculation functions
	while (++ptr < lastPtr)
	{
		//	if done with output options then break
		if ((EquationStart == *ptr) || (PriorityEquationStart == *ptr)
			||  (SuccessiveEquationStart == *ptr))
			break;
		
		switch (*ptr)
		{
			case OutputLogicActivityMonitor:
				//	get delay value, maximum of 60000 (60 seconds)
				++ptr;
				BitcodeInt32Value(maxCount, ptr);
				if (maxCount > 60000)
					maxCount = 60000;

				//	get left-hand expression token
				if (NULL != firstToken)
				{
					clearToken = MatrixTimeLogic_TokenTable_TokenFromBitcode(&firstToken);

					//	if have left-hand expression token (should always be the case)
					if (NULL != clearToken)
					{
						//	if token has been received since this equation was last evaluated
						if (clearToken->token.flags & MtlFlagsTokenReceived)
						{
							//	clear token received flag, timestamp output token and set calculated value
							clearToken->token.flags &= !MtlFlagsTokenReceived;
							tableToken->timestamp = Matrix.systemTime; 
							calculatedValue = 1;
						}
						else  //  not received since this equation was last evaluated
						{
							//	if specified time has elapsed with no token received, then the calculated value goes back to zero
							calculatedValue = (maxCount <= (uint16_t)((Matrix.systemTime - tableToken->timestamp) & 0x0000ffff)) ?
								0 : tableToken->token.value;
						}
					}
				}				
				break;
			
			case OutputLogicRisingEdgeUpCounter:
				//	get max count
				++ptr;
				BitcodeInt32Value(maxCount, ptr);

				//	if rising edge
				if (outputRisingEdge &&	!(tableToken->token.flags & MtlFlagsSkipToggle))
				{
					calculatedValue = tableToken->token.value + 1;
					if (calculatedValue >= maxCount)
						calculatedValue = 0;
				}
				else
					calculatedValue = tableToken->token.value;
				
				//	if high then clear skip toggle flag
				if (currentBitState)
					tableToken->token.flags &= ~MtlFlagsSkipToggle;
				break;
				
			case OutputLogicFallingEdgeUpCounter:
				//	get max count
				++ptr;
				BitcodeInt32Value(maxCount, ptr);

				//	if falling edge
				if (outputFallingEdge && !(tableToken->token.flags & MtlFlagsSkipToggle))
				{
					calculatedValue = tableToken->token.value + 1;
					if (calculatedValue >= maxCount)
						calculatedValue = 0;
				}
				else 
					calculatedValue = tableToken->token.value;

				//	if low then clear skip toggle flag
				if (!currentBitState)
					tableToken->token.flags &= ~MtlFlagsSkipToggle;
				break;

			case OutputLogicRisingEdgeToggle:				
				//	if bit changed from 0 to 1, and not skipping toggle
				if (outputRisingEdge && !(tableToken->token.flags & MtlFlagsSkipToggle))
					calculatedValue = tableToken->token.value ? 0 : 1;
				else
					calculatedValue = tableToken->token.value;
				
				//	if high then clear skip toggle flag
				if (currentBitState)
					tableToken->token.flags &= ~MtlFlagsSkipToggle;
				break;
		
			case OutputLogicFallingEdgeToggle:
				//	if bit changed from 1 to 0, and not skipping toggle
				if (outputFallingEdge && !(tableToken->token.flags & MtlFlagsSkipToggle))
					calculatedValue = tableToken->token.value ? 0 : 1;
				else
					calculatedValue = tableToken->token.value;

				//	if low then clear skip toggle flag
				if (!currentBitState)
					tableToken->token.flags &= ~MtlFlagsSkipToggle;
				break;

			case OutputLogicRisingEdgeSkipToggle:
				//	get token that should skip
				++ptr;
				clearToken = MatrixTimeLogic_TokenTable_TokenFromBitcode(&ptr);
			
				//	if bit changed from 0 to 1, and have valid token
				if (outputRisingEdge && (NULL != clearToken))
						clearToken->token.flags |= MtlFlagsSkipToggle;
				break;

			case OutputLogicFallingEdgeSkipToggle:
				//	get token that should skip
				++ptr;
				clearToken = MatrixTimeLogic_TokenTable_TokenFromBitcode(&ptr);

				//	if bit changed from 1 to 0, and have valid token
				if (outputFallingEdge && (NULL != clearToken))
						clearToken->token.flags |= MtlFlagsSkipToggle;
				break;

			case OutputLogicRisingEdgeVariableClear:
				//	get token that should clear
				++ptr;
				clearToken = MatrixTimeLogic_TokenTable_TokenFromBitcode(&ptr);

				//	if bit changed from 0 to 1, and have valid token
				if (outputRisingEdge && (NULL != clearToken))
						clearToken->token.value = 0;;
				break;
				
			case OutputLogicFallingEdgeVariableClear:
				//	get token that should clear
				++ptr;
				clearToken = MatrixTimeLogic_TokenTable_TokenFromBitcode(&ptr);

				//	if bit changed from 1 to 0, and have valid token
				if (outputFallingEdge && (NULL != clearToken))
						clearToken->token.value = 0;;
				break;
				
			case OutputLogicRisingEdgeDelay:
				//	get delay value, maximum of 60000 (60 seconds)
				++ptr;
				BitcodeInt32Value(maxCount, ptr);
				if (maxCount > 60000)
					maxCount = 60000;
			
				//	if current bit state is high, set calculated value based on elapsed time high
				if (currentBitState)
					calculatedValue = (maxCount <= (uint16_t)((Matrix.systemTime - tableToken->timestamp) & 0x0000ffff)) ?
						1 : tableToken->token.value;
				break;

			case OutputLogicFallingEdgeDelay:
				//	get delay value, maximum of 60000 (60 seconds)
				++ptr;
				BitcodeInt32Value(maxCount, ptr);
				if (maxCount > 60000)
					maxCount = 60000;

				//	if current bit is low, set calculated value based on elapsed time low
				if (!currentBitState)
					calculatedValue = (maxCount <= (uint16_t)((Matrix.systemTime - tableToken->timestamp) & 0x0000ffff)) ?
						0 : tableToken->token.value;
				break;
				
			case OutputSendTokenOnChange:
				if (calculatedValue != tableToken->token.value)
				{
					token.value = calculatedValue;
					SendToken(&token);
				}
				break;
				
			case OutputSendTokenOnOutputRisingEdge:
				if (1 <= (calculatedValue - tableToken->token.value))
				{
					token.value = calculatedValue;
					SendToken(&token);
				}
				break;

			case OutputSendTokenOnOutputFallingEdge:
				if (1 <= (tableToken->token.value - calculatedValue))
				{
					token.value = calculatedValue;
					SendToken(&token);
				}
				break;

			case OutputSendTokenOnOutputRisingByValue:
				//	get rising value threshold
				++ptr;
				BitcodeInt32Value(maxCount, ptr);

				//	if threshold exceeded, then send token
				if (maxCount <= (calculatedValue - tableToken->token.value))
				{
					token.value = calculatedValue;
					SendToken(&token);
				}
				else  //  else hold value
					calculatedValue = tableToken->token.value;
				break;

			case OutputSendTokenOnOutputFallingByValue:
				//	get falling value threshold
				++ptr;
				BitcodeInt32Value(maxCount, ptr);

				//	if threshold exceeded, then send token
				if (maxCount <= (tableToken->token.value - calculatedValue))
				{
					token.value = calculatedValue;
					SendToken(&token);
				}
				else  //  else hold value
					calculatedValue = tableToken->token.value;
				break;
				
			default:
				return -14;
				
		}  //  switch
	}  //  while

	//	save current bit state
	tableToken->token.flags = currentBitState ?
		(tableToken->token.flags | MtlFlagsInputBitstate) :
		(tableToken->token.flags & ~MtlFlagsInputBitstate);
	
	//	set the pointer reference, set the token value, and return success
	*bitcodeRef = ptr;
	tableToken->token.value = calculatedValue;
	return 0;
}

/**
  * @brief  Sends a token to either the internal channel map or to the CAN bus.
	*					If sent to the CAN bus, it is flagged as an event.
	* @param  token: A pointer to a token to send.
  * @retval None.
  */
static void SendToken(TOKEN *token)
{
	//	all time-logic output tokens go to the sequencer
	//	with the address given by the equation
	MatrixTokenSequencerController_TokenIn(token);
	
	//	if is public key
	if (!Key_IsLocalVariable(token->key))
	{
		//	broadcast to CAN bus as an event
		token->address = MATRIX_CAN_BROADCAST_ADDRESS;
		Matrix_PrivateSendCanToken(token);
	}
	
	//	all time-logic output tokens go to the application as address 132
	if ((NULL != Matrix.appInterface) && (NULL != Matrix.appInterface->tokenCallback))
	{
		token->address = MATRIX_EQUATION_PROCESSOR_NETWORK_ADDRESS;
		Matrix.appInterface->tokenCallback(token);
	}
}
