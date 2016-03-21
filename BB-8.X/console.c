#include <plib.h>
#include "HardwareProfile.h"
#include "TCPIPConfig.h"
#include "./USB/usb_function_cdc.h"

void _mon_putc(char c);

const unsigned char CharacterArray[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

char USB_In_Buffer[64];
char USB_Out_Buffer[64];



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

// send to USB serial
void debug(char* str, ...)
{
    va_list arglist;
    char PrintStr[128];

    int NumChars;

    va_start(arglist,str);

    NumChars = vsnprintf(PrintStr,128, str,arglist);

    va_end(arglist);

    // Now send it out the serial port one byte at a time
    if(NumChars > 0 && NumChars < 128)
    {
        if(USBUSARTIsTxTrfReady())
        {
            putUSBUSART(PrintStr,NumChars);
        }
    }
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

