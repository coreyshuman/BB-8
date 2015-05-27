#include <plib.h>
#include "HardwareProfile.h"
#include "TCPIPConfig.h"
#include "./USB/usb_function_cdc.h"

void _mon_putc(char c);

const unsigned char CharacterArray[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

char USB_In_Buffer[64];
char USB_Out_Buffer[64];

char UART_RX_Buffer[64];
BYTE UART_RX_StartPtr;
BYTE UART_RX_EndPtr;

void ConsoleInit(void)
{
    //OpenUART2A(UART_EN, (1 << 12)|UART_TX_ENABLE, (SYS_FREQ/(1<<mOSCGetPBDIV())/16)/BAUD_RATE-1);
    UARTConfigure(UART2A,UART_ENABLE_PINS_TX_RX_ONLY|UART_ENABLE_HIGH_SPEED);
    UARTSetFifoMode(UART2A, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(UART2A, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART2A, GetPeripheralClock(), BAUD_RATE);
    UARTEnable(UART2A, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    // Configure UART RX Interrupt
    mU2AEIntEnable(1);
    INTEnable(INT_SOURCE_UART_RX(UART2A), INT_ENABLED);
    INTSetVectorPriority(INT_VECTOR_UART(UART2A), INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_VECTOR_UART(UART2A), INT_SUB_PRIORITY_LEVEL_0);

    // ring buffer init
    UART_RX_StartPtr = UART_RX_EndPtr = 0;

    //setbuf(stdout, NULL );
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
    if(UART_RX_StartPtr < UART_RX_EndPtr)
    {
        return (UART_RX_EndPtr - UART_RX_StartPtr);
    }
    else
    {
        (64 - UART_RX_EndPtr + UART_RX_StartPtr);
    }
}

void ConsolePut(BYTE c)
{
     while (!UARTTransmitterIsReady(UART2A));
       UARTSendDataByte(UART2A, c);
     while (!UARTTransmissionHasCompleted(UART2A));

//    if(USBUSARTIsTxTrfReady())
//    {
//	putUSBUSART(&c,1);
//    }
}

//override standard printf output
void _mon_putc(char c)
{
    ConsolePut(c);
}

void PrintChar(BYTE toPrint)
{
    BYTE PRINT_VAR;
    PRINT_VAR = toPrint;
    toPrint = (toPrint>>4)&0x0F;
    ConsolePut(CharacterArray[toPrint]);
    toPrint = (PRINT_VAR)&0x0F;
    ConsolePut(CharacterArray[toPrint]);
    return;
}

void PrintDec(BYTE toPrint)
{
    ConsolePut(CharacterArray[toPrint/10]);
    ConsolePut(CharacterArray[toPrint%10]);
}

void ConsolePutROMString(const char* str)
{
    BYTE c;

    while( (c = *str++) )
        ConsolePut(c);
}

void ConsoleWriteBuffer(BYTE *buf, unsigned int length)
{
	unsigned int i = 0;
	while(i<length)
	{
            ConsolePut(buf[i++]);
	}
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