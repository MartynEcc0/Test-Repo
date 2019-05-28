/******************************************************************
 * @file CAN_Timer.c
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Interrupts.
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "SYS_H.h"
#include "CFG_CPU.h"
#include "CAN_main.h"
#include "SYS_Message.h"
#include "CAN_Timer.h"
/**********************************************************************//*
* FUNCTION     	: isr
* AUTHOR       	: Martyn Carribine
* INPUTS        : none
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: entry point for interrupts
*************************************************************************/
void __interrupt () isr (void)
{
    if (TICK_TIMER_INT_FLAG) // Permanently enabled therefore no need to check enable flag
    {
        sendMsg(MSG_ONE_MILLISECOND);
        TICK_TIMER_INT_FLAG = 0; 
    }
}
