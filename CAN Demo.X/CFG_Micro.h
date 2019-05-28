#ifndef CFG_Micro
#define	CFG_Micro
/******************************************************************
 * @file CFG_Micro.h
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Some device specific macros
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include <xc.h>

#define BUFFER_GLOBAL_INTERRUPT_FLAG_AND_DISABLE (GIEBuffer = 0)
#define GLOBAL_INTERRUPT_ENABLE_FLAG GIE
#define CLEAR_GLOBAL_INTERRUPT_ENABLE_FLAG (GIE = 0)
#define SET_GLOBAL_INTERRUPT_ENABLE_FLAG_BUFFER (GIEBuffer = 1)
#define BUFFERED_GLOBAL_INTERRUPT_ENABLE (GIE = GIEBuffer)

#endif	/* CFG_Micro */

