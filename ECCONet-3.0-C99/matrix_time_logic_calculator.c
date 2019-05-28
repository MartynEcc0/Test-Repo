/**
  ******************************************************************************
  * @file       matrix_time_logic_calculator.c
  * @copyright  © 2017 ECCO Group.  All rights reserved.
  * @author     M. Latham, Liquid Logic
  * @version    1.0.0
  * @date       April 2017
	*
  * @brief      Processes human-readable math and logic equations with tokens
	*							as the operands and result.
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
#include "matrix_time_logic.h"



//	operand stack
typedef struct
{
	//	the stack index
	uint32_t stackIndex;
	
	//	the stack
	int32_t stack[MTL_OPERAND_STACK_SIZE];

} MTL_OPERAND_STACK;
static MTL_OPERAND_STACK operandStack;

//	push an operand on the stack
#define PushOperand(x) {                                  						\
	if (MTL_OPERAND_STACK_SIZE <= operandStack.stackIndex) return -21;		\
	operandStack.stack[operandStack.stackIndex++] = x; }
	
//	pop an operand off the stack
#define PopOperand(x) {																								\
	if (0 == operandStack.stackIndex) return -22;												\
	(x) = operandStack.stack[--operandStack.stackIndex]; }

	
//	operator stack
typedef struct
{
	//	the stack index
	uint32_t stackIndex;
	
	//	the stack
	uint8_t stack[MTL_OPERATOR_STACK_SIZE];

} MTL_OPERATOR_STACK;
static MTL_OPERATOR_STACK operatorStack;
	
//	push an operator on the stack
#define PushOperator(x) {                                  						\
	if (MTL_OPERATOR_STACK_SIZE <= operatorStack.stackIndex) return -23;	\
	operatorStack.stack[operatorStack.stackIndex++] = x; }
	
//	pop an operator off the stack
#define PopOperator(x) {																							\
	if (0 == operatorStack.stackIndex) return -24;												\
	(x) = operatorStack.stack[--operatorStack.stackIndex]; }


//	Operator priority, where lowest int is highest priority.
//	NOTE: For efficiency, this table is ordered bitcode.
#define OPERATOR_PRECEDENCE_TABLE_SIZE  24
#define FIRST_OPERATOR OperatorLogicalNot
static const uint16_t OperatorPrecedenceTable[OPERATOR_PRECEDENCE_TABLE_SIZE] =
{
		 0, //  LogicalNot
		 0, //  BitwiseInvert
		 1, //  Multiply
		 1, //  Divide
		 1, //  Modulus
		 2, //  Add
		 2, //  Subtract
		 3, //  ShiftLeft
		 3, //  ShiftRight
		 4, //  IsLessThan
		 4, //  IsLessThanOrEqual
		 4, //  IsGreaterThan
		 4, //  IsGreaterThanOrEqual
		 5, //  IsEqual
		 5, //  IsNotEqual
		 6, //  BitwiseAnd
		 7, //  BitwiseXor
		 8, //  BitwiseOr
		 9, //  LogicalAnd
		10, //  LogicalOr
		11, //  ConditionalQuestion
		11, //  ConditionalSeparator
		12, //  OperatorOpenParentheses
		12, //  OperatorCloseParentheses
};

//	method prototypes
static int UnwindStacks(void);
extern MTL_TOKEN *MatrixTimeLogic_TokenTable_TokenFromBitcode(uint8_t **bitcodeRef);


/**
  * @brief  Parses a logic file to get any output value changes.
  * @param  bitcodeRef: Pointer to bitcode pointer at EquationStart code.  Advanced to the Equals code.
  * @param  lastPtr: A pointer to the byte following the end of the bitcode file.
  * @param  out_result: A pointer to a value to receive the result.
  * @retval Returns 0 on success, else error code.
  */
int MTL_PerformCalculation(uint8_t **bitcodeRef, uint8_t *lastPtr, int32_t *out_result, uint8_t **out_firstToken)
{
	MTL_TOKEN *tableToken;
	uint8_t *ptr;
	uint16_t code, prevCode;
	uint16_t precedence, prevPrecedence;
	int32_t value;
	int status;
	
	//	validate inputs
	ptr = *bitcodeRef;
	if ((NULL == ptr) || (NULL == lastPtr))
		return -1;
	
	//	get local pointer and validate equation start
	if ((EquationStart != *ptr) && (PriorityEquationStart != *ptr) && (SuccessiveEquationStart != *ptr))
		return -2;
	
	//	reset the stacks and clear first token
	operandStack.stackIndex = 0;
	operatorStack.stackIndex = 0;
	*out_firstToken = NULL;
	
	//	while expression
	while (++ptr < lastPtr)
	{
		//	get code, and break if done
		code = *ptr;
		if ((Equals == code) || (Lambda == code))
			break;
		
		//	get code and bump pointer
		switch (code)
		{
			case OperatorOpenParentheses:
				PushOperator(code);
				break;
			
			case OperatorCloseParentheses:
				//	while backing up to left parenthesis,
				//	pop operands, perform calc, and push result
				while (operatorStack.stackIndex)
				{
					prevCode = operatorStack.stack[operatorStack.stackIndex - 1];
					if (OperatorOpenParentheses == prevCode)
					{
						PopOperator(prevCode);
						break;
					}
					UnwindStacks();
				}
				break;
				
			case ConstantValue:
				//	push operand
				value = (int32_t)*++ptr << 24;
				value |= (int32_t)*++ptr << 16;
				value |= (int32_t)*++ptr << 8;
				value |= *++ptr;
				PushOperand(value);
				break;
			
			case TokenKey:
				//	save first token
				if (NULL == *out_firstToken)
					*out_firstToken = ptr;
			
				//	get token from table and push value
				tableToken = MatrixTimeLogic_TokenTable_TokenFromBitcode(&ptr);
				if (NULL == tableToken)
					return -3;
				PushOperand(tableToken->token.value);
				break;

			//	an operator other than open or close parenthesis
			default:
				//	get operator precedence
				precedence = code - FIRST_OPERATOR;
				if (precedence >= OPERATOR_PRECEDENCE_TABLE_SIZE)
					return -4;
				precedence = OperatorPrecedenceTable[precedence];

				//	if have operator on stack
				if (operatorStack.stackIndex)
				{
					//	get previous operator precedence
					prevCode = operatorStack.stack[operatorStack.stackIndex - 1];
					prevPrecedence = prevCode - FIRST_OPERATOR;
					if (prevPrecedence >= OPERATOR_PRECEDENCE_TABLE_SIZE)
						return -5;
					prevPrecedence = OperatorPrecedenceTable[prevPrecedence];
					
					//	if operator on top of stack has higher precedence than current op,
					//	then unwind the stack
					if (precedence > prevPrecedence)
					{
						if (0 != (status = UnwindStacks()))
							return status;
					}
				}
				
				//	push the current operator
				PushOperator(code);
				break;
		}
	}
	
	//	unwind the stack
	while (operatorStack.stackIndex && operandStack.stackIndex)
	{
		if (0 != (status = UnwindStacks()))
			return status;
	}

	//	set the pointer reference, pop output value, and return success
	*bitcodeRef = ptr;
	PopOperand(*out_result);
	return 0;
}

/**
  * @brief  Unwinds the stacks through one iteration.
  * @param  None.
  * @retval Returns 0 on success, else stack error code.
  */
static int UnwindStacks(void)
{
	uint8_t stackOp;
	int32_t operand1, operand2, operand3;
	
	//	pop operator
	PopOperator(stackOp);
	switch (stackOp)
	{
		case OperatorBitwiseInvert:
			PopOperand(operand1);
			operand1 = ~operand1;
			break;
		
		case OperatorLogicalNot:
			PopOperand(operand1);
			operand1 = !operand1;
			break;
		
		case OperatorConditionalSeparator:
			PopOperator(stackOp);
			PopOperand(operand2);
			PopOperand(operand1);
			PopOperand(operand3);
			operand1 = operand3 ? operand1 : operand2;
			break;

		default:
			PopOperand(operand2);
			PopOperand(operand1);
			switch (stackOp)
			{
				case OperatorMultiply:
					operand1 = operand1 * operand2;
					break;
				case OperatorDivide:
					operand1 = operand1 / operand2;
					break;
				case OperatorModulus:
					operand1 = operand1 % operand2;
					break;
				case OperatorAdd:
					operand1 = operand1 + operand2;
					break;
				case OperatorSubtract:
					operand1 = operand1 - operand2;
					break;
				case OperatorShiftLeft:
					operand1 = operand1 << operand2;
					break;
				case OperatorShiftRight:
					operand1 = operand1 >> operand2;
					break;
				case OperatorIsLessThan:
					operand1 = operand1 < operand2;
					break;
				case OperatorIsLessThanOrEqual:
					operand1 = operand1 <= operand2;
					break;
				case OperatorIsGreaterThan:
					operand1 = operand1 > operand2;
					break;
				case OperatorIsGreaterThanOrEqual:
					operand1 = operand1 >= operand2;
					break;
				case OperatorIsEqual:
					operand1 = operand1 == operand2;
					break;
				case OperatorIsNotEqual:
					operand1 = operand1 != operand2;
					break;
				case OperatorBitwiseAnd:
					operand1 = operand1 & operand2;
					break;
				case OperatorBitwiseXor:
					operand1 = operand1 ^ operand2;
					break;
				case OperatorBitwiseOr:
					operand1 = operand1 | operand2;
					break;
				case OperatorLogicalAnd:
					operand1 = operand1 && operand2;
					break;
				case OperatorLogicalOr:
					operand1 = operand1 || operand2;
					break;
				default:
					break;
			}
			break;
	}
	PushOperand(operand1);
	return 0;
}

