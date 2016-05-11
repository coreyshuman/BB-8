/********************************************************************
 FileName:      volume_control.c
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
  1.0   Initial release                            Corey Shuman 5/10/16
 */

#include "p24HJ256GP206.h"
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
#include "XGS_PIC_SYSTEM_V010.h"
#include "GenericTypeDefs.h"
#include "volume_control.h"


#define MAX9744_I2CADDR     0x4B
#define MUTE_PIN_IO         PORTDbits.RD4
#define MUTE_PIN_TRIS       TRISDbits.TRISD4

#define SYSCLK			(g_FCY)
#define PBCLK  			(SYSCLK/2)
#define Fsck			400000
#define BRG_VAL			((PBCLK/2/Fsck)-2)

// Init I2C connection to amplifier board
void Volume_Init(void)
{
    // run at 400 kHz
    OpenI2C1(I2C1_ON | I2C1_7BIT_ADD, BRG_VAL);
    //prevent pop at power up
    Volume_Mute(TRUE);
}

// delay for a bit, then unmute and set to max volume
void Volume_Proc(void)
{
    static WORD sm = 0;

    switch(sm)
    {
        case 600:
            // setup to play
            Volume_Mute(FALSE);
            //Volume_Write(63);
            sm++;
            break;

        case 601:
            // break forever
            break;

        default:
            sm ++;
            break;
    }
}

BOOL Volume_Write(BYTE vol)
{
    BOOL ack = FALSE;
    if(vol > 63)
        vol = 63;

    StartI2C1();
    IdleI2C1();
    MasterWriteI2C1(MAX9744_I2CADDR<<1|0); // address and write command
    IdleI2C1();
    MasterWriteI2C1(vol);
    IdleI2C1();
    ack = !I2C1STATbits.ACKSTAT; // 0 when slave ack, 1 if not ack
    StopI2C1();
    IdleI2C1();

    return ack;
}

BOOL Volume_Mute(BOOL mute)
{
    if(mute)
    {
        MUTE_PIN_TRIS = 0;
        MUTE_PIN_IO = 0;
    }
    else
    {
        MUTE_PIN_TRIS = 1;
        MUTE_PIN_IO = 1;
    }
    
    return TRUE;
}