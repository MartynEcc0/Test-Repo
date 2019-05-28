#ifndef SYS_Message
#define SYS_Message
/******************************************************************
 * @file SYS_MESSAGE.h
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Handles all messages passed between threads
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "SYS_H.h"
#include "CFG_Message.h"

void initMsgQueue( void );
void sendMsg(uint8_t msg);
U8 getMsg(void);

#endif // SYS_Message
