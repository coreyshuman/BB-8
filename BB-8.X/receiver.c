#include <plib.h>
#include "HardwareProfile.h"

// Receiver.c
// Read and process radio receiver pulses

volatile WORD rcValue[RX_INPUT_COUNT];
volatile WORD rcTemp[RX_INPUT_COUNT];
volatile char rcLastState[RX_INPUT_COUNT];

// Init Receiver
void ReceiverInit(void)
{
    int i;
    OpenTimer2(T2_ON | T2_PS_1_1, RX_TIMER_PERIOD);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_3 | T2_INT_SUB_PRIOR_3);

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
void __attribute((interrupt(ipl3), vector(_TIMER_2_VECTOR), nomips16)) _T2Interrupt(void)
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
    IFS0CLR = _IFS0_T2IF_MASK;
}

WORD Receiver_GetPulse(unsigned int chan)
{
    if(chan < 1 || chan > RX_INPUT_COUNT)
        return 0;

    return rcValue[chan-1];
}