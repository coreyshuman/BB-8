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

    SOUND_OUT          | RD7 | Pin 52 | Output | Output Compare 8

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



// include local XGS API header files
#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_PWM_SOUND.h"
#include "XGS_PIC_UART_DRV_V010.h"

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

#define CHANNEL_COUNT           2

PLAYERINFO player[CHANNEL_COUNT] __attribute__((far));


int NumRead = 0;
BOOL fsInitialized = FALSE;
volatile DWORD tick, tick0, tick1, tick2, tick3;

int PWM_Open(BYTE chan, char filename[])
{
    PLAYERINFO *p;
    if(chan == 0)
        chan = 1;
    if(chan > CHANNEL_COUNT)
        return FALSE;
    p = &player[chan-1];
    if(p->open)
        return FALSE;

    if(!fsInitialized)
    {
        fsInitialized = FSInit();
    }

    if(fsInitialized)
    {
        if(strstr(filename,".wav") == 0 && strlen(filename) < sizeof(p->filename)-5)
        {
            strcat(filename,".wav");
        }

        strncpy(p->filename, filename, sizeof(p->filename));

        p->file = FSfopen(filename, "r");

        if(p->file != NULL)
        {
            p->open = TRUE;
            // read header - discard for now
            NumRead = FSfread((char*)&p->wavInfo,1,sizeof(WaveFormat),p->file);
            if(NumRead != sizeof(WaveFormat))
            {
                FSfclose(p->file);
                p->file = NULL;
                ThrowError(E_MOD_PWM, "FSfread failed");
                return FALSE;
            }
            if(!VerifyWaveFormat(p->wavInfo))
            {
                FSfclose(p->file);
                p->file = NULL;
                return FALSE;
            }

            // clear remaining fmt chunk bytes
            if(p->wavInfo.Subchunk1Size > 16)
            {
                FSfseek(p->file,p->wavInfo.Subchunk1Size-16,SEEK_CUR );
            }
            // read data chunk
            NumRead = FSfread((char*)&p->dataInfo,1,sizeof(DataChunk),p->file);
            p->bytesRemaining = p->dataInfo.Subchunk2Size;

            NumRead = FSfread(p->buffer,1,sizeof(p->buffer),p->file);
            p->bytesRead += NumRead;

            p->phaseInc = (WORD)(PWM_FREQ / p->wavInfo.SampleRate);
            if(PWM_FREQ % p->wavInfo.SampleRate > 0 && debug)
            {
                UART_puts("Warning: SampleRate not supported\r\n");
            }
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

int PWM_Play(BYTE chan)
{
    PLAYERINFO *p;
    if(chan == 0)
        chan = 1;
    if(chan > CHANNEL_COUNT)
        return FALSE;
    p = &player[chan-1];
    if(!p->open)
        return FALSE;

    p->playing = TRUE;
    return TRUE;
}

int PWM_Pause(BYTE chan)
{
    PLAYERINFO *p;
    if(chan == 0)
        chan = 1;
    if(chan > CHANNEL_COUNT)
        return FALSE;
    p = &player[chan-1];
    if(!p->open)
        return FALSE;

    p->playing = FALSE;
    return TRUE;
}

int PWM_Stop(BYTE chan)
{
    PLAYERINFO *p;
    if(chan == 0)
        chan = 1;
    if(chan > CHANNEL_COUNT)
        return FALSE;
    p = &player[chan-1];
    if(!p->open)
        return FALSE;
    
    p->playing = FALSE;
    p->bytesRemaining = p->dataInfo.Subchunk2Size;
    p->phaseCnt = 0;
    // next two lines will cause data to load beginning of buffer
    //  when set to play (set accumulator to end of buffer, will auto wrap)
    p->phaseAcc = sizeof(p->buffer);
    p->bufferFlip = 1;
    // adjust read buffer
    FSfseek(p->file,-(p->bytesRead),SEEK_CUR );
    p->bytesRead = 0;
    return TRUE;
}

int PWM_Close(BYTE chan)
{
    PLAYERINFO *p;
    if(chan == 0)
        chan = 1;
    if(chan > CHANNEL_COUNT)
        return FALSE;
    p = &player[chan-1];
    if(!p->open)
        return FALSE;

    p->playing = FALSE; // stop first so interrupt doesn't access

    if(p->file != NULL)
    {
        FSfclose(p->file);
    }

    memset(p,0,sizeof(PLAYERINFO));
    return TRUE;
}

void PWM_Task()
{
    int chan;
    char str[80];

    PORTDbits.RD5 = 0;

    for(chan = 0; chan<CHANNEL_COUNT; chan++)
    {
        PLAYERINFO *p = &player[chan];
        if(p->sendEnd)
        {
            p->sendEnd = FALSE;
            sprintf(str, "e %u\r", chan+1);
            UART_puts(str);
        }
        if(!p->open || !p->playing)
            continue;

        PORTDbits.RD5 = 1;

        if(p->phaseAcc >= sizeof(p->buffer)/2 && p->bufferFlip == 0)
        {
            //sprintf(str, "%lu %lu %lu %lu %lu\r\n", tick0, tick1, tick2, tick3, tick);
            if(debug)
            {
                sprintf(str, "%lu\r\n", tick);
                UART_puts(str);
            }
            p->bufferFlip = 1;
            NumRead = FSfread(p->buffer,1,sizeof(p->buffer)/2,p->file);
            p->bytesRead += NumRead;
        }
        else if(p->phaseAcc < sizeof(p->buffer)/2 && p->bufferFlip == 1)
        {
            p->bufferFlip = 0;
            NumRead = FSfread(p->buffer + (sizeof(p->buffer)/sizeof(p->buffer[0]))/2,1,sizeof(p->buffer)/2,p->file);
            p->bytesRead += NumRead;
        }
    }
}

void PWM_Init(void)
{
    int i;
    fsInitialized = FSInit();
    if(!fsInitialized)
    {
        ThrowError(E_MOD_PWM, "FSInit Failed.");
    }

    TRISDbits.TRISD5 = 0;
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
                                            
    for(i=0; i<CHANNEL_COUNT; i++)
    {
        PLAYERINFO *p = &player[i];
        memset(p,0,sizeof(PLAYERINFO));
    }

    //T2CONbits.TON = 1;
    IPC1bits.T2IP = 1; 			// Set Timer 2 Interrupt Priority Level
    IFS0bits.T2IF = 0; 			// Clear Timer 2 Interrupt Flag
    IEC0bits.T2IE = 1; 			// Enable Timer 2 interrupt

    T2CONbits.TON = 1; 			// Enable Timer
}

int VerifyWaveFormat(WaveFormat info)
{
    if(debug)
    {
        UART_printf("ID: %c%c%c%c\r\n",info.ChunkID[0],info.ChunkID[1],info.ChunkID[2],info.ChunkID[3]);
        UART_printf("For:%c%c%c%c\r\n",info.Format[0],info.Format[1],info.Format[2],info.Format[3]);
        UART_printf("Fmt: %u\r\n", info.AudioFormat);
        UART_printf("chn: %u\r\n", info.NumberChannels);
        UART_printf("srt: %lu\r\n", info.SampleRate);
        UART_printf("brt: %lu\r\n", info.ByteRate);
        UART_printf("bal: %u\r\n", info.BlockAlign);
        UART_printf("bps: %u\r\n", info.BitsPerSample);
    }

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

    if(info.AudioFormat == PCM && (info.BitsPerSample != 8 && info.BitsPerSample != 16))
    {
        ThrowError(E_MOD_PWM, "Only 8 and 16 Bits PCM Supported");
        return FALSE;
    }

    if(info.AudioFormat == FLOAT && (info.BitsPerSample != 32))
    {
        ThrowError(E_MOD_PWM, "Only 32 Bit Float Supported");
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
    long sample = 0;
    long mixedSample = PWM_PERIOD_HALF;
    int chan;
    int activeChannels = 0;

    // clear interrupt flag right away so timing stays on track
    IFS0bits.T2IF = 0;

    tick = TMR2;

    for(chan=0; chan<CHANNEL_COUNT; chan++)
    {
        PLAYERINFO *p = &player[chan];
        WaveFormat wav = p->wavInfo;
        if(!p->open || !p->playing)
            continue;

        activeChannels ++;

        p->phaseCnt++;
        if(p->phaseCnt >= p->phaseInc)
        {
            p->phaseAcc += wav.BlockAlign;
            p->bytesRemaining -= wav.BlockAlign;
            p->phaseCnt = 0;
        }

        if(p->phaseAcc >= sizeof(p->buffer))
        {
            p->phaseAcc = 0;
        }

        if(p->bytesRemaining == 0)
        {
            p->playing = FALSE;
            p->sendEnd = TRUE;
        }

        if(wav.AudioFormat == PCM)
        {
            switch(wav.BitsPerSample)
            {
                case 8:
                    // old code - takes 440 ticks
                    //sample = snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] >> (8*(PHASE_ACC % sizeof(snd_buffer[0]))) & 0xFF;
                    //sample = (long int)((float)sample * PWM_FACTOR_8BIT);

                    // option 2 takes 398
                    //sample = *((BYTE *)snd_buffer + PHASE_ACC) * PWM_FACTOR_8BIT;

                    // option 3 - less accurate, uses 25 ticks!!!
                    //sample = *((BYTE *)p->buffer + p->phaseAcc) << 2;
                    // scale to -32768 -> 32767
                    sample = (*((BYTE *)p->buffer + p->phaseAcc) -0x80) << 8;
                    // min:   max:

                    break;
                case 16:
                    // option 1 - uses 457 ticks
                    //sample = ((short)(snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] >> (8*(PHASE_ACC%sizeof(snd_buffer[0]))) ) & 0xFFFF + 32768ul ) * PWM_FACTOR_16BIT;

                    // option 2 - uses 33 ticks, clips at top end
                    //sample = ((LONG)*((SHORT *)p->buffer + p->phaseAcc/2) + 0x8000ul) >> 6;
                    // already at scale -32768 -> 32767
                    sample = ((LONG)*((SHORT *)p->buffer + p->phaseAcc/2));
                    break;

//                case 32:
//                    sample = (unsigned long int)(snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] + 0xFFFFul) * PWM_FACTOR_32BIT;
//                    break;
            }
        }
        else if(wav.AudioFormat == FLOAT)    // FLOAT
        {
            // option 1 - 278 ticks
            //sample = (int)((float)snd_buffer[PHASE_ACC/sizeof(snd_buffer[0])] * (float)PWM_TIMER_PERIOD);

            //float samplef = *((float *)snd_buffer + PHASE_ACC/4);
            //UART_printf("%f\r\n", samplef);

            // option 2 - 267 ticks
            // scale to -32768 -> 32767
            sample = (DWORD)(*((float *)p->buffer + p->phaseAcc/4) * (float)0x8000);
            // adjust from signed to unsigned
            //sample += PWM_PERIOD_HALF;
        }

        // average samples together
        if(activeChannels == 1)
            mixedSample = sample;
        else
            mixedSample += sample;
        if(activeChannels > 1)
            mixedSample >> 2;

        /*
        // mixin sample data to final output buffer
        if(mixedSample < PWM_PERIOD_HALF && sample < PWM_PERIOD_HALF)
        {

        }
        if(mixedSample > PWM_PERIOD_HALF && sample > PWM_PERIOD_HALF)
        {

        }
        else
        {
            mixedSample = mixedSample + sample;
        }
        */
    }

    // convert from -32768 -> 32767 scale to 0 -> 918 (aprox)
    mixedSample += 0x8000;
    //mixedSample /= 71;
    mixedSample = mixedSample >> 7;
    if(mixedSample > PWM_TIMER_PERIOD)
    {
        mixedSample = PWM_TIMER_PERIOD;
    }
    OC8RS = mixedSample; // Write Duty Cycle value for next PWM cycle

    tick = TMR2 - tick;
}
