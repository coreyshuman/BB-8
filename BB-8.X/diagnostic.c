/********************************************************************
 FileName:      diagnostic.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Collect and output diagnostic and debug data.

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
  1.0   Initial release                            Corey Shuman 9/11/15


********************************************************************/
#include <plib.h>
#include <stdio.h>
#include "GenericTypeDefs.h"
#include "console.h"
#include "TCPIP Stack/Tick.h"
#include "time.h"
#include "HardwareProfile.h"
#include "OLED_driver.h"
#include "receiver.h"
#include "diagnostic.h"
#include "navigation_controller.h"

enum SM_DIAG {
    SM_DIAG_CLEAR = 0,
    SM_DIAG_UPDATE,
    SM_DIAG_WRITE

} sm;

DWORD debugMap;
enum DIAG_MOD debugModule;

static double pry[3];
BOOL dArmed;
BOOL dAccelEnabled;

void OledReceiverBar(int idx, int val);

void DiagInit(void)
{
    debugMap = 0ul;
    pry[0] = pry[1] = pry[2] = 0;
    dArmed = FALSE;
    dAccelEnabled = FALSE;
    sm = SM_DIAG_CLEAR;
    
}



void DiagProcess(void)
{
    static QWORD tick = TICK_SECOND;
    static BOOL diagEnabled = FALSE;

    SetModule(MOD_DIAG);
    if(!diagEnabled && TickGet() - tick >= TICK_SECOND*5)
    {
        OLED_clear();
        OLED_write(OLED_ADDR);
        tick = TickGet();
        diagEnabled = TRUE;
    }
    else if(diagEnabled && TickGet() - tick >= TICK_SECOND/20)
    {
        // update oled
        char str[40];
        int i;

        switch(sm)
        {
            case SM_DIAG_CLEAR: OLED_clear(); sm++; break;
            case SM_DIAG_UPDATE:
                // update accelerometer readings
                sprintf(str, "P%6.1f", (double)pry[0]);
                OLED_text(5,0,str,1);
                sprintf(str, "R%6.1f", (double)pry[1]);
                OLED_text(5,8,str,1);
                sprintf(str, "Y%6.1f", (double)pry[2]);
                OLED_text(5,16,str,1);
                if(dArmed)
                    OLED_text(5,24, "ARMED ",1);
                else
                    OLED_text(5,24, "DISARM",1);
                if(dAccelEnabled)
                    OLED_text(5,32, "ACCEL ",1);
                else
                    OLED_text(5,32, "STATIC",1);
                // update receiver readings
                for(i=1; i<=8; i++)
                {
                    OledReceiverBar(i,NavigationGetTelemetry(i));
                }
                sm ++;
                break;
            case SM_DIAG_WRITE: OLED_write(OLED_ADDR); sm = 0; break;

        }
        tick = TickGet();
    }
}

void PrintAccelToOled(double vpry[], BOOL armed, BOOL accelEnabled)
{
    pry[0] = vpry[0];
    pry[1] = vpry[1];
    pry[2] = vpry[2];
    dArmed = armed;
    dAccelEnabled = accelEnabled;
}

/* Take value of 0 - 255 and map to 9 bar graph*/
void OledReceiverBar(int idx, int val)
{
    char bar[10] = ".........";

    if(val < 23)
    {
        bar[0] = '<';
    }
    else if(val > 230)
    {
        bar[8] = '>';
    }
    else
    {
        val /= 23;
        val -= 1;
        bar[val] = '|';
    }

    OLED_text(70,idx*8-9,bar,1);
}