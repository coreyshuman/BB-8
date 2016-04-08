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

// Functions
void SerialReset(void);
int SerialFindHandler(char* command);
void SerialExecuteHandler(BYTE handle);
enum SERIAL_RESPONSE SerialProcessInput(void);
void UART_TX_PutByte(char c);
void UART_RX_PutByte(char c);
char UART_RX_GetByte(void);
unsigned char UART_RX_GetCount(void);
void UART_RX_ClearBuffer(void);



// Enums
enum SERIAL_RESPONSE {
    SR_NONE = 0,
    SR_INPROG,
    SR_GOOD,
    SR_ERROR_TMO,
    SR_ERROR_OVF
};

// Structs
struct HandlerDef {
  char command[MAX_COMMAND_LENGTH];
  BYTE maxArguments;
  void (*func)(char[][MAX_ARGUMENT_LENGTH] , int);
};

// Constants
const char DELIM1 = ' ';
const char DELIM2 = ',';
const char ENDL = '\n';

// Variables
char UART_RX_Buffer[64];
BYTE UART_RX_StartPtr;
BYTE UART_RX_EndPtr;

DWORD serTimeoutTick;

// serial state variable
// - state 0 is 'get command'
// - state 1 - maxarg is 'get argument'
BYTE serState;

// input command buffers
int  serIdx;
char serCommand[MAX_COMMAND_LENGTH];
char serArguments[MAX_ARGUMENT_COUNT][MAX_ARGUMENT_LENGTH];
BYTE serArgumentCount;

// variable to hold command handler definitions
struct HandlerDef serHandlers[MAX_HANDLERS];
BYTE serHandlerCount;

void SerialInit(void)
{
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

    serHandlerCount = 0;
    serTimeoutTick = 0;
    
    SerialReset();
    UART_RX_ClearBuffer();

    // cts debug
    enableDiagFilter(DBG_SERIAL);
    
}



/* Call periodically to run the handler */
void SerialProcess(void)
{
    SetModule(MOD_SERIAL);

    enum SERIAL_RESPONSE srx = SerialProcessInput();

    if(srx == SR_GOOD)
    {
        int handle = SerialFindHandler(serCommand);
        if(handle > -1)
        {
            if(isDiagFilterOn(DBG_SERIAL)) {
                debug("SER EXEC %d\r\n", handle);
            }
            SerialExecuteHandler(handle);
        }
        else
        {
            if(isDiagFilterOn(DBG_SERIAL)) {
                debug("SER EXEC NOT FOUND\r\n");
            }
        }

        SerialReset();
    }
    else if(srx == SR_ERROR_TMO)
    {
        // error
        if(isDiagFilterOn(DBG_SERIAL)) {
            debug("SER TMO\r\n");
        }
        SerialReset();
    }
    else if(srx == SR_ERROR_OVF)
    {
        // error
        if(isDiagFilterOn(DBG_SERIAL)) {
            debug("SER OVF\r\n");
        }
        SerialReset();
        UART_RX_ClearBuffer();
    }
    
}

 
/* Reads serial input and parses commands and arguments */
enum SERIAL_RESPONSE SerialProcessInput()
{
    char inByte;
    enum SERIAL_RESPONSE ret = SR_NONE;
    
    while(UART_RX_GetCount() > 0)
    {
        inByte = UART_RX_GetByte();
        serTimeoutTick = TickGet();
        ret = SR_INPROG;

        if(isDiagFilterOn(DBG_SERIAL)) {
            mLED_4_Toggle();
            if(inByte >= 0x20)
                debug("(%c)", inByte);
            else
                debug("[%02X]", inByte);
        }

        // process deliminators and update state
        if(inByte == DELIM1 || inByte == DELIM2)
        {
            serState ++;
            serIdx = 0;
            continue;
        }
        else if(inByte == ENDL)
        {
            serArgumentCount = serState;
            if(serArgumentCount > MAX_ARGUMENT_COUNT)
            {
                serArgumentCount = MAX_ARGUMENT_COUNT;
            }
            ret = SR_GOOD;
            break;
        }

        // store commands and arguments
        if(serState == 0)
        {
            if(serIdx >= MAX_COMMAND_LENGTH)
            {
                ret = SR_ERROR_OVF;
                break;
            }
            serCommand[serIdx++] = inByte;
        }
        else if(serState <= MAX_ARGUMENT_COUNT)
        {
            if(serIdx >= MAX_ARGUMENT_LENGTH)
            {
                ret = SR_ERROR_OVF;
                break;
            }
            serArguments[serState - 1][serIdx++] = inByte;
        }
    }

    if((serIdx > 0 || serState > 0) && TickGet() - serTimeoutTick > SER_TIMEOUT_INTERVAL) {
        ret = SR_ERROR_TMO;
    }
    
    return ret;
}

/* Called to run a located handler */
void SerialExecuteHandler(BYTE handle)
{
    if(handle >= 0 && handle < MAX_HANDLERS)
    {
        serHandlers[handle].func(serArguments, serArgumentCount);
    }
}

/* Used to match a command to a handler */
int SerialFindHandler(char* command)
{
    int i;
    int ret = -1;

    for(i = 0; i < serHandlerCount; i++)
    {
        struct HandlerDef *thisHandler = &serHandlers[i];

        if(strncmp(thisHandler->command, command, MAX_COMMAND_LENGTH) == 0)
        {
            ret = i;
            break;
        }
    }
    return ret;
}

/* Add a command handler function to handler pool */
int SerialAddHandler(char* command, BYTE maxArguments, void (*func)(char[][MAX_ARGUMENT_LENGTH], int))
{
    if(serHandlerCount >= MAX_HANDLERS)
    {
        return -1;
    }
    struct HandlerDef *thisHandler = &serHandlers[serHandlerCount];

    strncpy(thisHandler->command, command, MAX_COMMAND_LENGTH);
    thisHandler->maxArguments = maxArguments;
    thisHandler->func = func;
    return serHandlerCount++;
}

void SerialReset()
{
    serIdx = 0;
    serState = 0;
    serArgumentCount = 0;
    memset(serCommand, 0, MAX_COMMAND_LENGTH);
    memset(serArguments, 0, MAX_ARGUMENT_COUNT * (MAX_ARGUMENT_LENGTH));
}

// CTS TODO - use interrupts for UART TX as well!
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
