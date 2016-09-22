/********************************************************************
 FileName:      diagnostic.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Collect and output diagnostic and debug data.

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
  1.0   Initial release                            Corey Shuman 9/11/15


********************************************************************/
#include <plib.h>
#include <stdio.h>
#include "GenericTypeDefs.h"
#include "console.h"
#include "lib/time/Tick.h"
#include "HardwareProfile.h"
#include "OLED_driver.h"
#include "receiver.h"
#include "diagnostic.h"
#include "navigation_controller.h"
#include "motor_controller.h"
#include "usb_support.h"
#include "console.h"
#include "lighting_controller.h"

// enums
enum SM_DIAG {
    SM_DIAG_CLEAR = 0,
    SM_DIAG_UPDATE,
    SM_DIAG_WRITE

} sm;

enum SM_BATT {
    SM_READ_VOLT,
    SM_PROC_VOLT,
    SM_READ_CURR,
    SM_PROC_CURR
} sm_batt;

enum DIAGNOSTIC_STATE {
    DS_INIT = 0,
    DS_NEO,
    DS_M1F,
    DS_M1B,
    DS_M2F,
    DS_M2B,
    DS_M3F,
    DS_M3B,
    DS_M4F,
    DS_M4B,
    DS_ENDWAIT,
    DS_END
};


// static variables
DWORD debugMap;
enum DIAG_MOD debugModule;
static double pry[3];
BOOL dArmed;
BOOL dAccelEnabled;
BOOL dAutoVoiceEnabled;
BYTE ledColorsDebug[] = {4,90,10,5,40,5,0,0,30,10,80,5,64,30,64};
WORD batteryVoltageRaw[10]; // arrays used for averaging samples
double batteryVoltage;
WORD batteryCurrentRaw[10];
double batteryCurrent;
BYTE batteryWriteIdx;

// function declerations
void OledReceiverBar(BYTE idx, WORD WORD);
void DiagnosticUpdateTestScreen(BOOL showTest, const char* str);
void BatteryADCInit(void);
void BatteryADCProcess(void);
void ConvertBatteryReadings(void);

// functions
void DiagInit(void)
{
    int i;
    debugMap = 0ul;
    pry[0] = pry[1] = pry[2] = 0;
    dArmed = FALSE;
    dAccelEnabled = FALSE;
    sm = SM_DIAG_CLEAR;

    BatteryADCInit();
    sm_batt = SM_READ_VOLT;
    for(i=0; i<10; i++) {
        batteryVoltageRaw[i] = 0;
        batteryCurrentRaw[i] = 0;
    }
    batteryWriteIdx = 0;
    batteryVoltage = 0.0;
    batteryCurrent = 0.0;

    // enable diag filters here for now
    enableDiagFilter(DBG_NAV);
    enableDiagFilter(DBG_SERIAL);
}

void DiagProcess(void)
{
    static QWORD tick = TICK_SECOND;
    static BOOL diagEnabled = FALSE;

    SetModule(MOD_DIAG);

    BatteryADCProcess();

    if(!diagEnabled && TickGet() - tick >= TICK_SECOND*5)
    {
        OLED_clear();
        OLED_write(OLED_ADDR);
        tick = TickGet();
        diagEnabled = TRUE;
    }
    else if(diagEnabled && TickGet() - tick >= TICK_SECOND/20)
    {
        // update oled
        char str[40];
        int i;

        switch(sm)
        {
            case SM_DIAG_CLEAR: OLED_clear(); sm++; break;
            case SM_DIAG_UPDATE:
                // update battery readings
                ConvertBatteryReadings();
                sprintf(str, "V:%2.3f", batteryVoltage);
                OLED_text(5,0,str,1);
                sprintf(str, "I:%2.3f", batteryCurrent);
                OLED_text(5,8,str,1);
                // update accelerometer readings
                sprintf(str, "P%6.1f", (double)pry[0]);
                OLED_text(5,16,str,1);
                sprintf(str, "R%6.1f", (double)pry[1]);
                OLED_text(5,24,str,1);
                sprintf(str, "Y%6.1f", (double)pry[2]);
                OLED_text(5,32,str,1);
                if(dArmed)
                    OLED_text(5,40, "ARMED ",1);
                else
                    OLED_text(5,40, "DISARM",1);
                if(dAccelEnabled) {
                    OLED_text(5,48, "ACCEL ",1);
                } else {
                    OLED_text(5,48, "STATIC",1);
                }
                if(dAutoVoiceEnabled) {
                    OLED_text(5,56, "TALKING",1);
                } else {
                    OLED_text(5,56, "SILENT ",1);
                }
                // update receiver readings
                for(i=1; i<=8; i++)
                {
                    OledReceiverBar(i,NavigationGetTelemetry(i));
                }
                sm ++;
                break;
            case SM_DIAG_WRITE: OLED_write(OLED_ADDR); sm = 0; break;

        }
        tick = TickGet();
    }
}

void PrintAccelToOled(double vpry[], BOOL armed, BOOL accelEnabled, BOOL autoVoiceEnabled)
{
    pry[0] = vpry[0];
    pry[1] = vpry[1];
    pry[2] = vpry[2];
    dArmed = armed;
    dAccelEnabled = accelEnabled;
    dAutoVoiceEnabled = autoVoiceEnabled;
}

/* Take value of 0 - 255 and map to 9 bar graph*/
void OledReceiverBar(BYTE idx, WORD val)
{
    val = val / 0x101; // map 0xFFFF to 255
    char bar[10] = ".........";

    if(val < 23)
    {
        bar[0] = '<';
    }
    else if(val > 230)
    {
        bar[8] = '>';
    }
    else
    {
        val /= 23;
        val -= 1;
        bar[val] = '|';
    }

    OLED_text(70,idx*8+8,bar,1);
}

/* ADC Setup for Battery Voltage and Current monitoring.
 * Voltage is on AN8 using MUXA, Current is on AN9 using MUXB.
 */
void BatteryADCInit(void)
{
    CloseADC10();
    ConfigIntADC10(ADC_INT_OFF); // disable ADC interrupt

    OpenADC10(
        /* Config 1 - 16-bit integer format, internal counter auto conversion */
        ADC_FORMAT_INTG | ADC_CLK_AUTO,
        /* Config 2 - Use internal vref, alternate MUXA/MUXB */
        ADC_VREF_AVDD_AVSS | ADC_ALT_INPUT_ON,
        /* Config 3 - 31Td sample time, internal conversion clock */
        ADC_SAMPLE_TIME_31 | ADC_CONV_CLK_INTERNAL_RC,
        /* Config Port - Enable analog on AN8 and AN9 */
        ENABLE_AN8_ANA | ENABLE_AN9_ANA,
        /*Config Scan - Not using scan mode */
        0
    );

    // set channels: AN8 on MUXA, AN9 on MUXB. Use NVREF as negative input for both
    SetChanADC10( ADC_CH0_NEG_SAMPLEB_NVREF | ADC_CH0_POS_SAMPLEB_AN9 | ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN8 );

    EnableADC10();
}

/* Process for reading and averaging voltage and current measurements.
 */
void BatteryADCProcess(void)
{
    switch(sm_batt) {
        case SM_READ_VOLT:
            AcquireADC10(); // start acquisition
            sm_batt++;
            break;
        case SM_PROC_VOLT:
            if(BusyADC10()) { // check if DONE flag set
                batteryVoltageRaw[batteryWriteIdx] = ReadADC10(0);
                sm_batt++;
            }
            break;
        case SM_READ_CURR:
            AcquireADC10(); // ADC will automatically switch to MUXB
            sm_batt++;
            break;
        case SM_PROC_CURR:
            if(BusyADC10()) { // check if DONE flag set
                batteryCurrentRaw[batteryWriteIdx] = ReadADC10(0);
                if(++ batteryWriteIdx >= 10) {
                    batteryWriteIdx = 0;
                }
                sm_batt = SM_READ_VOLT;
            }
            break;
    }
}

/* Average Battery readings and convert to human-readable values
 * ADC reading is 0-1023 = 0-3.3v
 * Voltage is 63.69mV/Volt
 * Current is 36.60mV/Amp
 */
void ConvertBatteryReadings(void)
{
    WORD volt = 0;
    WORD curr = 0;
    int i;
    for(i=0; i<10; i++) {
        volt += batteryVoltageRaw[i];
        curr += batteryCurrentRaw[i];
    }
    volt /= 10;
    curr /= 10;

    batteryVoltage = ((double)volt) * 0.0506486; // 3.3/1023.0/.06369;
    batteryCurrent = ((double)curr) * 0.0888652; // 3.3/1023.0/.03630;
}

/* Test mode to walk through hardware tests
 * Currently tests motors
 * Test is done when returns true
 */
BOOL DiagnosticTestMode(void)
{
    int i;
    int ret = FALSE;
    static enum DIAGNOSTIC_STATE dsState = DS_INIT;
    static BOOL dsWait = FALSE;
    static BOOL btnPressed = TRUE;
    static DWORD dsTick = 0;
    static BYTE dsCount = 0;

    // delay for debouncing
    if(TickGet() - dsTick < TICK_SECOND/5)
        return FALSE;

    dsTick = TickGet();

    ConsoleProcess();
    ProcessUSB();

    if(!btnPressed && mSwitch2)
    {
        btnPressed = TRUE;
        dsWait = FALSE;
    }
    else if(btnPressed && !mSwitch2)
    {
        btnPressed = FALSE;
    }

    if(dsWait)
        return FALSE;

    switch(dsState)
    {
        case DS_INIT:
            DiagnosticUpdateTestScreen(TRUE, "");
            dsWait = FALSE;
            dsState++;
            break;
        case DS_NEO:
            dsCount++;
            if(dsCount == 5)
            {
                for(i=0; i<5; i++)
                {
                    BYTE idx = (dsCount-1)/5 + i;
                    SetLedColor(i+1, ledColorsDebug[3*(idx%5)+1],ledColorsDebug[3*(idx%5)],ledColorsDebug[3*(idx%5)+2]);
                }
                SetLedColor(1,64,64,64);
                UpdateLighting();
                DiagnosticUpdateTestScreen(TRUE, "Neopixel 1");
            }
            else if(dsCount == 10)
            {
                for(i=0; i<5; i++)
                {
                    BYTE idx = (dsCount-1)/5 + i + 1;
                    SetLedColor(i+1, ledColorsDebug[3*(idx%5)+1],ledColorsDebug[3*(idx%5)],ledColorsDebug[3*(idx%5)+2]);
                }
                UpdateLighting();
                DiagnosticUpdateTestScreen(TRUE, "Neopixel 2");
            }
            else if(dsCount == 15)
            {
                for(i=0; i<5; i++)
                {
                    BYTE idx = (dsCount-1)/5 + i + 2;
                    SetLedColor(i+1, ledColorsDebug[3*(idx%5)+1],ledColorsDebug[3*(idx%5)],ledColorsDebug[3*(idx%5)+2]);
                }
                UpdateLighting();
                DiagnosticUpdateTestScreen(TRUE, "Neopixel 3");
                
            }
            else if(dsCount == 20)
            {
                for(i=0; i<5; i++)
                {
                    BYTE idx = (dsCount-1)/5 + i + 3;
                    SetLedColor(i+1, ledColorsDebug[3*(idx%5)+1],ledColorsDebug[3*(idx%5)],ledColorsDebug[3*(idx%5)+2]);
                }
                UpdateLighting();
                DiagnosticUpdateTestScreen(TRUE, "Neopixel 4");
                
            }
            else if(dsCount == 25)
            {
                for(i=0; i<5; i++)
                {
                    BYTE idx = (dsCount-1)/5 + i + 4;
                    SetLedColor(i+1, ledColorsDebug[3*(idx%5)+1],ledColorsDebug[3*(idx%5)],ledColorsDebug[3*(idx%5)+2]);
                }
                UpdateLighting();
                DiagnosticUpdateTestScreen(TRUE, "Neopixel 5");
                dsCount = 0;
            }
            if(btnPressed && dsCount == 10)
                dsState++;
            break;
        case DS_M1F:
            DiagnosticUpdateTestScreen(TRUE, "Motor 1 Forward");
            MotorUpdate(1, 1000);
            dsWait = TRUE;
            dsState++;
            break;
         case DS_M1B:
            DiagnosticUpdateTestScreen(TRUE, "Motor 1 Backward");
            MotorUpdate(1, -1000);
            dsWait = TRUE;
            dsState++;
            break;
        case DS_M2F:
            DiagnosticUpdateTestScreen(TRUE, "Motor 2 Forward");
            MotorUpdate(1, 0);
            MotorUpdate(2, 1000);
            dsWait = TRUE;
            dsState++;
            break;
        case DS_M2B:
            DiagnosticUpdateTestScreen(TRUE, "Motor 2 Backward");
            MotorUpdate(2, -1000);
            dsWait = TRUE;
            dsState++;
            break;
        case DS_M3F:
            DiagnosticUpdateTestScreen(TRUE, "Motor 3 Forward");
            MotorUpdate(2, 0);
            MotorUpdate(3, 1000);
            dsWait = TRUE;
            dsState++;
            break;
        case DS_M3B:
            DiagnosticUpdateTestScreen(TRUE, "Motor 3 Backward");
            MotorUpdate(3, -1000);
            dsWait = TRUE;
            dsState++;
            break;
        case DS_M4F:
            DiagnosticUpdateTestScreen(TRUE, "Motor 4 Forward");
            MotorUpdate(3, 0);
            MotorUpdate(4, 1000);
            dsWait = TRUE;
            dsState++;
            break;
        case DS_M4B:
            DiagnosticUpdateTestScreen(TRUE, "Motor 4 Backward");
            MotorUpdate(4, -1000);
            dsWait = TRUE;
            dsState++;
            break;
        case DS_ENDWAIT:
            DiagnosticUpdateTestScreen(FALSE, "Complete!");
            MotorsStop();
            dsWait = TRUE;
            dsState++;
            break;
        case DS_END:
            ret = TRUE;
            break;
    }

    // allow processes that we need for test to run
    MotorProcess();
    return ret;
}

void DiagnosticUpdateTestScreen(BOOL showTest, const char* str)
{
    OLED_clear();
    OLED_text(5,0,"Diagnostic",2);
    if(showTest)
        OLED_text(5,24,"Test: ",1);
    OLED_text(5,32,str,1);
    OLED_write(OLED_ADDR);
}