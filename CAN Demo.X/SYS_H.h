#ifndef SYS_H
#define	SYS_H
/******************************************************************
 * @file SYS_H.h
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Useful typedefs.
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "CFG_Micro.h"

#if !defined FALSE && !defined TRUE
#define FALSE           (0)
#define TRUE            (!FALSE)
#endif /* !defined FALSE && #if !defined TRUE */

#define U8_MAX 255

typedef unsigned char bool_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed long int32_t;
typedef unsigned long uint32_t;

typedef unsigned char bool;
typedef signed char S8;
typedef unsigned char U8;
typedef signed short S16;
typedef unsigned short U16;
typedef signed long S32;
typedef unsigned long U32;

#endif	/* SYS_H */

