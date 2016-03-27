/********************************************************************
 FileName:      console.c
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
  2.0   Redesign for USB support                   Corey Shuman 3/27/16


********************************************************************/
#include <plib.h>
#include "HardwareProfile.h"
#include "TCPIPConfig.h"
#include "usb_support.h"
#include "diagnostic.h"

void _mon_putc(char c);

const unsigned char CharacterArray[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

char Console_In_Buffer[64];
char Console_Out_Buffer[128];
char USB_Out_Buffer[sizeof(Console_Out_Buffer)]; // buffer must not change while usb operation is on-going
unsigned int Console_RX_Start_Pointer;
unsigned int Console_RX_End_Pointer;
unsigned int Console_TX_Start_Pointer;
unsigned int Console_TX_End_Pointer;


/*********************************
 * Console buffer helpers
 *
 *********************************/
unsigned int ConsoleTxGetCount(void)
{
    if(Console_TX_Start_Pointer <= Console_TX_End_Pointer)
    {
        return (Console_TX_End_Pointer - Console_TX_Start_Pointer);
    }
    else
    {
        return (sizeof(Console_Out_Buffer) - Console_TX_End_Pointer + Console_TX_Start_Pointer);
    }
}

void ConsoleTxPutByte(char c)
{
    Console_Out_Buffer[Console_TX_End_Pointer] = c;
    Console_TX_End_Pointer ++;
    if(Console_TX_End_Pointer >= sizeof(Console_Out_Buffer))
        Console_TX_End_Pointer = 0;
}

char ConsoleTxGetByte(void)
{
    char c = Console_Out_Buffer[Console_TX_Start_Pointer];
    Console_TX_Start_Pointer ++;
    if(Console_TX_Start_Pointer >= sizeof(Console_Out_Buffer))
        Console_TX_Start_Pointer = 0;
    return c;
}

unsigned int ConsoleRxGetCount(void)
{
    if(Console_RX_Start_Pointer <= Console_RX_End_Pointer)
    {
        return (Console_RX_End_Pointer - Console_RX_Start_Pointer);
    }
    else
    {
        return (sizeof(Console_In_Buffer) - Console_RX_End_Pointer + Console_RX_Start_Pointer);
    }
}

void ConsoleRxPutByte(char c)
{
    Console_In_Buffer[Console_RX_End_Pointer] = c;
    Console_RX_End_Pointer ++;
    if(Console_RX_End_Pointer >= sizeof(Console_In_Buffer))
        Console_RX_End_Pointer = 0;
}

char ConsoleRxGetByte(void)
{
    char c = Console_In_Buffer[Console_RX_Start_Pointer];
    Console_RX_Start_Pointer ++;
    if(Console_RX_Start_Pointer >= sizeof(Console_In_Buffer))
        Console_RX_Start_Pointer = 0;
    return c;
}

/*************************************
 * Console process
 *
 *************************************/
void ConsoleInit(void)
{
    Console_RX_Start_Pointer = 0;
    Console_RX_End_Pointer = 0;
    Console_TX_Start_Pointer = 0;
    Console_TX_End_Pointer = 0;
}

void ConsoleProcess(void)
{
    int txCount;
    int i = 0;
    int j = 0;

    SetModule(MOD_CONS);

    // break if USB not ready
    if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1)) return;

    // push tx buffer data to usb
    txCount = ConsoleTxGetCount();
    if(txCount > 0 && USBUSARTIsTxTrfReady())
    {
        // unroll tx buffer
        j = txCount;
        while(j --)
            USB_Out_Buffer[i++] = ConsoleTxGetByte();

        putUSBUSART(USB_Out_Buffer, txCount);
    }
}

/*************************************
 * Console send public functions
 *
 *************************************/
void ConsoleSend(const char *buf, unsigned int length)
{
    unsigned int i = 0;
    while(i<length)
    {
        ConsoleTxPutByte(buf[i++]);
    }
}

void ConsolePut(const char c)
{
    ConsoleTxPutByte(c);
}

// build printf functionality into console output function
void debug(const char* str, ...)
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
        ConsoleSend(PrintStr, NumChars);
    }
}

//override standard printf output
void _mon_putc(char c)
{
    ConsolePut(c);
}
