/********************************************************************
 FileName:      receiver.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Driver captures up to 8 channels of RC pulse-width modulation.

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
  1.0   Initial release                            Corey Shuman 5/26/15


********************************************************************/

#include <plib.h>
#include "HardwareProfile.h"

// Resolution of 10us, so interrupt every 10us
//#define RX_TIMER_PERIOD   800
//#define RX_USEC_PER_CYCLE  10

#define RX_USEC_PER_CYCLE  5
#define RX_TIMER_PERIOD   80000000l / (1000000l / RX_USEC_PER_CYCLE)


volatile WORD rcValue[RX_INPUT_COUNT];
volatile WORD rcTemp[RX_INPUT_COUNT];
volatile char rcLastState[RX_INPUT_COUNT];

// Init Receiver
void ReceiverInit(void)
{
    int i;
    OpenTimer4(T4_ON | T4_PS_1_1, RX_TIMER_PERIOD);
    ConfigIntTimer4(T4_INT_ON | T4_INT_PRIOR_2 | T4_INT_SUB_PRIOR_3);

    for(i=0; i<RX_INPUT_COUNT; i++)
    {
        rcValue[i] = 1500;
        rcTemp[i] = 0;
    }

    //CTS DEBUG
    TEST_LED_TRIS = OUTPUT_PIN;

#if RX_INPUT_COUNT >= 1
    RX_INPUT_1_TRIS = INPUT_PIN;
#endif
#if RX_INPUT_COUNT >= 2
    RX_INPUT_2_TRIS = INPUT_PIN;
#endif
#if RX_INPUT_COUNT >= 3
    RX_INPUT_3_TRIS = INPUT_PIN;
#endif
#if RX_INPUT_COUNT >= 4
    RX_INPUT_4_TRIS = INPUT_PIN;
#endif
#if RX_INPUT_COUNT >= 5
    RX_INPUT_5_TRIS = INPUT_PIN;
#endif
#if RX_INPUT_COUNT >= 6
    RX_INPUT_6_TRIS = INPUT_PIN;
#endif
#if RX_INPUT_COUNT >= 7
    RX_INPUT_7_TRIS = INPUT_PIN;
#endif
#if RX_INPUT_COUNT >= 8
    RX_INPUT_8_TRIS = INPUT_PIN;
#endif
}

// Interrupt Timer Process
void __attribute((interrupt(ipl2), vector(_TIMER_4_VECTOR), nomips16)) _T4Interrupt(void)
{
    int chan = 0;
#if RX_INPUT_COUNT >= 1
    if(!rcLastState[chan] && RX_INPUT_1_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
        TEST_LED_IO = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_1_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_1_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
        TEST_LED_IO = 1;
    }
#endif
#if RX_INPUT_COUNT >= 2
    chan = 1;
    if(!rcLastState[chan] && RX_INPUT_2_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_2_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_2_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
    }
#endif
#if RX_INPUT_COUNT >= 3
    chan = 2;
    if(!rcLastState[chan] && RX_INPUT_3_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_3_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_3_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
    }
#endif
#if RX_INPUT_COUNT >= 4
    chan = 3;
    if(!rcLastState[chan] && RX_INPUT_4_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_4_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_4_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
    }
#endif
#if RX_INPUT_COUNT >= 5
    chan = 4;
    if(!rcLastState[chan] && RX_INPUT_5_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_5_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_5_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
    }
#endif
#if RX_INPUT_COUNT >= 6
    chan = 5;
    if(!rcLastState[chan] && RX_INPUT_6_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_6_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_6_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
    }
#endif
#if RX_INPUT_COUNT >= 7
    chan = 6;
    if(!rcLastState[chan] && RX_INPUT_7_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_7_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_7_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
    }
#endif
#if RX_INPUT_COUNT >= 8
    chan = 7;
    if(!rcLastState[chan] && RX_INPUT_8_IO)
    {
        rcLastState[chan] = 1;
        rcTemp[chan] = 0;
    }
    else if(rcLastState[chan] && RX_INPUT_8_IO)
    {
        rcTemp[chan] += RX_USEC_PER_CYCLE;
    }
    else if(rcLastState[chan] && !RX_INPUT_8_IO)
    {
        rcLastState[chan] = 0;
        rcValue[chan] = rcTemp[chan];
    }
#endif

    // Reset interrupt flag
    IFS0CLR = _IFS0_T4IF_MASK;
}

WORD ReceiverGetPulse(WORD chan)
{
    if(chan < 1 || chan > RX_INPUT_COUNT)
        return 0;

    return rcValue[chan-1];
}