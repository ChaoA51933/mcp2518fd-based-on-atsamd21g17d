/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes

#include <xc.h>
#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"
#include "canfd.h"



// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************


int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    CANFDSPI_Init();
    
    SYSTICK_TimerStart ();

    while ( true )
    {
        
        SYSTICK_DelayMs (1000);
        
        CANFDSPI_Test();
    }
}


/*******************************************************************************
 End of File
*/

