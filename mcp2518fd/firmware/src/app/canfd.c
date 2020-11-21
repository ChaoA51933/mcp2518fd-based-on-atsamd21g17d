/*******************************************************************************
* File Name       : canfd.c
* Description     : 
* Original Author : wangchao
* Created on      : Nov 19, 2020, 2:41 PM
*******************************************************************************/

#ifndef CANFD_C
#define	CANFD_C

//------------------------------------------------------------------------------
// Include
#include "canfd.h"
#include "definitions.h"                // SYS function prototypes
#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"
#include "drv_canfdspi_defines.h"
#include <xc.h>
//------------------------------------------------------------------------------
// Constants and Macro Definitions
#define APP_USE_TX_INT
#define APP_USE_RX_INT
//------------------------------------------------------------------------------
// Public Variables

//------------------------------------------------------------------------------
// Private Enumerated and Structure Definitions

CAN_CONFIG config;

// Transmit objects
CAN_TX_FIFO_CONFIG txConfig;
CAN_TX_FIFO_EVENT txFlags;
CAN_TX_MSGOBJ txObj;
uint8_t txd[MAX_DATA_BYTES];

// Receive objects
CAN_RX_FIFO_CONFIG rxConfig;
REG_CiFLTOBJ fObj;
REG_CiMASK mObj;
CAN_RX_FIFO_EVENT rxFlags;
CAN_RX_MSGOBJ rxObj;
uint8_t rxd[MAX_DATA_BYTES];

uint8_t tec;
uint8_t rec;
CAN_ERROR_STATE errorFlags;

//------------------------------------------------------------------------------
// Private Function Prototypes (i.e. static)

//------------------------------------------------------------------------------
// Private Variables (i.e. static)

////////////////////////////////////////////////////////////////////////////////
// Public Function Implementation
/*******************************************************************************
* Public Function : CANFDSPI_Init
* Description     : 
* Purpose         :
* Input           : 
* Return          : 
* Calls           :    
* Called by       :
* Note			  : 
*******************************************************************************/
void CANFDSPI_Init( void )
{
    // Reset device
    DRV_CANFDSPI_Reset();

    // Enable ECC and initialize RAM
    DRV_CANFDSPI_EccEnable();

    DRV_CANFDSPI_RamInit(0xff);

    // Configure device
    DRV_CANFDSPI_ConfigureObjectReset(&config);
    config.IsoCrcEnable = 1;
    config.StoreInTEF = 0;
    config.TXQEnable = 0;

    DRV_CANFDSPI_Configure(&config);

    // Setup TX FIFO
    DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
    txConfig.FifoSize = 7;
    txConfig.PayLoadSize = CAN_PLSIZE_64;
    txConfig.TxPriority = 1;

    DRV_CANFDSPI_TransmitChannelConfigure(APP_TX_FIFO, &txConfig);

    // Setup RX FIFO
    DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig);
    rxConfig.FifoSize = 15;
    rxConfig.PayLoadSize = CAN_PLSIZE_64;

    DRV_CANFDSPI_ReceiveChannelConfigure(APP_RX_FIFO, &rxConfig);

    // Setup RX Filter
    fObj.word = 0;
    fObj.bF.SID = 0x300;
    fObj.bF.EXIDE = 0;
    fObj.bF.EID = 0x00;

    DRV_CANFDSPI_FilterObjectConfigure(CAN_FILTER0, &fObj.bF);

    // Setup RX Mask
    mObj.word = 0;
    mObj.bF.MSID = 0x7F8;
    mObj.bF.MIDE = 1; // Only allow standard IDs
    mObj.bF.MEID = 0x0;
    DRV_CANFDSPI_FilterMaskConfigure(CAN_FILTER0, &mObj.bF);

    // Link FIFO and Filter
    DRV_CANFDSPI_FilterToFifoLink(CAN_FILTER0, APP_RX_FIFO, true);

    // Setup Bit Time
    DRV_CANFDSPI_BitTimeConfigure(CAN_500K_2M, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M);

    // Setup Transmit and Receive Interrupts
    DRV_CANFDSPI_GpioModeConfigure(GPIO_MODE_INT, GPIO_MODE_INT);
    DRV_CANFDSPI_TransmitChannelEventEnable(APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT);
    DRV_CANFDSPI_ReceiveChannelEventEnable(APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT);
    DRV_CANFDSPI_ModuleEventEnable(CAN_TX_EVENT | CAN_RX_EVENT);

    // Select Normal Mode
    //DRV_CANFDSPI_OperationModeSelect(CAN_NORMAL_MODE);
    
    DRV_CANFDSPI_OperationModeSelect(CAN_EXTERNAL_LOOPBACK_MODE);
    
    STBY_Clear();
}

/*******************************************************************************
* Public Function : CANFDSPI_Test
* Description     : 
* Purpose         :
* Input           : 
* Return          : 
* Calls           :    
* Called by       :
* Note			  : 
*******************************************************************************/
void CANFDSPI_Test( void )
{
    uint8_t attempts = 50;
    uint8_t n;
    int16_t i;
    static uint16_t messageID_add = 0;
    
    /**********************Prepare Data****************************************/
    Nop();
    Nop();
    txObj.bF.id.SID = 0x300 + ((messageID_add++) & 0xF);

    txObj.bF.ctrl.DLC = CAN_DLC_64;
    txObj.bF.ctrl.IDE = 0;
    txObj.bF.ctrl.BRS = 1;
    txObj.bF.ctrl.FDF = 1;

    n = DRV_CANFDSPI_DlcToDataBytes((CAN_DLC) txObj.bF.ctrl.DLC);

    for (i = 0; i < n; i++)
    {
        txd[i] = rand() & 0xff;
    }

    /**********************transmit********************************************/
    // Check if FIFO is not full
    do {
#ifdef APP_USE_TX_INT
        
#else
        DRV_CANFDSPI_TransmitChannelEventGet(APP_TX_FIFO, &txFlags);
#endif
        if (attempts == 0) {
            Nop();
            Nop();
            DRV_CANFDSPI_ErrorCountStateGet(&tec, &rec, &errorFlags);
            return;
        }
        attempts--;
    }
#ifdef APP_USE_TX_INT
    while (nINT0_Get());
#else
    while (!(txFlags & CAN_TX_FIFO_NOT_FULL_EVENT));
#endif

    DRV_CANFDSPI_TransmitChannelLoad(APP_TX_FIFO, &txObj, txd, n, true);
    printf("\r\n Transmit message's ID = %04x, and txd[0] = %02x", txObj.bF.id.SID, txd[0]);

    SYSTICK_DelayMs (10);    
    /**********************receive*********************************************/
    // Check if FIFO is not empty
#ifdef APP_USE_RX_INT    
    if (!nINT1_Get()) {
#else
    DRV_CANFDSPI_ReceiveChannelEventGet(APP_RX_FIFO, &rxFlags);
    if (rxFlags & CAN_RX_FIFO_NOT_EMPTY_EVENT) {
#endif
        // Get message
        DRV_CANFDSPI_ReceiveMessageGet(APP_RX_FIFO, &rxObj, rxd, MAX_DATA_BYTES);
        
        printf("\r\n Receive message's ID = %04x, and rxd[0] = %02x", txObj.bF.id.SID, rxd[0]);        
    }
    
}

////////////////////////////////////////////////////////////////////////////////
// Private Function Implementation
/*******************************************************************************
* Private Function: TempFunc
* Description     : 
* Purpose         :
* Input           : 
* Return          : 
* Calls           :    
* Called by       :
* Note			  :
*******************************************************************************/

/******************************************************************************/
#undef CANFD_C
#endif  /* CANFD_C */
