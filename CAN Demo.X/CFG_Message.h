#ifndef CFG_Message
#define	CFG_Message
/******************************************************************
 * @file SYS_MESSAGE.h
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Enumerates all messages used in this application
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "SYS_H.h"

typedef enum msgType_t
{
    MSG_NOT_AVAILABLE = 0x0,
    MSG_ONE_MILLISECOND,
} msgType_t;

#endif	/* CFG_Message */

