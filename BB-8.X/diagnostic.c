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

enum SM_DIAG {
    SM_DIAG_CLEAR = 0,
    SM_DIAG_UPDATE,
    SM_DIAG_WRITE

} sm;

DWORD debugMap;
enum DIAG_MOD debugModule;

static double pry[3];

void OledReceiverBar(int idx, int val);

void DiagInit(void)
{
    debugMap = 0ul;
    pry[0] = pry[1] = pry[2] = 0;
    sm = SM_DIAG_CLEAR;
    OLED_clear();
}



void DiagProcess(void)
{
    static QWORD tick = TICK_SECOND;
    static BOOL diagEnabled = FALSE;

    SetModule(MOD_DIAG);
    if(!diagEnabled && TickGet() - tick >= TICK_SECOND*3)
    {
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
                OLED_text(5,7,str,1);
                sprintf(str, "Y%6.1f", (double)pry[2]);
                OLED_text(5,15,str,1);
                // update receiver readings
                for(i=1; i<=8; i++)
                {
                    OledReceiverBar(i,ReceiverGetPulse(i));
                }
                sm ++;
                break;
            case SM_DIAG_WRITE: OLED_write(OLED_ADDR); sm = 0; break;

        }
        tick = TickGet();
    }
}

void PrintAccelToOled(double vpry[])
{
    pry[0] = vpry[0];
    pry[1] = vpry[1];
    pry[2] = vpry[2];
}

void OledReceiverBar(int idx, int val)
{
    char bar[10] = ".........";

    if(val < 950)
    {
        bar[0] = '<';
    }
    else if(val > 2050)
    {
        bar[8] = '>';
    }
    else
    {
        val += 55;
        val /= 111;
        val -= 9;
        bar[val] = '|';
    }

    OLED_text(70,idx*8-9,bar,1);
}