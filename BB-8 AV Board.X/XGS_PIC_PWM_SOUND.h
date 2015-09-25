/********************************************************************
 FileName:      XGS_PIC_PWM_AUDIO.h
 Dependencies:  See INCLUDES section
 Processor:		PIC24 Microcontrollers
 Hardware:		Designed for use on XGS game
                           development boards.
 Complier:  	C30

 This is a PWM-based audio driver that reads WAV files from the SD
 card and plays the audio out of the sound port.
 Files are expected to be 8-bit, mono, 8 khz

 I/O pin map for sound port

    SOUND_OUT          | RD7 | Pin 55 | Output | Output Compare 8

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

// watch for multiple inclusions
#ifndef XGS_PIC_PWM_SOUND
#define XGS_PIC_PWM_SOUND



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//I/O pin map for sound port
//
// SOUND_OUT          | RD7 | Pin 55 | Output | Output Compare 8
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// sound I/O declarations
#define SND_PORT		PORTDbits
#define SND_TRIS		TRISDbits

#define SND_OUT       		SND_PORT.RD7
#define SND_OUT_TRIS     	SND_TRIS.TRISD7

#define E_MOD_PWM               "PWMs"


void PWM_Init();
void PWM_Task();
int PWM_Open_File(char filename[]);
int PWM_Play();
void PWM_Stop();


// end multiple inclusions
#endif
