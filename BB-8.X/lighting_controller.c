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
#include "TCPIP Stack/Tick.h"

// definitions
#define TOPBOARD_ADDRESS    0x33
#define TOPBOARD_LED_REGISTER   0x11
#define TOPBOARD_CMD_UPDATE_LED 0x01
#define LED_DATA_COUNT          15

// enums
enum LED_STATE {
    LSM_WAIT,
    LSM_TRAN,
    LSM_SEND
};

// static variables
BYTE ledMap[LED_DATA_COUNT];
/* This is our LED colors: Orange, Red dim, Blue dim, Red, Blue Light */
BYTE ledColors[] = {30,90,0, 0,40,0, 0,0,30, 5,85,5, 64,30,64};
DWORD ledTick;
BYTE updateRate;   // 1 - 5 seconds update speed
BYTE smLed;

// function definitions
void TopBoard_Command(BYTE cmd);
void TopBoard_SendData( BYTE reg, BYTE data[], WORD len);
void UpdateTransition(BYTE data);

void LightingInit(void)
{
    int i;
    for(i=0; i<LED_DATA_COUNT; i++)
    {
        ledMap[i] = ledColors[i];
    }
    updateRate = 5; // init slowest changes
    smLed = LSM_WAIT;
}

void LightingProcess(void)
{
    BYTE temp[3];
    int i;
    switch(smLed)
    {
        case LSM_WAIT:
            if(TickGet() - ledTick > TICK_SECOND * updateRate)
            {
                ledTick = TickGet();
                // update the rate based on movement speed
                // CTS TODO
                // shift colors
                
                temp[0] = ledMap[LED_DATA_COUNT-3];
                temp[1] = ledMap[LED_DATA_COUNT-2];
                temp[2] = ledMap[LED_DATA_COUNT-1];
                for(i=LED_DATA_COUNT-6; i >= 0; i-=3)
                {
                    ledMap[i+3] = ledMap[i];
                    ledMap[i+4] = ledMap[i+1];
                    ledMap[i+5] = ledMap[i+2];
                }
                ledMap[0] = temp[0];
                ledMap[1] = temp[1];
                ledMap[2] = temp[2];
                 
                // send color data
                SendLightingData();
                smLed = LSM_TRAN;
            }
            break;
        case LSM_TRAN:
            // send transistion data
            UpdateTransition(5);
            smLed = LSM_SEND;
            break;
            
        case LSM_SEND:
            // trigger update
            UpdateLighting();
            smLed = LSM_WAIT;
            break;
    }
}

/* Send local LED map to top board registers */
void SendLightingData(void)
{
    TopBoard_SendData(TOPBOARD_LED_REGISTER, ledMap, sizeof(ledMap));
}

/* Trigger top board to send register data to NeoPixels */
void UpdateLighting(void)
{
    TopBoard_Command(TOPBOARD_CMD_UPDATE_LED);
}

/* Send transition data to Top Board*/
void UpdateTransition(BYTE data)
{
    TopBoard_SendData(0x10, &data, 1);
}

/* Update local LED map data for a single LED */
void SetLedColor(BYTE n, BYTE r, BYTE g, BYTE b)
{
    if(n > 0 && n <= 5)
    {
        n--;
        BYTE index = n*3;
        ledMap[index] = g;
        ledMap[index+1] = r;
        ledMap[index+2] = b;
    }
    
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
