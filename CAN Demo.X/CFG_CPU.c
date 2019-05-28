/******************************************************************
 * @file CFG_CPU.c
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Set up the initial conditions of the microcontroller.
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "CFG_CPU.h"
#include "CAN_Timer.h"
/**********************************************************************//*
* FUNCTION     	: initCpu
* AUTHOR       	: Martyn Carribine
*************************************************************************/
void initCpu()
{
//    // SET UP SYSTEM CLOCK

#ifdef PLL_ENABLED
    OSCTUNE = 0b01000000;   // Enable PLL 
#else  // PLL_ENABLED
    OSCTUNE = 0b00000000;   // Enable PLL     
#endif // PLL_ENABLED
    OSCCON = 0b01110000;    // Set up HFINTOSC to 16MHZ (PLL'ed up to 64MHz)
    OSCCON2 = 0b00000000;

//
//    // SET UP SCHMIT INPUTS
//    INLVLA = 0xD0;

// SET UP INITIAL OUTPUT LEVELS
    PORTA = PORTA_OP_MASK; 
    PORTB = PORTB_OP_MASK;        
    PORTC = PORTC_OP_MASK;
    PORTD = PORTD_OP_MASK;        
    PORTE = PORTE_OP_MASK;
    
//    // SET UP ANALOGUE INPUTS
//    ANSELA = PORTA_ANA_MASK; 
//    ANSELB = PORTB_ANA_MASK; //     
//    ANSELC = PORTC_ANA_MASK; 

// SET UP PORT DIRECTIONS
    TRISA = PORTA_IO_MASK; 
    TRISB = PORTB_IO_MASK;
    TRISC = PORTC_IO_MASK;
    TRISD = PORTD_IO_MASK;
    TRISE = PORTE_IO_MASK;    

}


