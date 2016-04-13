/********************************************************************
 FileName:      lighting_controller.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Controls lighting patterns for BB-8 Body. Lighting is a set of
 Neopixels connected to a top board that communicates with this
 MCU via I2C.



 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Description                                Name and date
  ----  -----------------------------------------  ----------------
  1.0   Initial release                            Corey Shuman 4/10/16


********************************************************************/
#include <plib.h>
#include "HardwareProfile.h"
#include "diagnostic.h"
#include "lighting_controller.h"
#include "device_i2c.h"

// definitions
#define TOPBOARD_ADDRESS    0x33

#define TOPBOARD_LED_REGISTER   0x11

#define TOPBOARD_CMD_UPDATE_LED 0x01

// static variables
BYTE ledMap[15];

// function definitions
void TopBoard_Command(BYTE cmd);
void TopBoard_SendData( BYTE reg, BYTE data[], WORD len);

void LightingInit(void)
{
    
}

void LightingProcess(void)
{
    
}

/* Send local LED map to top board registers */
void SendLightingData(void)
{
    TopBoard_SendData(TOPBOARD_LED_REGISTER, ledMap, sizeof(ledMap));
}

/* Trigger top board to send register data to NeoPixels */
void UpdateLighting(void)
{
    SendLightingData();
    TopBoard_Command(TOPBOARD_CMD_UPDATE_LED);
}

/* Update local LED map data for a single LED */
void SetLedColor(BYTE n, BYTE r, BYTE g, BYTE b)
{
    BYTE index = n*3;
    ledMap[index] = g;
    ledMap[index+1] = r;
    ledMap[index+2] = b;
}


// I2C Helper Functions
void TopBoard_Command(BYTE cmd)
{
    i2c_begin_transmission(I2C1, FALSE);
    i2c_send_address(I2C1, TOPBOARD_ADDRESS, 0);
    i2c_write_byte(I2C1, 0x00);
    i2c_write_byte(I2C1, cmd);
    i2c_end_transmission(I2C1);
}

void TopBoard_SendData( BYTE reg, BYTE data[], WORD len)
{
    int i;
    i2c_begin_transmission(I2C1, FALSE);
    i2c_send_address(I2C1, TOPBOARD_ADDRESS, 0);
    i2c_write_byte(I2C1, reg);
    for (i = 0; i < len; i++)
    {
        i2c_write_byte(I2C1, data[i]);
    }

    i2c_end_transmission(I2C1);
}
