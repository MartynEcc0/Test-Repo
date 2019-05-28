/******************************************************************
 * @file CAN_Timer.c
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Controls all the timers based on the 1mS tick.
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "CFG_CPU.h"
#include "CAN_Timer.h"
#include "SYS_Message.h"
#include "CAN_Variables.h"

static U8 patternCount = 0;

typedef struct timer_t
{
    uint16_t timer;
    uint16_t timeout;
    void (*onTimeout)(void);
}
timer_t;

static timer_t timers[NUM_TIMERS] = 
{
    { 0, 0, debugTimer},
    { 0, 0, powerOnTimer},
    { 0, 0, powerOffTimer},
    { 0, 0, patternOnTimer},
    { 0, 0, patternOffTimer},
};

/**********************************************************************//*
* FUNCTION     	: systemTickInit
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: Initialises a one millisecond 'tick' timer 
*
*************************************************************************/

void systemTickInit(void)
{
//     Use timer 6 to generate 1ms Interrupt tick
    TICK_TIMER_COUNT = TICK_TIMER_RESET;
    TICK_TIMER = 0;
    
#ifdef PLL_ENABLED
    TICK_TIMER_CONFIG = 0b00011111;   // 1:4 Post-scale
#else  // PLL_ENABLED
    TICK_TIMER_CONFIG = 0b00000111;   // 1:1 Post-scale
#endif // PLL_ENABLED    
    
    TICK_TIMER_INT_FLAG = 0;
    TICK_TIMER_INT_ENABLE = 1;
}

/**********************************************************************//*
* FUNCTION     	: timerRoutine
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: updates all timers at the tick rate and executes the appropriate function on timeout
*
*************************************************************************/

void timerRoutine(void)
{

    uint8_t i;
    //reset the interrupt flag
    //process the tick timer
    for (i = 0; i < NUM_TIMERS; i++)
    {
        if (timers[i].timer > 0)
        {
            timers[i].timer--;

            if (0 == timers[i].timer)
            {
                timers[i].onTimeout();
            }
        }
    }

}
/**********************************************************************//*
* FUNCTION     	: setTimer
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: Loads a timer
*
*************************************************************************/

void setTimer(timerIdx_t timerIdx, uint16_t timeout)
{
    timer_t* pTimer = timers + timerIdx;

    // disable timer interrupt while setting timeout value
    TICK_TIMER_INT_ENABLE = 0;
    pTimer->timer = timeout;
    pTimer->timeout = timeout;
    TICK_TIMER_INT_ENABLE = 1;
}
/**********************************************************************//*
* FUNCTION     	: resetTimer
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: resets a timer to its default value
*
*************************************************************************/

void resetTimer(timerIdx_t timerIdx)
{
    timer_t* pTimer = timers + timerIdx;

    // disable timer interrupt while setting timeout value
    TICK_TIMER_INT_ENABLE = 0;
    pTimer->timer = pTimer->timeout;
    TICK_TIMER_INT_ENABLE = 1;
}

/**********************************************************************//*
* FUNCTION     	: stopTimer
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: Stops a timer
*
*************************************************************************/

void stopTimer(timerIdx_t timerIdx)
{
    // disable timer interrupt while setting timeout value
    TICK_TIMER_INT_ENABLE = 0;
    timers[timerIdx].timer = 0;
    TICK_TIMER_INT_ENABLE = 1;
}

/**********************************************************************//*
* FUNCTION     	: debugTimer
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: stops the DALI tick time from turning the relay off while replying to RSMS command
*
*************************************************************************/

void debugTimer(void)
{
    setTimer(DEBUG_TIMER, DEBUG_TICK);
    LED_PIN = !LED_PIN;
}

/**********************************************************************//*
* FUNCTION     	: powerOn
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: stops the DALI tick time from turning the relay off while replying to RSMS command
*
*************************************************************************/

void powerOnTimer(void)
{
    POWER_RELAY = 1;
    setTimer(POWER_OFF_TIMER, 15000);
    
    
    patternCount = 9;
    setTimer(PATTERN_OFF_TIMER, 5000);
    
}

/**********************************************************************//*
* FUNCTION     	: powerOff
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: stops the DALI tick time from turning the relay off while replying to RSMS command
*
*************************************************************************/

void powerOffTimer(void)
{
    POWER_RELAY = 0;
    setTimer(POWER_ON_TIMER, 5000);

}


/**********************************************************************//*
* FUNCTION     	: powerOn
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: stops the DALI tick time from turning the relay off while replying to RSMS command
*
*************************************************************************/

void patternOnTimer(void)
{
    PATTERN_TRIS = 1;
    
    if(patternCount)
    {
        if(--patternCount)
        {
            setTimer(PATTERN_OFF_TIMER, 5000);
        }
    }
    
}

/**********************************************************************//*
* FUNCTION     	: powerOff
* AUTHOR       	: Martyn Carribine
* INPUTS        : void
* OUTPUTS      	: none
* RETURNS      	: void
* DESCRIPTION  	: stops the DALI tick time from turning the relay off while replying to RSMS command
*
*************************************************************************/

void patternOffTimer(void)
{
    PATTERN_TRIS = 0;
    setTimer(PATTERN_ON_TIMER, 500);

}