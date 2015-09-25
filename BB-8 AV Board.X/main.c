/********************************************************************
 FileName:      main.c
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
  1.0   Initial release                            Corey Shuman 9/18/15


********************************************************************/
#include "p24HJ256GP206.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GenericTypeDefs.h"
#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_UART_DRV_V010.h"
#include "XGS_PIC_PWM_SOUND.h"
#include "exception.h"

void COMM_Task(void);

int main (void)
{
    int z;
    // Always call ConfigureClock first
    SYS_ConfigureClock(FCY_VGA);

    UART_Init(38400);

    PWM_Init();

    UART_puts("BB-8 Audio Initialized\r\n");
    
    while(1)
    {
        // read any UART messages
        COMM_Task();

        // process messages
        PWM_Task();

    }

    return 0;
}

char command[80];
int idx = 0;
int sm = 0;
void COMM_Task(void)
{
    unsigned char c;

    while(UART_getchar(&c))
    {
        if(c == '\r' || c == '\n')
        {
            UART_Newline();
        }
        else
        {
            UART_putchar(c); // echo back
        }
        if(sm == 0)
        {
            command[idx++] = c;
            if(c == ' ')
            {
                command[idx++] = c = '\n';
            }
            if(c == '\n' || c == '\r')
            {
                command[idx-1] = '\0';
                if(strcmp(command,"open ") == 0)
                {
                    sm = 1; // get data
                }
                else if(strcmp(command,"stop") == 0)
                {
                    PWM_Stop();
                    UART_puts("->Stop\r\n");
                }
                else if(strcmp(command,"play") == 0)
                {
                    PWM_Play();
                    UART_puts("->Play\r\n");
                }
                else if(strcmp(command,"test") == 0)
                {
                    char test[40];
                    sprintf(test, "a:%d b:%lu c:%lu", 22, 4500123L, (DWORD)44500123L);
                    UART_puts(test);
                    UART_printf("a:%d b:%lu c:%lu", 22, 4500123L, (DWORD)44500123L);
                }
                else
                {
                    UART_puts("->Unrecognized\r\n");
                }
                idx = 0;
            }
        }
        else if(sm == 1)
        {
            command[idx++] = c;
            if(c == '\n' || c == '\r')
            {
                command[idx-1] = '\0';
                if(PWM_Open_File(command))
                {
                    UART_puts("->Open\r\n");
                }
                else
                {
                    //ThrowError("INPUT","File Open Failed");
                    UART_puts("File Not Found\r\n");
                }
                idx = 0;
                sm = 0;
            }
        }
    }
}