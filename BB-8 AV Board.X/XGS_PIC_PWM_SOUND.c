/********************************************************************
 FileName:      XGS_PIC_PWM_AUDIO.c
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// First and foremost, include the chip definition header
#include "p24HJ256GP206.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Include SD card
#include "FSIO.h"

// include local XGS API header files
#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_PWM_SOUND.h"
#include "XGS_PIC_UART_DRV_V010.h"
#include "waveformat.h"
#include "exception.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int xtod(char c) {
if (c>='0' && c<='9') return c-'0';
if (c>='A' && c<='F') return c-'A'+10;
if (c>='a' && c<='f') return c-'a'+10;
return 0;			// Not Hex
}

int HextoDec(char hex[])
{
	return xtod(hex[0])*16 + xtod(hex[1]);
}

int VerifyWaveFormat(WaveFormat info);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PWM_FREQ 48000ul
#define PWM_TIMER_PERIOD    918 // g_FCY / PWM_FREQ (44073148L/48000)
#define PWM_PERIOD_HALF     459
#define PWM_FACTOR_8BIT     3.6   //(float)PWM_TIMER_PERIOD / (float)255ul   // assume unsigned
#define PWM_FACTOR_16BIT    0.014 //(float)PWM_TIMER_PERIOD / (float)0xFFFFul   // assume signed
#define PWM_FACTOR_32BIT    .000000214      //(float)PWM_TIMER_PERIOD / (float)0xFFFFFFFFul   // assume signed
// OLD SETTINGS:
// The PWM system is running at 255682 hz
// Clock period = 168 (42954540/256000)
// Target speed is 256 kHz, rounded to closes period


WORD PHASE_ACC = 0;
WORD PHASE_INC = 0;
WORD PHASE_COUNT = 0;
DWORD snd_buffer[2048] __attribute__((far));
long int BYTES_REMAINING = 0;
WaveFormat WaveInfo;
DataChunk DataHeader;
int swi = 0;
FSFILE * file = NULL;
int NumRead = 0;
int Buffed;
BOOL stream = FALSE;
volatile BOOL approachingEOF = FALSE;
volatile int samplesRemaining = 0;
BOOL fsInitialized = FALSE;
volatile DWORD tick, tick0, tick1, tick2, tick3;

int PWM_Open_File(char filename[])
{
    if(stream)
        return FALSE;

    if(!fsInitialized)
    {
        fsInitialized = FSInit();
    }

    if(fsInitialized)
    {
        file = FSfopen(filename, "r");

        if(file != NULL)
        {
            // read header - discard for now
            NumRead = FSfread((char*)&WaveInfo,1,sizeof(WaveInfo),file);
            if(NumRead != sizeof(WaveInfo))
            {
                FSfclose(file);
                file = NULL;
                ThrowError(E_MOD_PWM, "FSfread failed");
                return FALSE;
            }
            if(!VerifyWaveFormat(WaveInfo))
            {
                FSfclose(file);
                file = NULL;
                return FALSE;
            }

            // clear remaining fmt chunk bytes
            if(WaveInfo.Subchunk1Size > 16)
            {
                FSfseek(file,WaveInfo.Subchunk1Size-16,SEEK_CUR );
            }
            // read data chunk
            NumRead = FSfread((char*)&DataHeader,1,sizeof(DataHeader),file);
            BYTES_REMAINING = DataHeader.Subchunk2Size;

            NumRead = FSfread(snd_buffer,1,sizeof(snd_buffer),file);
            if(NumRead < sizeof(snd_buffer))
            {
                approachingEOF = TRUE;
                samplesRemaining = NumRead;
            }
            // cts debug
            int i;
            long *a;
            float *f;

            BYTE *b = &WaveInfo;
            UART_printf("sizeof(WaveInfo)=%u\r\n", sizeof(WaveInfo));
            for(i=1; i<=sizeof(WaveInfo); i++)
            {
                UART_printf("%02x ", *b++);
                if(i%16 == 0)
                {
                    UART_Newline();
                }
            }
            UART_Newline();
            b = &DataHeader;
            UART_printf("sizeof(DataHeader)=%u\r\n", sizeof(DataHeader));
            for(i=1; i<=sizeof(DataHeader); i++)
            {
                UART_printf("%02x ", *b++);
                if(i%16 == 0)
                {
                    UART_Newline();
                }
            }
            UART_Newline();
            b = &snd_buffer;
            for(i=1; i<=16*24; i++)
            {
                UART_printf("%02x ", *b++);
                if(i%16 == 0)
                {
                    UART_Newline();
                }
            }
            /*
            for(i=0; i<64; i++)
            {
                a = &snd_buffer;
                f = &snd_buffer;
                UART_printf("%u: %f (%llx)\r\n", i+1, *(f+i), *(a+i));
            }
             * */
            UART_printf("\r\nsizeof(DWORD)=%u sizeof(float)=%u\r\n", sizeof(DWORD) ,sizeof(float));
            PHASE_INC = (WORD)(PWM_FREQ / WaveInfo.SampleRate);
            UART_printf("FREQ: %lu SAMP: %lu PHASE_INC: %u\r\n", PWM_FREQ, WaveInfo.SampleRate, PHASE_INC);
            return TRUE;
        }
        else
        {
            ThrowError(E_MOD_PWM, "FSfOpen failed");
        }
    }
    else
    {
        ThrowError(E_MOD_PWM, "FSInit failed");
    }
    return FALSE;
}

int PWM_Play()
{
    if(file == NULL)
        return FALSE;

    stream = TRUE;
    T2CONbits.TON = 1; //timer go!
    return TRUE;
}

void PWM_Stop()
{
    T2CONbits.TON = 0; //timer stop
    OC8RS = PWM_TIMER_PERIOD/2;
    stream = FALSE;
    PHASE_ACC = 0;
    PHASE_INC = 0;
    PHASE_COUNT = 0;
    swi = 0;
    approachingEOF = FALSE;

    if(file != NULL)
    {
        FSfclose(file);
        file = NULL;
    }

    memset(snd_buffer,0,sizeof(snd_buffer));
}

void PWM_Task()
{
    if (stream == FALSE)
        return;

    if(approachingEOF)
    {
        if(samplesRemaining == 0)
        {
            PWM_Stop();
        }
        return;
    }

    if(PHASE_ACC >= sizeof(snd_buffer)/2 && swi == 0)
    {
        char str[80];
        sprintf(str, "%lu %lu %lu %lu %lu\r\n", tick0, tick1, tick2, tick3, tick);
        UART_puts(str);
        PORTDbits.RD5 = 1;
        swi = 1;
        NumRead = FSfread(snd_buffer,1,sizeof(snd_buffer)/2,file);
        if(NumRead < sizeof(snd_buffer)/2)
        {
            approachingEOF = TRUE;
            samplesRemaining = NumRead + (sizeof(snd_buffer) - PHASE_ACC) - 1;
        }
    }
    else if(PHASE_ACC < sizeof(snd_buffer)/2 && swi == 1)
    {
        PORTDbits.RD5 = 0;
        swi = 0;
        NumRead = FSfread(snd_buffer + (sizeof(snd_buffer)/sizeof(snd_buffer[0]))/2,1,sizeof(snd_buffer)/2,file);
        if(NumRead < sizeof(snd_buffer)/2)
        {
            approachingEOF = TRUE;
            samplesRemaining = NumRead + sizeof(snd_buffer)/2 - PHASE_ACC - 1;
        }
    }
}

void PWM_Init(void)
{
    fsInitialized = FSInit();
    if(!fsInitialized)
    {
        ThrowError(E_MOD_PWM, "FSInit Failed.");
    }

    TRISDbits.TRISD5 = 0;
    PORTDbits.RD5 = 1;
    // Make sure that timer 2 is disabled
    T2CONbits.TON = 0; 				// Disable Timer

    // Audio output is on OC8/RD7 pin
    // Initialize Output Compare Module
    OC8CONbits.OCM = 0b000; 		// Disable Output Compare Module
    OC8R = PWM_TIMER_PERIOD/2; 				// Write the duty cycle for the first PWM pulse
    OC8RS = PWM_TIMER_PERIOD/2; 				// Write the duty cycle for the second PWM pulse
    OC8CONbits.OCTSEL = 0; 			// Select Timer 2 as output compare time base

    // Initialize and enable Timer2
    T2CONbits.TCS = 0; 			// Select internal instruction cycle clock
    T2CONbits.TGATE = 0; 			// Disable Gated Timer mode

    OC8CONbits.OCM = 0b110; 		//pwm mode no fault
    T2CONbits.TCKPS = 0b00;			//1:1 prescaller
    TMR2 = 0x00; 				// Clear timer register

    PR2 = PWM_TIMER_PERIOD;			// Load the period value
                                            
    memset(snd_buffer,0,sizeof(snd_buffer));

    //T2CONbits.TON = 1;
    IPC1bits.T2IP = 1; 			// Set Timer 2 Interrupt Priority Level
    IFS0bits.T2IF = 0; 			// Clear Timer 2 Interrupt Flag
    IEC0bits.T2IE = 1; 			// Enable Timer 2 interrupt
}

int VerifyWaveFormat(WaveFormat info)
{
    // cts debug
    UART_printf("ID: %c%c%c%c\r\n",info.ChunkID[0],info.ChunkID[1],info.ChunkID[2],info.ChunkID[3]);
    UART_printf("For:%c%c%c%c\r\n",info.Format[0],info.Format[1],info.Format[2],info.Format[3]);
    UART_printf("Fmt: %u\r\n", info.AudioFormat);
    UART_printf("chn: %u\r\n", info.NumberChannels);
    UART_printf("srt: %lu\r\n", info.SampleRate);
    UART_printf("brt: %lu\r\n", info.ByteRate);
    UART_printf("bal: %u\r\n", info.BlockAlign);
    UART_printf("bps: %u\r\n", info.BitsPerSample);

    if(info.ChunkID[0] != 'R' || info.ChunkID[1] != 'I' || info.ChunkID[2] != 'F' || info.ChunkID[3] != 'F')
    {
        ThrowError(E_MOD_PWM, "Invalid File Format");
        return FALSE;
    }

    if(info.Format[0] != 'W' || info.Format[1] != 'A' || info.Format[2] != 'V' || info.Format[3] != 'E')
    {
        ThrowError(E_MOD_PWM, "Invalid File Format");
        return FALSE;
    }

    if(info.AudioFormat != PCM && info.AudioFormat != FLOAT)
    {
        ThrowError(E_MOD_PWM, "Only PCM and FLOAT Supported");
        return FALSE;
    }

    if(info.BitsPerSample != 8 && info.BitsPerSample != 16 && info.BitsPerSample != 32)
    {
        ThrowError(E_MOD_PWM, "Only 8, 16, 32 Bits Per Sample Supported");
        return FALSE;
    }

    if(info.SampleRate > PWM_FREQ)
    {
        ThrowError(E_MOD_PWM, "Maximum Sample Rate Exceeded");
        return FALSE;
    }

    return TRUE;
}

// Timer 2 ISR steps through file during playback
void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt( void )
{
    DWORD sample;

    // clear interrupt flag right away so timing stays on track
    IFS0bits.T2IF = 0;

    tick = TMR2;

    PHASE_COUNT++;
    tick0 = TMR2 - tick;
    if(PHASE_COUNT >= PHASE_INC)
    {
        PHASE_ACC += WaveInfo.BlockAlign;
        BYTES_REMAINING -= WaveInfo.BlockAlign;
        PHASE_COUNT = 0;

        if(approachingEOF)
        {
            if(samplesRemaining > 0)
            {
                samplesRemaining -= 1;
            }
            else
            {
                // stop here - recurring task will handle cleanup
                IFS0bits.T2IF = 0;
                return;
            }
        }
    }

    if(PHASE_ACC >= sizeof(snd_buffer))
    {
        PHASE_ACC = 0;
    }
tick1 = TMR2 - tick;
    if(WaveInfo.AudioFormat == PCM)
    {
        switch(WaveInfo.BitsPerSample)
        {
            case 8:
                tick2 = TMR2 - tick;
                // old code - takes 440 ticks
                //sample = snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] >> (8*(PHASE_ACC % sizeof(snd_buffer[0]))) & 0xFF;
                //sample = (long int)((float)sample * PWM_FACTOR_8BIT);

                // option 2 takes 398
                //sample = *((BYTE *)snd_buffer + PHASE_ACC) * PWM_FACTOR_8BIT;

                // option 3 - less accurate, uses 25 ticks!!!
                sample = *((BYTE *)snd_buffer + PHASE_ACC) << 2;
                if(sample > PWM_TIMER_PERIOD)
                {
                    sample = PWM_TIMER_PERIOD;
                }


                tick3 = TMR2 - tick;
                break;
            case 16:
                tick2 = TMR2 - tick;
                // option 1 - uses 457 ticks
                //sample = ((short)(snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] >> (8*(PHASE_ACC%sizeof(snd_buffer[0]))) ) & 0xFFFF + 32768ul ) * PWM_FACTOR_16BIT;

                // option 2 - uses 33 ticks, clips at top end
                sample = ((LONG)*((SHORT *)snd_buffer + PHASE_ACC/2) + 0x8000ul) >> 6;
                if(sample > PWM_TIMER_PERIOD)
                {
                    sample = PWM_TIMER_PERIOD;
                }

                tick3 = TMR2 - tick;

                break;

            case 32:
                sample = (unsigned long int)(snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] + 0xFFFFul) * PWM_FACTOR_32BIT;
                break;
        }
    }
    else if(WaveInfo.AudioFormat == FLOAT)    // FLOAT
    {
        tick2 = TMR2 - tick;
        // option 1 - 278 ticks
        //sample = (int)((float)snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] * (float)PWM_TIMER_PERIOD);

        //float samplef = *((float *)snd_buffer + PHASE_ACC/4);
        //UART_printf("%f\r\n", samplef);

        // option 2 - 267 ticks
        sample = (DWORD)(*((float *)snd_buffer + PHASE_ACC/4) * (float)PWM_PERIOD_HALF);
        // adjust from signed to unsigned
        sample += PWM_PERIOD_HALF;

        tick3 = TMR2 - tick;
    }


    OC8RS = sample; // Write Duty Cycle value for next PWM cycle

    tick = TMR2 - tick;
}
