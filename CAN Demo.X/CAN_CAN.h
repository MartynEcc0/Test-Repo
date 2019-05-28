#ifndef CAN_CAN
#define	CAN_CAN
/******************************************************************
 * @file CAN_CAN.h
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Timers header file.
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "SYS_H.h"

#define dSTANDARD_CAN_MSG_ID_2_0B 1
#define dEXTENDED_CAN_MSG_ID_2_0B 2

typedef union {

    struct {
        uint8_t idType;
        uint32_t id;
        uint8_t dlc;
        uint8_t data0;
        uint8_t data1;
        uint8_t data2;
        uint8_t data3;
        uint8_t data4;
        uint8_t data5;
        uint8_t data6;
        uint8_t data7;
    } frame;
    uint8_t array[14];
} uCAN_MSG_t;
extern volatile uCAN_MSG_t eccoNetCanMessage; 



void initiailseCan(void);
uint8_t CAN_transmit(uCAN_MSG_t *tempCanMsg);


#endif	/* CAN_CAN */
