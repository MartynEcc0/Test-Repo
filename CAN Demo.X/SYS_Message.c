/******************************************************************
 * @file SYS_MESSAGE.c
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Handles all messages passed between threads
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/

#include "SYS_H.h"
#include "CFG_Micro.h"
#include "CFG_Message.h"
#include "SYS_Message.h"


#define MSG_QUEUE_SIZE 8
#define MSG_QUEUE_SIZE_MASK 0b111

// buffer of GIE flag for PICs
U8 GIEBuffer;

static uint8_t the_msgQueue[MSG_QUEUE_SIZE];    // the message queue

/* 	By using single byte values we can be sure of atomic updates. At any
	time the_msgPutIndex is the index at which the next message will be put
	and the_msgGetIndex is the index from which the next message will be read.
	If they are equal then the queue is empty and if, allowing for wrapping,
	the_msgPutIndex + 1 == the_msgGetIndex then the queue is full. Note that
	this will always leave one slot empty */

static int the_msgPutIndex = 0;   // where to put the next incoming message
static uint8_t the_msgGetIndex = 0;   // where to get the next outgoing message

//static bool_t the_queueOverflowed = FALSE;


/************************************************************************
*
* FUNCTION      : msgInit
* AUTHOR        : Martyn Carribine
* INPUTS        : none
* OUTPUTS       : none
* RETURNS       : none
* DESCRIPTION   : Resets the queue.
*
*************************************************************************/
void initMsgQueue( void )
{
    uint8_t i;
	
    for (i=0; i<MSG_QUEUE_SIZE; i++)
    {
        the_msgQueue[i] = MSG_NOT_AVAILABLE;
    }

    the_msgPutIndex = 0;
    the_msgGetIndex = 0;
//    the_queueOverflowed = FALSE;
}
	

/************************************************************************
*
* FUNCTION      : hasMsgQueueOverflowed
* AUTHOR        : Martyn Carribine
* INPUTS        : none
* OUTPUTS       : none
* RETURNS       : bool_t - TRUE if the message queue has overflowed since
*                 the last initialisation, otherwise FALSE
* DESCRIPTION   :
*
*************************************************************************/
//bool_t hasMsgQueueOverflowed(void)
//{
//    return the_queueOverflowed;
//}


/************************************************************************
*
* FUNCTION      : msgSend
* AUTHOR        : Martyn Carribine
* INPUTS        : uint8_t msg - the message
* OUTPUTS       : none
* RETURNS       : none
* DESCRIPTION   : Pushes message on to the queue.  Messages that arrive
*                 when the queue is full are just ignored.
*
*************************************************************************/
void sendMsg(uint8_t msg)
{
    BUFFER_GLOBAL_INTERRUPT_FLAG_AND_DISABLE;
    if (1 == GLOBAL_INTERRUPT_ENABLE_FLAG)
    {
        do
        {
            CLEAR_GLOBAL_INTERRUPT_ENABLE_FLAG;
        }while (1 == GLOBAL_INTERRUPT_ENABLE_FLAG);
        SET_GLOBAL_INTERRUPT_ENABLE_FLAG_BUFFER;
    }


    the_msgQueue[(the_msgPutIndex++ & MSG_QUEUE_SIZE_MASK)] = msg;
    BUFFERED_GLOBAL_INTERRUPT_ENABLE;
}


/************************************************************************
*
* FUNCTION      : msgGet
* AUTHOR        : Martyn Carribine
* INPUTS        : none
* OUTPUTS       : none
* RETURNS       : uint8_t - the message
* DESCRIPTION   : Pops the oldest message off the front of the queue.
*
*************************************************************************/
uint8_t getMsg(void)
{
    msgType_t msg = MSG_NOT_AVAILABLE;
    uint8_t tmpMsgGetIndex = the_msgGetIndex;

    // Check queue is not empty
    if (tmpMsgGetIndex != (the_msgPutIndex & MSG_QUEUE_SIZE_MASK))
    {
        msg = the_msgQueue[tmpMsgGetIndex];
        the_msgQueue[tmpMsgGetIndex] = MSG_NOT_AVAILABLE;
        ++tmpMsgGetIndex;

        // Check for wraparound
        if (tmpMsgGetIndex >= MSG_QUEUE_SIZE)
           tmpMsgGetIndex = 0;

        the_msgGetIndex = tmpMsgGetIndex;
    }

    return (msg);
}
