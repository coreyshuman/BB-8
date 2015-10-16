/********************************************************************
 FileName:      audio_controller.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Audio controller for XGS custom firmware audio board.

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
  1.0   Initial release                            Corey Shuman 9/30/15


********************************************************************/
#include <plib.h>
#include "HardwareProfile.h"
#include "TCPIPConfig.h"
#include "./USB/usb_function_cdc.h"
#include "TCPIP Stack/Tick.h"
#include "audio_controller.h"
#include "console.h"
#include "diagnostic.h"

#define MAX_FILES 11

char AUD_UART_RX_Buffer[64];
BYTE AUD_UART_RX_StartPtr;
BYTE AUD_UART_RX_EndPtr;

int  resIdx = 0;
char response[20];

char BB8_Sample_Names[MAX_FILES][10] = {
    "bb01", "bb02", "bb03", "bb04", "bb05", "bb06", "bb07", "bb08", "bb09",
    "bb10", "bb11"
};

enum AUDIO_CONTROLLER_STATE {
    ACS_DELAY,
    ACS_DELAY_WAIT,
    ACS_OPEN,
    ACS_OPEN_ACK,
    ACS_PLAY,
    ACS_PLAY_ACK,
    ACS_PLAY_WAIT,
    ACS_CLOSE,
    ACS_CLOSE_WAIT
} ;

enum AUDIO_RESPONSE {
    AR_NONE = 0,
    AR_OK,
    AR_ERROR,
    AR_END1,
    AR_END2
};

enum AUDIO_CONTROLLER_STATE acs = ACS_DELAY;
int delaySeconds = 0;
DWORD tick = 0;

void AudioCommandOpen(char* filename, int chan);
void AudioCommandPlay(int chan);
void AudioCommandClose(int chan);
enum AUDIO_RESPONSE AudioGetResponse();
void AudioSendData(BYTE *buf, unsigned int length);
unsigned char AudioRxGetCount(void);
void AudioRxPutByte(char c);
char AudioRxGetByte(void);
void AudioRxClearBuffer(void);

void AudioInit(void)
{
    //OpenUART3A(UART_EN, (1 << 12)|UART_TX_ENABLE, (SYS_FREQ/(1<<mOSCGetPBDIV())/16)/BAUD_RATE-1);
    UARTConfigure(UART3A,UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode(UART3A, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(UART3A, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART3A, GetPeripheralClock(), AUDIO_BAUD_RATE);
    UARTEnable(UART3A, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
    // Configure UART RX Interrupt
    mU3AEIntEnable(1);
    INTEnable(INT_SOURCE_UART_RX(UART3A), INT_ENABLED);
    INTSetVectorPriority(INT_VECTOR_UART(UART3A), INT_PRIORITY_LEVEL_5);
    INTSetVectorSubPriority(INT_VECTOR_UART(UART3A), INT_SUB_PRIORITY_LEVEL_0);

    // ring buffer init
    AUD_UART_RX_StartPtr = AUD_UART_RX_EndPtr = 0;

    //setbuf(stdout, NULL );

    // cts debug
    enableDiagFilter(DBG_AUDIO);
}

void AudioProcess(void)
{
    enum AUDIO_RESPONSE arx;
    switch(acs)
    {
        case ACS_DELAY:
            delaySeconds = 0;
            // get delay between 3 and 15 seconds
            while(delaySeconds<3)
            {
                delaySeconds = rand()%15;
            }
            tick  = TickGet();
            acs++;
            if(isDiagFilterOn(DBG_AUDIO)) {
                debug("AUD delay %i\r\n", delaySeconds);
            }
            break;
        case ACS_DELAY_WAIT:
            if(TickGet() - tick >= delaySeconds*TICK_SECOND)
            {
                acs++;
            }
            break;
        case ACS_OPEN:
        {
            int fileIdx = rand()%MAX_FILES;
            AudioCommandOpen(BB8_Sample_Names[fileIdx], 1);
            tick = TickGet();
            acs++;
            if(isDiagFilterOn(DBG_AUDIO)) {
                debug("AUD open %s ", BB8_Sample_Names[fileIdx]);
            }
            break;
        }
        case ACS_OPEN_ACK:
            arx = AudioGetResponse();
            if(arx == AR_OK)
            {
                acs++;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("ok\r\n");
                }

            }
            else if(arx == AR_ERROR)
            {
                acs = ACS_CLOSE;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("err\r\n");
                }
            }
            if(TickGet() - tick > 3*TICK_SECOND)
            {
                // error
                acs = ACS_CLOSE;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("tmo\r\n");
                }
            }
            break;
        case ACS_PLAY:
            AudioCommandPlay(1);
            tick = TickGet();
            acs++;
            if(isDiagFilterOn(DBG_AUDIO)) {
                debug("AUD play ");
            }
            break;
        case ACS_PLAY_ACK:
            arx = AudioGetResponse();
            if(arx == AR_OK)
            {
                acs++;
                tick = TickGet();
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("ok\r\n");
                }
            }
            else if(arx == AR_ERROR)
            {
                acs = ACS_CLOSE;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("err\r\n");
                }
            }
            if(TickGet() - tick > 3*TICK_SECOND)
            {
                // error
                acs = ACS_CLOSE;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("tmo\r\n");
                }
            }
            break;
        case ACS_PLAY_WAIT:
            arx = AudioGetResponse();
            if(arx == AR_OK || arx == AR_ERROR)
            {
                // error
                acs = ACS_CLOSE;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("AUD err\r\n");
                }
            }
            else if(arx == AR_END1 || arx == AR_END2)
            {
                acs++;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("AUD ok\r\n");
                }
            }
            if(TickGet() - tick > 60*TICK_SECOND)
            {
                // error
                acs = ACS_CLOSE;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("AUD tmo\r\n");
                }
            }
            break;
        case ACS_CLOSE:
            AudioCommandClose(1);
            acs = 0;
            if(isDiagFilterOn(DBG_AUDIO)) {
                debug("AUD close\r\n");
            }
            break;
        case ACS_CLOSE_WAIT:
            arx = AudioGetResponse();
            if(arx == AR_OK)
            {
                acs=ACS_DELAY;
                tick = TickGet();
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("ok\r\n");
                }
            }
            else if(arx == AR_ERROR)
            {
                acs = ACS_DELAY;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("err\r\n");
                }
            }
            if(TickGet() - tick > 3*TICK_SECOND)
            {
                // error
                acs = ACS_DELAY;
                if(isDiagFilterOn(DBG_AUDIO)) {
                    debug("tmo\r\n");
                }
            }
            break;
    }
}

void AudioCommandOpen(char* filename, int chan)
{
    char buf[20];
    sprintf(buf, "o %i %s\r", chan, filename);
    
    AudioSendData(buf, strlen(buf));
}

void AudioCommandPlay(int chan)
{
    char buf[10];
    sprintf(buf, "p %i\r", chan);
    
    AudioSendData(buf, strlen(buf));
}

void AudioCommandClose(int chan)
{
    char buf[10];
    sprintf(buf, "c %i\r", chan);
    
    AudioSendData(buf, strlen(buf));
}

enum AUDIO_RESPONSE AudioGetResponse()
{
    enum AUDIO_RESPONSE ret = AR_NONE;
    int rx = 0;

    while(AudioRxGetCount() > 0 && resIdx < sizeof(response))
    {
        response[resIdx] = AudioRxGetByte();
        if(isDiagFilterOn(DBG_AUDIO)) {
            if(response[resIdx] >= 0x20)
                debug("(%c)", response[resIdx]);
            else
                debug("[%02X]", response[resIdx]);
        }
        if(response[resIdx] == '\r')
        {
            response[resIdx] = '\0';
            rx = TRUE;
            break;
        }
        
        resIdx ++;
    }

    if(resIdx >= sizeof(response))
    {
        resIdx = 0;
        AudioRxClearBuffer();
        if(isDiagFilterOn(DBG_AUDIO)) {
            debug("AUD OVF");
        }
    }

    if(rx)
    {
        resIdx = 0;
        if(strcmp(response,"ok") == 0)
        {
            ret = AR_OK;
        }
        else if(strcmp(response,"err") == 0)
        {
            ret = AR_ERROR;
        }
        else if(strcmp(response,"e 1") == 0)
        {
            ret = AR_END1;
        }
        else if(strcmp(response,"e 2") == 0)
        {
            ret = AR_END2;
        }
        AudioRxClearBuffer();
    }
    return ret;
}

void AudioTxPutByte(char c)
{
    while (!UARTTransmitterIsReady(UART3A));
       UARTSendDataByte(UART3A, c);
     while (!UARTTransmissionHasCompleted(UART3A));
}

void AudioSendData(BYTE *buf, unsigned int length)
{
	unsigned int i = 0;
	while(i<length)
	{
            AudioTxPutByte(buf[i++]);
	}
}

unsigned char AudioRxGetCount(void)
{
    if(AUD_UART_RX_StartPtr <= AUD_UART_RX_EndPtr)
    {
        return (AUD_UART_RX_EndPtr - AUD_UART_RX_StartPtr);
    }
    else
    {
        return (64 - AUD_UART_RX_EndPtr + AUD_UART_RX_StartPtr);
    }
}

void AudioRxPutByte(char c)
{
    AUD_UART_RX_Buffer[AUD_UART_RX_EndPtr] = c;
    AUD_UART_RX_EndPtr ++;
    if(AUD_UART_RX_EndPtr >= 64)
        AUD_UART_RX_EndPtr = 0;
    LATEbits.LATE0 = ~LATEbits.LATE0;
}

char AudioRxGetByte(void)
{
    char c = AUD_UART_RX_Buffer[AUD_UART_RX_StartPtr];
    AUD_UART_RX_StartPtr ++;
    if(AUD_UART_RX_StartPtr >= 64)
        AUD_UART_RX_StartPtr = 0;
    return c;
}

void AudioRxClearBuffer(void)
{
    AUD_UART_RX_StartPtr = AUD_UART_RX_EndPtr;
}

void __ISR(_UART_3A_VECTOR, ipl5) _UART3AISRHandler(void) {
    // Is this an RX interrupt?
    if (INTGetFlag(INT_U3ARX)) {
        while(UARTReceivedDataIsAvailable(UART3A))
        {
            AudioRxPutByte(UARTGetDataByte(UART3A));
            //ConsolePut(UARTGetDataByte(UART2A));
        }
        // handle overflow flag
        if (mU3AEGetIntFlag()) {
            U3ASTAbits.OERR = 0;
            mU3AEClearIntFlag();
        }
        // Clear the RX interrupt Flag
        INTClearFlag(INT_U3ARX);
        LATEbits.LATE1 = ~LATEbits.LATE1;
    }

    // We don't care about TX interrupt
    if (INTGetFlag(INT_U3ARX)) {
        INTClearFlag(INT_U3ARX);
    }
}