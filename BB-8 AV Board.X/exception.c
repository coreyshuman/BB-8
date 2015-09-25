/********************************************************************
 FileName:      exception.c
 Dependencies:  See INCLUDES section
 Processor:		PIC24 Microcontrollers
 Hardware:		Designed for use on XGS game
                           development boards.
 Complier:  	C30

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
  1.0   Initial release                            Corey Shuman 9/19/15


********************************************************************/
#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_UART_DRV_V010.h"

void ThrowException(char * msg)
{
    UART_puts(msg);

    // cts todo - trigger error LED
    while(1) {};
}

void ThrowError(char * module, char * msg)
{
    UART_puts(module);
    UART_putchar(':');
    UART_puts(msg);
    UART_Newline();
}