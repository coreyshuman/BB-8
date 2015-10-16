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

char command[10];
char param1[10];
char param2[20];
int cmd = 0;
int chan = 0;
int idx = 0;
int sm = 0;


enum COMM_CMD {
    CCMD_NONE,
    CCMD_OPEN,
    CCMD_PLAY,
    CCMD_PAUSE,
    CCMD_STOP,
    CCMD_CLOSE,
    CCMD_DEBUG
};
enum COMM_SM {
    CSM_GETCM,
    CSM_PARAM1,
    CSM_PARAM2,
    CSM_EXECUTE,
    CSM_ERROR
};

void COMM_Task(void)
{
    unsigned char c;

    while(sm == CSM_ERROR || sm == CSM_EXECUTE || UART_getchar(&c))
    {
        if(c == '\r' || c == '\n')
        {
            if(debug)
                UART_Newline();
        }
        else
        {
            if(debug)
                UART_putchar(c); // echo back
        }

        if(sm == CSM_GETCM)
        {
            if(c == 0x08) // backspace
            {
                if(idx > 0)
                    command[--idx] = 0;
            }
            else
            {
                if(idx < sizeof(command))
                    command[idx++] = c;
            }
            
            if(c == '\n' || c == '\r' || c == ' ')
            {
                command[idx-1] = '\0';
                if(strcmp(command,"o") == 0)
                {
                    cmd = CCMD_OPEN;
                    if(c == ' ')
                        sm = CSM_PARAM1;
                    else
                        sm = CSM_ERROR;
                }
                else if(strcmp(command,"s") == 0)
                {
                    cmd = CCMD_STOP;
                    if(c == ' ')
                        sm = CSM_PARAM1;
                    else
                        sm = CSM_EXECUTE;
                }
                else if(strcmp(command,"p") == 0)
                {
                    cmd = CCMD_PLAY;
                    if(c == ' ')
                        sm = CSM_PARAM1;
                    else
                        sm = CSM_EXECUTE;
                }
                else if(strcmp(command,"a") == 0)
                {
                    cmd = CCMD_PAUSE;
                    if(c == ' ')
                        sm = CSM_PARAM1;
                    else
                        sm = CSM_EXECUTE;
                }
                else if(strcmp(command,"c") == 0)
                {
                   cmd = CCMD_CLOSE;
                    if(c == ' ')
                        sm = CSM_PARAM1;
                    else
                        sm = CSM_EXECUTE;
                }
                else if(strcmp(command,"d") == 0)
                {
                   cmd = CCMD_DEBUG;
                   sm = CSM_EXECUTE;
                }
                else
                {
                    cmd = 0;
                    sm = CSM_ERROR;
                }
                idx = 0;
                chan = 0;
            }
        }
        else if(sm == CSM_PARAM1)
        {
            if(c == 0x08) // backspace
            {
                if(idx > 0)
                    param1[--idx] = 0;
            }
            else
            {
                if(idx < sizeof(param1))
                    param1[idx++] = c;
            }
            if(c == '\n' || c == '\r' || c == ' ')
            {
                param1[idx-1] = '\0';
                chan = atoi(param1);
                if(chan == 0)
                {
                    sm = CSM_ERROR;
                }
                else if(c == ' ')
                {
                    if(cmd = CCMD_OPEN)
                        sm = CSM_PARAM2;
                    else
                        sm = CSM_ERROR;
                }
                else
                {
                    sm = CSM_EXECUTE;
                }
                idx = 0;
            }
        }
        else if(sm == CSM_PARAM2)
        {
            if(c == 0x08) // backspace
            {
                if(idx > 0)
                    param2[--idx] = 0;
            }
            else
            {
                if(idx < sizeof(param2))
                    param2[idx++] = c;
            }
            if(c == '\n' || c == '\r')
            {
                param2[idx-1] = '\0';
                if(cmd == CCMD_OPEN)
                {
                    sm = CSM_EXECUTE;
                }
                else
                {
                    sm = CSM_ERROR;
                }
                idx = 0;
            }
        }
        else if(sm == CSM_EXECUTE)
        {
            if(cmd == CCMD_OPEN)
            {
                if(PWM_Open(chan, param2))
                    UART_puts("ok\r");
                else
                    UART_puts("err\r");
            }
            else if(cmd == CCMD_PLAY)
            {
                if(PWM_Play(chan))
                    UART_puts("ok\r");
                else
                    UART_puts("err\r");
            }
            else if(cmd == CCMD_PAUSE)
            {
                if(PWM_Pause(chan))
                    UART_puts("ok\r");
                else
                    UART_puts("err\r");
            }
            else if(cmd == CCMD_STOP)
            {
                if(PWM_Stop(chan))
                    UART_puts("ok\r");
                else
                    UART_puts("err\r");
            }
            else if(cmd == CCMD_CLOSE)
            {
                if(PWM_Close(chan))
                    UART_puts("ok\r");
                else
                    UART_puts("err\r");
            }
            else if(cmd == CCMD_DEBUG)
            {
                if(debug == FALSE)
                {
                    debug = TRUE;
                    UART_puts("debug on\r\n");
                }
                else
                {
                    debug = FALSE;
                    UART_puts("debug off\r\n");
                }
            }
            else
            {
                sm = CSM_ERROR;
            }
            idx = 0;
            if(sm != CSM_ERROR)
                sm = CSM_GETCM;
        }
        else if(sm == CSM_ERROR)
        {
            idx = 0;
            sm = CSM_GETCM;
            cmd = 0;
            chan = 0;
            UART_puts("err\r");
        }
    }
}