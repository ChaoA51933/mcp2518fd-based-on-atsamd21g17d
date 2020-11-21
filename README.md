# mcp2518fd-based-on-atsamd21g17d
Objective:
==========
This repository contains an example of mcp2518fd operation with the SPI interface to MCU ATSAMD21G17D. Mainly of this code is from Microchip's example (using another mcu).

Related Documentation:
=====================
1) [MCP2518FD-Data-Sheet-20006027A](https://ww1.microchip.com/downloads/en/DeviceDoc/MCP2518FD-Data-Sheet-20006027A.pdf)
2) [MCP25XXFD-CAN-FD-Controller-Module-Family-Reference-Manual-DS20005678E](https://ww1.microchip.com/downloads/en/DeviceDoc/MCP25XXFD-CAN-FD-Controller-Module-Family-Reference-Manual-DS20005678E.pdf)
3) [SAM_D21_DA1_Family_DataSheet_DS40001882F](https://ww1.microchip.com/downloads/en/DeviceDoc/SAM_D21_DA1_Family_DataSheet_DS40001882F.pdf)

Software Used:
==============
1) [MPLAB® X IDE 5.45](microchip.com/mplab/mplab-x-ide)
2) [MPLAB® XC16 2.50](microchip.com/mplab/compilers)
3) [MPLAB® Harmony Configurator (MHC) 3.6.1](https://www.microchip.com/mplab/mplab-harmony)
4) CMSIS 5.6.0
5) SAMD21_DFP 3.3.98

Hardware Used:
=============
1) [MCP2518FD CLICK (PID: MIKROE-3060)](https://www.mikroe.com/mcp2518fd-click)
2) [SAMD21 Curiosity Nano Evaluation Kit (Part Number: DM320119)](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/DM320119)
3) [Curiosity Nano Base for Click boards (Part Number: AC164162)](https://www.microchip.com/developmenttools/ProductDetails/AC164162)

Setup:
======
The test boards I do the test is as follow.

![image](https://github.com/ChaoA51933/mcp2518fd-based-on-atsamd21g17d/tree/main/images/hardware_platform.jpg) 

The following picture describe the interface of hardware and software.

![image](https://github.com/ChaoA51933/mcp2518fd-based-on-atsamd21g17d/tree/main/images/mcu_mcp2518fd.png) 
![image](https://github.com/ChaoA51933/mcp2518fd-based-on-atsamd21g17d/tree/main/images/mcu_ata6563.png) 

The spi clk between ATSAMD21G17D and mcp25625 is 1MHz, spi mode is 0. All codes except the following files are generated by mhc.
1) drv_canfdspi_register.h
2) drv_canfdspi_defines.h
3) drv_canfdspi_api.h
4) drv_canfdspi_api.c
5) canfd.h
6) canfd.c

The ossicilator for mcp2518fd is 40M, and the bitrate of Normal CAN is 500Kbps, Data CAN is 2Mbps.



Operation:
==========
When run this example, MCU will transmit message formate like ID is from 0x300 to 0x30f, and DLC is 64 byte, but will just receive message with ID from 0x300 to 0x307. Test result is as follow.

![image](https://github.com/ChaoA51933/mcp2518fd-based-on-atsamd21g17d/tree/main/images/test_result.jpg) 

Summary:
========
Actually, this is one driver example for mcp2518fd, you could use these code for other MCU's use.
