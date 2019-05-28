#ifndef CAN_Timer
#define	CAN_Timer
/******************************************************************
 * @file CAN_Timer.h
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Timers header file.
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "SYS_H.h"

//#define PLL_ENABLED



#define TICK_TIMER_INT_FLAG     TMR4IF
#define TICK_TIMER_INT_ENABLE   TMR4IE
#define TICK_TIMER_RESET        249
#define TICK_TIMER              TMR4
#define TICK_TIMER_COUNT        PR4
#define TICK_TIMER_CONFIG       T4CON


#define DEBUG_TICK 1000
  

typedef enum timerIdx_t {
    DEBUG_TIMER,
    POWER_ON_TIMER,
    POWER_OFF_TIMER,
    PATTERN_ON_TIMER,
    PATTERN_OFF_TIMER,
    NUM_TIMERS      
} timerIdx_t;

void systemTickInit(void);
void timerRoutine(void);
void setTimer(timerIdx_t timerIdx, uint16_t timeout);
void resetTimer(timerIdx_t timerIdx);
void stopTimer(timerIdx_t timerIdx);
void debugTimer();
void powerOnTimer();
void powerOffTimer();
void patternOnTimer();
void patternOffTimer();

#endif	/* CAN_Timer */
