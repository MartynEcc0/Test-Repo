/******************************************************************
 * @file CAN_CAN.c
 * @author Martyn Carribine
 * @date 06/02/2019
 * @brief Controls all the timers based on the 1mS tick.
 * Copyright (c) ECCO Safety Group
 * 
 ******************************************************************/
#include "CFG_CPU.h"
#include "CAN_CAN.h"

static void convertCANid2Reg(uint32_t tempPassedInID, uint8_t canIdType, uint8_t *passedInEIDH, uint8_t *passedInEIDL, uint8_t *passedInSIDH, uint8_t *passedInSIDL);

volatile uCAN_MSG_t eccoNetCanMessage; 

void initiailseCan(void)
{
    CANCON = 0x80;
    while (0x80 != (CANSTAT & 0xE0)); // wait until ECAN is in config mode
    
    /**
    Mode 0
    */
    ECANCON = 0x00;
    
    /**
    Initialize CAN I/O
    */
    CIOCON = 0x20;
    
    /**
    Mask and Filter definitions
    ........................................................    
    CAN ID		ID Type		Mask				Filter		Buffer    
    ........................................................    
    ........................................................
    */
 
/**    
    Initialize Receive Masks
*/   
    RXM0EIDH = 0x00;
    RXM0EIDL = 0x00;
    RXM0SIDH = 0x00;
    RXM0SIDL = 0x00;
    RXM1EIDH = 0x00;
    RXM1EIDL = 0x00;
    RXM1SIDH = 0x00;
    RXM1SIDL = 0x00;
 
    /**
    Initialize Receive Filters
    */   
    RXF0EIDH = 0x00;
    RXF0EIDL = 0x00;
    RXF0SIDH = 0x00;
    RXF0SIDL = 0x00;
    RXF1EIDH = 0x00;
    RXF1EIDL = 0x00;
    RXF1SIDH = 0x00;
    RXF1SIDL = 0x00;
    RXF2EIDH = 0x00;
    RXF2EIDL = 0x00;
    RXF2SIDH = 0x00;
    RXF2SIDL = 0x00;
    RXF3EIDH = 0x00;
    RXF3EIDL = 0x00;
    RXF3SIDH = 0x00;
    RXF3SIDL = 0x00;
    RXF4EIDH = 0x00;
    RXF4EIDL = 0x00;
    RXF4SIDH = 0x00;
    RXF4SIDL = 0x00;
    RXF5EIDH = 0x00;
    RXF5EIDL = 0x00;
    RXF5SIDH = 0x00;
    RXF5SIDL = 0x00;

    /**
    Initialize CAN Timings
    */
    
  	/**
	Baud rate: 125kbps
	System frequency: 16000000
    ECAN clock frequency: 16000000
	Time quanta: 8
	Sample point: 1-1-4-2
	Sample point: 75%
	*/ 
    
    BRGCON1 = 0x07;
    BRGCON2 = 0x98;
    BRGCON3 = 0x81;
    
    CANCON = 0x00;
    while (0x00 != (CANSTAT & 0xE0)); // wait until ECAN is in Normal mode   
}


uint8_t CAN_transmit(uCAN_MSG_t *tempCanMsg) {
    uint8_t tempEIDH = 0;
    uint8_t tempEIDL = 0;
    uint8_t tempSIDH = 0;
    uint8_t tempSIDL = 0;

    uint8_t returnValue = 0;

    if (TXB0CONbits.TXREQ != 1) 
    {
        convertCANid2Reg(tempCanMsg->frame.id, tempCanMsg->frame.idType, &tempEIDH, &tempEIDL, &tempSIDH, &tempSIDL);

        TXB0EIDH = tempEIDH;
        TXB0EIDL = tempEIDL;
        TXB0SIDH = tempSIDH;
        TXB0SIDL = tempSIDL;
        TXB0DLC  = tempCanMsg->frame.dlc;
        TXB0D0   = tempCanMsg->frame.data0;
        TXB0D1   = tempCanMsg->frame.data1;
        TXB0D2   = tempCanMsg->frame.data2;
        TXB0D3   = tempCanMsg->frame.data3;
        TXB0D4   = tempCanMsg->frame.data4;
        TXB0D5   = tempCanMsg->frame.data5;
        TXB0D6   = tempCanMsg->frame.data6;
        TXB0D7   = tempCanMsg->frame.data7;

        TXB0CONbits.TXREQ = 1; //Set the buffer to transmit		
        returnValue = 1;
        
    } 
    else if (TXB1CONbits.TXREQ != 1) 
    {

        convertCANid2Reg(tempCanMsg->frame.id, tempCanMsg->frame.idType, &tempEIDH, &tempEIDL, &tempSIDH, &tempSIDL);

        TXB1EIDH = tempEIDH;
        TXB1EIDL = tempEIDL;
        TXB1SIDH = tempSIDH;
        TXB1SIDL = tempSIDL;
        TXB1DLC  = tempCanMsg->frame.dlc;
        TXB1D0   = tempCanMsg->frame.data0;
        TXB1D1   = tempCanMsg->frame.data1;
        TXB1D2   = tempCanMsg->frame.data2;
        TXB1D3   = tempCanMsg->frame.data3;
        TXB1D4   = tempCanMsg->frame.data4;
        TXB1D5   = tempCanMsg->frame.data5;
        TXB1D6   = tempCanMsg->frame.data6;
        TXB1D7   = tempCanMsg->frame.data7;

        TXB1CONbits.TXREQ = 1; //Set the buffer to transmit		
        returnValue = 1;
    } 
    else if (TXB2CONbits.TXREQ != 1) 
    {

        convertCANid2Reg(tempCanMsg->frame.id, tempCanMsg->frame.idType, &tempEIDH, &tempEIDL, &tempSIDH, &tempSIDL);

        TXB2EIDH = tempEIDH;
        TXB2EIDL = tempEIDL;
        TXB2SIDH = tempSIDH;
        TXB2SIDL = tempSIDL;
        TXB2DLC  = tempCanMsg->frame.dlc;
        TXB2D0   = tempCanMsg->frame.data0;
        TXB2D1   = tempCanMsg->frame.data1;
        TXB2D2   = tempCanMsg->frame.data2;
        TXB2D3   = tempCanMsg->frame.data3;
        TXB2D4   = tempCanMsg->frame.data4;
        TXB2D5   = tempCanMsg->frame.data5;
        TXB2D6   = tempCanMsg->frame.data6;
        TXB2D7   = tempCanMsg->frame.data7;

        TXB2CONbits.TXREQ = 1; //Set the buffer to transmit		
        returnValue = 1;
    }
    return (returnValue);
}

static void convertCANid2Reg(uint32_t tempPassedInID, uint8_t canIdType, uint8_t *passedInEIDH, uint8_t *passedInEIDL, uint8_t *passedInSIDH, uint8_t *passedInSIDL) 
{
    uint8_t wipSIDL = 0;

    if (canIdType == dEXTENDED_CAN_MSG_ID_2_0B) {

        //EIDL
        *passedInEIDL = 0xFF & tempPassedInID; //CAN_extendedLo_ID_TX1 = &HFF And CAN_UserEnter_ID_TX1
        tempPassedInID = tempPassedInID >> 8; //CAN_UserEnter_ID_TX1 = CAN_UserEnter_ID_TX1 >> 8

        //EIDH
        *passedInEIDH = 0xFF & tempPassedInID; //CAN_extendedHi_ID_TX1 = &HFF And CAN_UserEnter_ID_TX1
        tempPassedInID = tempPassedInID >> 8; //CAN_UserEnter_ID_TX1 = CAN_UserEnter_ID_TX1 >> 8

        //SIDL
        //push back 5 and or it
        wipSIDL = 0x03 & tempPassedInID;
        tempPassedInID = tempPassedInID << 3; //CAN_UserEnter_ID_TX1 = CAN_UserEnter_ID_TX1 << 3
        wipSIDL = (0xE0 & tempPassedInID) + wipSIDL;
        wipSIDL = (uint8_t)(wipSIDL + 0x08); // TEMP_CAN_standardLo_ID_TX1 = TEMP_CAN_standardLo_ID_TX1 + &H8
        *passedInSIDL = (uint8_t)(0xEB & wipSIDL); //CAN_standardLo_ID_TX1 = &HEB And TEMP_CAN_standardLo_ID_TX1

        //SIDH
        tempPassedInID = tempPassedInID >> 8;
        *passedInSIDH = 0xFF & tempPassedInID;
    } 
    else //(canIdType == dSTANDARD_CAN_MSG_ID_2_0B)
    {
        *passedInEIDH = 0;
        *passedInEIDL = 0;
        tempPassedInID = tempPassedInID << 5;
        *passedInSIDL = 0xFF & tempPassedInID;
        tempPassedInID = tempPassedInID >> 8;
        *passedInSIDH = 0xFF & tempPassedInID;
    }
}