/*******************************************************************************
* File Name       : canfd.h
* Description     : 
* Original Author : wangchao
* Created on      : Nov 19, 2020, 2:41 PM
*******************************************************************************/

#ifndef CANFD_H
#define	CANFD_H

#ifdef	__cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Conditional Definitions
#ifdef CANFD_C
    #define CANFD_PUBLIC
    #define CANFD_CONST
#else
    #define CANFD_PUBLIC extern
    #define CANFD_CONST  const
#endif

//------------------------------------------------------------------------------    
// Include

//------------------------------------------------------------------------------
// Public Constants and Macro Definitions
// Transmit Channels
#define APP_TX_FIFO CAN_FIFO_CH2

// Receive Channels
#define APP_RX_FIFO CAN_FIFO_CH1

/*--------------------- Macro for exam----------------------------------------*/    

/*--------------------- Macro for exam----------------------------------------*/    
    
//------------------------------------------------------------------------------
// Public Enumerated and Structure Definitions

//------------------------------------------------------------------------------
// Public Variables

//------------------------------------------------------------------------------
// Public Function Prototypes
void CANFDSPI_Init( void );
void CANFDSPI_Test( void );    

/******************************************************************************/    
#ifdef	__cplusplus
}
#endif

#endif	/* CANFD_H */

