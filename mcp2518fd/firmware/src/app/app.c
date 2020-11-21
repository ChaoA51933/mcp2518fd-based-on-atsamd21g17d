/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "xc.h"
#include "system_config.h"
#include <math.h>

#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

APP_DATA appData;

CAN_CONFIG config;
CAN_OPERATION_MODE opMode;

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

uint32_t delayCount;

REG_t reg;

bool txFromFlash;

// Picture to transmit in Flash
const uint8_t jpgFlashData[] = {
#include "csv/pic.csv"
    //#include "csv/largepic.csv"
    //    #include "csv/verylargepic.csv"
};

// 100KB for received picture
uint8_t jpgRamData[APP_MAX_JPG_SIZE];

APP_SwitchState switchState, lastSwitchState, newSwitchState;
uint8_t debounceCounterSW0;

bool switchChanged;

bool ramInitialized;

APP_Payload payload;

uint32_t txId = TX_RESPONSE_ID;
uint32_t jpgTxIdx = 0;
uint32_t jpgRxIdx = 0;
CAN_BITTIME_SETUP selectedBitTime = CAN_500K_2M;
uint8_t ledCount = 0, ledState = 0;

uint8_t i;
uint32_t sizeOfTxData = 0;

CAN_BUS_DIAGNOSTIC busDiagnostics;
uint8_t tec;
uint8_t rec;
CAN_ERROR_STATE errorFlags;
bool errorDetected = false;

// Bit time configurations
const uint16_t BitTimeConfig125K[] = {
    1, 0x11, 500
};

const uint16_t BitTimeConfig250K[] = {
    7, 0x08, 500, 833, 1000, 1500, 2000, 3076, 4000
};

const uint16_t BitTimeConfig500K[] = {
    8, 0x00, 1000, 2000, 3076, 4000, 5000, 6666, 8000, 10000
};

const uint16_t BitTimeConfig1M[] = {
    2, 0x0f, 4000, 8000
};


// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
 */

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

inline void APP_LED_Clear(void)
{
    LED0_Set();
}

inline void APP_LED_Set(void)
{
    LED0_Clear();
}

void APP_DebounceSwitches(bool* changed)
{
    APP_SwitchState previousState;
    bool b;

    previousState = switchState;

    // Read new states
    newSwitchState.SW0 = SW0_Get();

    // Debounce SW0
    if (lastSwitchState.SW0 == newSwitchState.SW0)
    {
        if (debounceCounterSW0 > 0)
        {
            debounceCounterSW0--;
        }
        else
        {
            switchState.SW0 = newSwitchState.SW0;
        }
    }
    else
    {
        debounceCounterSW0 = APP_DEBOUNCE_TIME;
    }

    // Keep new state
    lastSwitchState.SW0 = newSwitchState.SW0;

    // Changed?
    b = (previousState.SW0 != switchState.SW0);

    if (b) {
        Nop();
        Nop();
        *changed = true;
    }
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

void APP_Initialize(void)
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    // Init switch debounce
    lastSwitchState.SW0 = APP_SWITCH_PRESSED;

    switchState.SW0 = APP_SWITCH_RELEASED;

    txFromFlash = true;
    switchChanged = true;

    payload.On = false;

    ramInitialized = false;
    selectedBitTime = CAN_500K_2M;
}

void APP_Tasks(void)
{
    // Debounce switches
    APP_DebounceSwitches(&switchChanged);
    if (switchChanged) {
        Nop();
        Nop();
    }

    /* Check the application's current state. */
    switch (appData.state) {
            /* Application's initial state. */
        case APP_STATE_INIT:
        {
            Nop();
            Nop();
            APP_LED_Set();

            APP_CANFDSPI_Init();

            // Reset debounce counters
            debounceCounterS1 = APP_DEBOUNCE_TIME;

            appData.state = APP_STATE_INIT_TXOBJ;

            APP_LED_Clear();
            break;
        }

            /* Initialize TX Object */
        case APP_STATE_INIT_TXOBJ:
        {
            Nop();
            Nop();
            // Configure transmit message
            txObj.word[0] = 0;
            txObj.word[1] = 0;

            txObj.bF.id.SID = TX_RESPONSE_ID;
            txObj.bF.id.EID = 0;

            txObj.bF.ctrl.BRS = 1;
            txObj.bF.ctrl.DLC = CAN_DLC_64;
            txObj.bF.ctrl.FDF = 1;
            txObj.bF.ctrl.IDE = 0;

            // Configure message data
            int i;
            for (i = 0; i < MAX_DATA_BYTES; i++) txd[i] = txObj.bF.id.SID + i;

            appData.state = APP_STATE_FLASH_LEDS;
            break;
        }

            /* Flash all LEDs */
        case APP_STATE_FLASH_LEDS:
        {
            // Delay loop
            Nop();
            Nop();

            // Delay expired, update LEDs and reset delayCount
            if (delayCount == 0) {
                if (ledCount < APP_N_LED) {
                    ledState |= 1 << ledCount;
                } else {
                    ledState = 0;
                }

                APP_LED_Write(ledState);

                ledCount++;
                delayCount = APP_LED_TIME;
            } else {
                delayCount--;
            }

            if (ledCount > (APP_N_LED + 1)) {
#ifdef APP_BLINK_LED_AFTER_SETBITTIME
                ledCount = 0;
#endif
                appData.state = APP_STATE_RECEIVE;
            } else {
                appData.state = APP_STATE_FLASH_LEDS;
            }

            break;
        }

            /* Receive a message */
        case APP_STATE_RECEIVE:
        {
            Nop();
            Nop();
            appData.state = APP_ReceiveMessage_Tasks();
            break;
        }

        case APP_STATE_PAYLOAD:
        {
            Nop();
            Nop();
            APP_PayLoad_Tasks();

            appData.state = APP_STATE_SWITCH_CHANGED;
            //            appData.state = APP_STATE_RECEIVE;
            break;
        }

            /* Transmit changes in switch states */
        case APP_STATE_SWITCH_CHANGED:
        {
            if (switchChanged) {
                // Transmit new state
                txObj.bF.id.SID = BUTTON_STATUS_ID;

                txObj.bF.ctrl.DLC = CAN_DLC_4;
                txObj.bF.ctrl.IDE = 0;
                txObj.bF.ctrl.BRS = 1;
                txObj.bF.ctrl.FDF = 1;

                txd[0] = 0;
                if (switchState.S1 == APP_SWITCH_PRESSED) txd[0] += 0x8;
                if (switchState.S2 == APP_SWITCH_PRESSED) txd[0] += 0x4;
                if (switchState.S3 == APP_SWITCH_PRESSED) txd[0] += 0x2;
                if (switchState.S4 == APP_SWITCH_PRESSED) txd[0] += 0x1;

                // Clear other data bytes
                txd[1] = 0;
                txd[2] = 0;
                txd[3] = 0;

                switchChanged = false;
                Nop();
                Nop();
                APP_TransmitMessageQueue();
            }

            appData.state = APP_STATE_RECEIVE;

            break;
        }

            /* Request Configuration mode */
        case APP_STATE_REQUEST_CONFIG:
        {
            // FIXME:
            //            appData.state = APP_STATE_RECEIVE;
            //            break;

            // Select Configuration Mode
            DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_0, CAN_CONFIGURATION_MODE);

            appData.state = APP_STATE_WAIT_FOR_CONFIG;

            break;
        }

            /* Wait for Configuration mode, so it is safe to reset */
        case APP_STATE_WAIT_FOR_CONFIG:
        {
            // MCP25xxFD will finish transmit message first
            // We need to make sure transmit is done before we go to APP_STATE_INIT and reset the MCP25xxFD
            // Otherwise, we will get error frames

            opMode = DRV_CANFDSPI_OperationModeGet(DRV_CANFDSPI_INDEX_0);

            if (opMode != CAN_CONFIGURATION_MODE) {
                appData.state = APP_STATE_WAIT_FOR_CONFIG;
            } else {
                appData.state = APP_STATE_INIT;
            }

            break;
        }

            /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            Nop();
            Nop();
            appData.state = APP_STATE_INIT;

            break;
        }
    }
}

void APP_CANFDSPI_Init()
{
    // Reset device
    DRV_CANFDSPI_Reset(DRV_CANFDSPI_INDEX_0);

    // Enable ECC and initialize RAM
    DRV_CANFDSPI_EccEnable(DRV_CANFDSPI_INDEX_0);

    if (!ramInitialized) {
        DRV_CANFDSPI_RamInit(DRV_CANFDSPI_INDEX_0, 0xff);
        ramInitialized = true;
    }

    // Configure device
    DRV_CANFDSPI_ConfigureObjectReset(&config);
    config.IsoCrcEnable = 1;
    config.StoreInTEF = 0;

    DRV_CANFDSPI_Configure(DRV_CANFDSPI_INDEX_0, &config);

    // Setup TX FIFO
    DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
    txConfig.FifoSize = 7;
    txConfig.PayLoadSize = CAN_PLSIZE_64;
    txConfig.TxPriority = 1;

    DRV_CANFDSPI_TransmitChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, &txConfig);

    // Setup RX FIFO
    DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig);
    rxConfig.FifoSize = 15;
    rxConfig.PayLoadSize = CAN_PLSIZE_64;

    DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, &rxConfig);

    // Setup RX Filter
    fObj.word = 0;
    fObj.bF.SID = 0xda;
    fObj.bF.EXIDE = 0;
    fObj.bF.EID = 0x00;

    DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &fObj.bF);

    // Setup RX Mask
    mObj.word = 0;
    mObj.bF.MSID = 0x0;
    mObj.bF.MIDE = 1; // Only allow standard IDs
    mObj.bF.MEID = 0x0;
    DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &mObj.bF);

    // Link FIFO and Filter
    DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, APP_RX_FIFO, true);

    // Setup Bit Time
    DRV_CANFDSPI_BitTimeConfigure(DRV_CANFDSPI_INDEX_0, selectedBitTime, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M);

    // Setup Transmit and Receive Interrupts
    DRV_CANFDSPI_GpioModeConfigure(DRV_CANFDSPI_INDEX_0, GPIO_MODE_INT, GPIO_MODE_INT);
    DRV_CANFDSPI_TransmitChannelEventEnable(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT);
    DRV_CANFDSPI_ReceiveChannelEventEnable(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT);
    DRV_CANFDSPI_ModuleEventEnable(DRV_CANFDSPI_INDEX_0, CAN_TX_EVENT | CAN_RX_EVENT);

    // Select Normal Mode
    //DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_0, CAN_NORMAL_MODE);
    
    DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_0, CAN_EXTERNAL_LOOPBACK_MODE);
}

void APP_TransmitMessageQueue()
{
    APP_LED_Set(APP_TX_LED);

    uint8_t attempts = MAX_TXQUEUE_ATTEMPTS;

    // Check if FIFO is not full
    do {
#ifdef APP_USE_TX_INT
        Delay_us(50);
#else
        DRV_CANFDSPI_TransmitChannelEventGet(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, &txFlags);
#endif
        if (attempts == 0) {
            Nop();
            Nop();
            DRV_CANFDSPI_ErrorCountStateGet(DRV_CANFDSPI_INDEX_0, &tec, &rec, &errorFlags);
            return;
        }
        attempts--;
    }
#ifdef APP_USE_TX_INT
    while (!APP_TX_INT());
#else
    while (!(txFlags & CAN_TX_FIFO_NOT_FULL_EVENT));
#endif

    // Load message and transmit
    uint8_t n = DRV_CANFDSPI_DlcToDataBytes(txObj.bF.ctrl.DLC);

    DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, &txObj, txd, n, true);

    APP_LED_Clear(APP_TX_LED);
}

APP_STATES APP_ReceiveMessage_Tasks()
{
    APP_STATES nextState;
    uint8_t i;

    // Normally we got to APP_STATE_PAYLOAD
    nextState = APP_STATE_PAYLOAD;

    // Check if FIFO is not empty
#ifdef APP_USE_RX_INT
    if (APP_RX_INT()) {
#else
    DRV_CANFDSPI_ReceiveChannelEventGet(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, &rxFlags);
    if (rxFlags & CAN_RX_FIFO_NOT_EMPTY_EVENT) {
#endif
        //        APP_LED_Set(APP_RX_LED);

        // Get message
        DRV_CANFDSPI_ReceiveMessageGet(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, &rxObj, rxd, MAX_DATA_BYTES);

        switch (rxObj.bF.id.SID) {
            case TX_REQUEST_ID:

                // Check for TX request command
                Nop();
                Nop();
                txObj.bF.id.SID = TX_RESPONSE_ID;

                txObj.bF.ctrl.DLC = rxObj.bF.ctrl.DLC;
                txObj.bF.ctrl.IDE = rxObj.bF.ctrl.IDE;
                txObj.bF.ctrl.BRS = rxObj.bF.ctrl.BRS;
                txObj.bF.ctrl.FDF = rxObj.bF.ctrl.FDF;

                for (i = 0; i < MAX_DATA_BYTES; i++) txd[i] = rxd[i];

                APP_TransmitMessageQueue();
                break;

            case LED_STATUS_ID:
                // Check for LED command
                APP_LED_Write(rxd[0]);
                break;

            case BITTIME_SET_ID:
                // Check for Bit Time Setup command
                Nop();
                Nop();
                selectedBitTime = rxd[0];
                nextState = APP_STATE_REQUEST_CONFIG;
                break;

            case BUTTON_STATUS_ID:
                // Check for Button Status command
                // This can be used to test two EVBs without a CAN tool

                // S1 turns on D1&D2
                if (rxd[0] & 0x01) {
                    APP_LED_Set(APP_LED_D1);
                    APP_LED_Set(APP_LED_D2);
                } else {
                    APP_LED_Clear(APP_LED_D1);
                    APP_LED_Clear(APP_LED_D2);
                }

                // S2 turns on D3&D4
                if (rxd[0] & 0x02) {
                    APP_LED_Set(APP_LED_D3);
                    APP_LED_Set(APP_LED_D4);
                } else {
                    APP_LED_Clear(APP_LED_D3);
                    APP_LED_Clear(APP_LED_D4);
                }

                // S3 turns on D5&D6
                if (rxd[0] & 0x04) {
                    APP_LED_Set(APP_LED_D5);
                    APP_LED_Set(APP_LED_D6);
                } else {
                    APP_LED_Clear(APP_LED_D5);
                    APP_LED_Clear(APP_LED_D6);
                }

                // S1 turns on D7&D8
                if (rxd[0] & 0x08) {
                    APP_LED_Set(APP_LED_D7);
                    APP_LED_Set(APP_LED_D8);
                } else {
                    APP_LED_Clear(APP_LED_D7);
                    APP_LED_Clear(APP_LED_D8);
                }
                break;

            case PAYLOAD_ID:
                // Check for Payload command
                Nop();
                Nop();
                payload.On = rxd[0];
                payload.Dlc = rxd[1];
                if (rxd[2] == 0) payload.Mode = true;
                else payload.Mode = false;
                payload.Counter = 0;
                payload.Delay = rxd[3];
                payload.BRS = rxd[4];

                break;

            case BITTIME_CFG_GET_ID:
                APP_TransmitBitTimeConfig(BITTIME_CFG_125K_ID);
                APP_TransmitBitTimeConfig(BITTIME_CFG_250K_ID);
                APP_TransmitBitTimeConfig(BITTIME_CFG_500K_ID);
                APP_TransmitBitTimeConfig(BITTIME_CFG_1M_ID);
                break;
        }
    }

    //    APP_LED_Clear(APP_RX_LED);

    return nextState;
}

void APP_PayLoad_Tasks()
{
    static uint8_t delayCount = 0;
    uint8_t i, n;
    
    payload.On = 1;
    payload.Mode = 0;
    
    payload.Dlc = CAN_DLC_64;

    // Send payload?
    if (payload.On) {
        // Delay transmission
        if (delayCount == 0) {
            delayCount = payload.Delay;

            // Prepare data
            Nop();
            Nop();
            txObj.bF.id.SID = TX_RESPONSE_ID;

            //txObj.bF.ctrl.DLC = payload.Dlc;
            txObj.bF.ctrl.DLC = 0xF;
            txObj.bF.ctrl.IDE = 0;
            //txObj.bF.ctrl.BRS = payload.BRS;
            txObj.bF.ctrl.BRS = 1;
            txObj.bF.ctrl.FDF = 1;

            n = DRV_CANFDSPI_DlcToDataBytes((CAN_DLC) payload.Dlc);

            if (payload.Mode) {
                // Random data
                for (i = 0; i < n; i++) txd[i] = rand() & 0xff;
            } else {
                // Counter
                for (i = 0; i < n; i++) 
                {txd[i] = i;}
            }

            APP_TransmitMessageQueue();
        } else {
            delayCount--;
        }
    } else {
        delayCount = 0;
    }
}

void APP_TransmitBitTimeConfig(uint32_t cfg)
{
    txObj.bF.id.SID = cfg;
    txObj.bF.ctrl.IDE = 0;
    txObj.bF.ctrl.BRS = 1;
    txObj.bF.ctrl.FDF = 1;

    uint8_t i;
    uint8_t n = 0;

    for (i = 0; i < 64; i++) txd[i] = 0;

    switch (cfg) {
        case BITTIME_CFG_125K_ID:
            txd[n] = BitTimeConfig125K[0];
            n++;
            txd[n] = BitTimeConfig125K[1];
            n++;

            for (i = 0; i < txd[0]; i++) {
                txd[n] = BitTimeConfig125K[i + 2] & 0xff;
                n++;
                txd[n] = (BitTimeConfig125K[i + 2] >> 8) & 0xff;
                n++;
            }

            txObj.bF.ctrl.DLC = DRV_CANFDSPI_DataBytesToDlc(n);
            APP_TransmitMessageQueue();

            break;

        case BITTIME_CFG_250K_ID:
            txd[n] = BitTimeConfig250K[0];
            n++;
            txd[n] = BitTimeConfig250K[1];
            n++;

            for (i = 0; i < txd[0]; i++) {
                txd[n] = BitTimeConfig250K[i + 2] & 0xff;
                n++;
                txd[n] = (BitTimeConfig250K[i + 2] >> 8) & 0xff;
                n++;
            }

            txObj.bF.ctrl.DLC = DRV_CANFDSPI_DataBytesToDlc(n);
            APP_TransmitMessageQueue();

            break;

        case BITTIME_CFG_500K_ID:
            txd[n] = BitTimeConfig500K[0];
            n++;
            txd[n] = BitTimeConfig500K[1];
            n++;

            for (i = 0; i < txd[0]; i++) {
                txd[n] = BitTimeConfig500K[i + 2] & 0xff;
                n++;
                txd[n] = (BitTimeConfig500K[i + 2] >> 8) & 0xff;
                n++;
            }

            txObj.bF.ctrl.DLC = DRV_CANFDSPI_DataBytesToDlc(n);
            APP_TransmitMessageQueue();

            break;

        case BITTIME_CFG_1M_ID:
            txd[n] = BitTimeConfig1M[0];
            n++;
            txd[n] = BitTimeConfig1M[1];
            n++;

            for (i = 0; i < txd[0]; i++) {
                txd[n] = BitTimeConfig1M[i + 2] & 0xff;
                n++;
                txd[n] = (BitTimeConfig1M[i + 2] >> 8) & 0xff;
                n++;
            }

            txObj.bF.ctrl.DLC = DRV_CANFDSPI_DataBytesToDlc(n);
            APP_TransmitMessageQueue();

            break;
    }
}

/*******************************************************************************
 End of File
 */
