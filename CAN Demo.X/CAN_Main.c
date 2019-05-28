/******************************************************************
 * @file CAN_Main.c
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Main routine for the Microchip CAN demo board
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "SYS_H.h"
#include "CFG_CPU.h"
#include "CAN_Timer.h"
#include "SYS_Message.h"


/*******************************************************************************
*   Global Variables
*******************************************************************************/
//volatile uint32_t systemTime = 0;
//extern const MATRIX_INTERFACE_TABLE MatrixInterfaceTable;


/*******************************************************************************
*   Local Variables
*******************************************************************************/
static uint8_t msg = 0;

int main(void)
{
    initCpu(); //Set up the clock
    systemTickInit(); //start the system tick
    PATTERN_RELAY = 0;
    PATTERN_TRIS = 1;
    setTimer(DEBUG_TIMER, DEBUG_TICK);
    setTimer(POWER_ON_TIMER, 2000);
    PEIE = 1;
    GIE = 1;
    
    
///////////////////////////////////////////////////////////////////////////////////////////
//  Main Loop
///////////////////////////////////////////////////////////////////////////////////////////
    while(1)
    {
//        Matrix_Clock(systemTime);
        msg = getMsg();
        switch (msg)
        {
            case MSG_ONE_MILLISECOND:
                timerRoutine();
                break;
        }            
    }//end of while loop
}//end of main

