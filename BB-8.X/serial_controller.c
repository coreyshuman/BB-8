/********************************************************************
 FileName:      serial_controller.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 XBEE serial controller for serial command processing.

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
  1.0   Initial release                            Corey Shuman 3/21/16


********************************************************************/
#include <plib.h>
//#include <stdio.h>
#include "HardwareProfile.h"
#include "TCPIP Stack/Tick.h"
#include "serial_controller.h"
#include "console.h"
#include "diagnostic.h"



enum SERIAL_RESPONSE {
    SR_NONE = 0,
    SR_GOOD = 1,
    SR_ERROR = 2,
	SR_PANIC = 3
};

char UART_RX_Buffer[64];
BYTE UART_RX_StartPtr;
BYTE UART_RX_EndPtr;

void UART_TX_PutByte(char c);
void UART_RX_PutByte(char c);
char UART_RX_GetByte(void);
unsigned char UART_RX_GetCount(void);
void UART_RX_ClearBuffer(void);
enum SERIAL_RESPONSE SerialGetResponse();



int  serIdx = 0;
char response[20];

int throttle;
int yaw;
int pitch;
int roll;



DWORD serTick;

void SerialInit(void)
{
    serTick = 0;
    
    //OpenUART2A(UART_EN, (1 << 12)|UART_TX_ENABLE, (SYS_FREQ/(1<<mOSCGetPBDIV())/16)/BAUD_RATE-1);
    UARTConfigure(UART2A,UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode(UART2A, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(UART2A, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART2A, GetPeripheralClock(), SERIAL_BAUD_RATE);
    UARTEnable(UART2A, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    // Configure UART RX Interrupt
    mU2AEIntEnable(1);
    INTEnable(INT_SOURCE_UART_RX(UART2A), INT_ENABLED);
    INTSetVectorPriority(INT_VECTOR_UART(UART2A), INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_VECTOR_UART(UART2A), INT_SUB_PRIORITY_LEVEL_0);

    // ring buffer init
    UART_RX_StartPtr = UART_RX_EndPtr = 0;

    //setbuf(stdout, NULL );

    throttle = yaw = pitch = roll = 0;

    // cts debug
    enableDiagFilter(DBG_SERIAL);
    
}

void SerialProc(void)
{
    char output[50];

    SetModule(MOD_SERIAL);

    if(TickGet() - serTick > 1*TICK_SECOND)
    {
        serTick = TickGet();
        UART_TX_PutByte('U');

        mLED_3_Toggle();
    }

    enum SERIAL_RESPONSE srx = SerialGetResponse();

    if(srx == SR_GOOD)
    {
        // expected input is XX,XX,XX,XX/n
        if(serIdx == 12)
        {
            response[2] = response[5] = response[8] = 0;
            throttle = strtol(&response[0], NULL, 16);
            yaw = strtol(&response[3], NULL, 16);
            pitch = strtol(&response[6], NULL, 16);
            roll = strtol(&response[9], NULL, 16);
            // debug data
            if(isDiagFilterOn(DBG_SERIAL)) {
                debug("throttle=%d, yaw=%d, pitch=%d, roll=%d\r\n", throttle, yaw, pitch, roll);
            }
        }
        else
        {
            // debug data
            
            if(isDiagFilterOn(DBG_SERIAL)) {
                sprintf(output, "SER len err=%d", serIdx);
                debug(output);
            }
        }

        serIdx = 0;
    }
    else if(srx == SR_ERROR)
    {
        // error
        if(isDiagFilterOn(DBG_SERIAL)) {
            debug("SER err\r\n");
        }
    }
    
}

enum SERIAL_RESPONSE SerialGetResponse()
{
    enum SERIAL_RESPONSE ret = SR_NONE;
    int rx = 0;
    static DWORD lastResponse = 0;

    while(UART_RX_GetCount() > 0)
    {
        response[serIdx] = UART_RX_GetByte();
        lastResponse = TickGet();

        if(isDiagFilterOn(DBG_SERIAL)) {
            mLED_4_Toggle();
            if(response[serIdx] >= 0x20)
                debug("(%c)", response[serIdx]);
            else
                debug("[%02X]", response[serIdx]);
        }
        
        if(response[serIdx] == '\n')
        {
            response[serIdx] = '\0';
            rx = TRUE;
            serIdx++; // need to increment here before break
            break;
        }

        serIdx ++;
    }

    if(serIdx >= sizeof(response))
    {
        serIdx = 0;
        UART_RX_ClearBuffer();
        if(isDiagFilterOn(DBG_SERIAL)) {
            debug("SER OVF");
        }
    }

    if(serIdx > 0 && TickGet() - lastResponse > TICK_SECOND / 10) {
        serIdx = 0;
        UART_RX_ClearBuffer();
        if(isDiagFilterOn(DBG_SERIAL)) {
            debug("SER TMO");
        }
    }

    if(rx)
    {
        ret = SR_GOOD;
    }

    
    return ret;
}

void UART_TX_PutByte(char c)
{
    while (!UARTTransmitterIsReady(UART2A));
    UARTSendDataByte(UART2A, c);
    while (!UARTTransmissionHasCompleted(UART2A));
}

void UART_RX_PutByte(char c)
{
    UART_RX_Buffer[UART_RX_EndPtr] = c;
    UART_RX_EndPtr ++;
    if(UART_RX_EndPtr >= 64)
        UART_RX_EndPtr = 0;
}

char UART_RX_GetByte(void)
{
    char c = UART_RX_Buffer[UART_RX_StartPtr];
    UART_RX_StartPtr ++;
    if(UART_RX_StartPtr >= 64)
        UART_RX_StartPtr = 0;
    return c;
}

unsigned char UART_RX_GetCount(void)
{
    if(UART_RX_StartPtr <= UART_RX_EndPtr)
    {
        return (UART_RX_EndPtr - UART_RX_StartPtr);
    }
    else
    {
        return (64 - UART_RX_StartPtr + UART_RX_EndPtr);
    }
}

void UART_RX_ClearBuffer(void)
{
    UART_RX_StartPtr = UART_RX_EndPtr;
}

void __ISR(_UART_2A_VECTOR, ipl4) _UART2AISRHandler(void) {
    // Is this an RX interrupt?
    if (INTGetFlag(INT_U2ARX)) {
        while(UARTReceivedDataIsAvailable(UART2A))
        {
            UART_RX_PutByte(UARTGetDataByte(UART2A));
            //ConsolePut(UARTGetDataByte(UART2A));
        }
        
        // handle overflow flag
        if (mU2AEGetIntFlag()) {
            U2ASTAbits.OERR = 0;
            mU2AEClearIntFlag();
        }
        // Clear the RX interrupt Flag
        INTClearFlag(INT_U2ARX);
    }

    // We don't care about TX interrupt
    if (INTGetFlag(INT_U2ARX)) {
        INTClearFlag(INT_U2ARX);
    }
}
