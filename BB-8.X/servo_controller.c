/********************************************************************
 FileName:      servo_controller.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Servo controller for BB8 head

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
  1.0   Initial release                            Corey Shuman 8/19/15


********************************************************************/
#include <plib.h>
#include <math.h>
#include "HardwareProfile.h"
#include "servo_controller.h"
#include "receiver.h"
#include "mpu_support.h"
#include "console.h"
#include "diagnostic.h"

#define SERVO_TIMER_PERIOD 25000    // 50 hz (80000000 / 64 / 50)
#define SERVO_SPEED_MULT    1.25    // (25000*1.5ms/20ms)/1500us (1.5 ms)
#define PITCH_RATIO         5.56    // (2000us - 1000us) / 180 degrees

int servoAverageValues[2][10];
BYTE loadingAverageCnt;
double pitch;


void ServoInit(void)
{
    int i,j;

    // head servos
    OpenOC4(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE,1875,1875);
    OpenOC5(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE,1875,1875);
    OpenTimer2(T2_ON | T2_PS_1_64, 25000);

    pitch = 0;
    loadingAverageCnt = 0;
    for(i=0; i<2; i++)
    {
        for(j=0; j<10; j++)
        {
            servoAverageValues[i][j] = 1500;
        }
    }
}

void ServoProcess(void)
{
    int i;
    int servoAverage[2];
    int temp;

    SetModule(MOD_SERVO);

    // shift averages
    for(i=10; i>1; i--)
    {
        servoAverageValues[0][i-1] = servoAverageValues[0][i-2];
        servoAverageValues[1][i-1] = servoAverageValues[1][i-2];
    }

    // get values
    temp = ReceiverGetPulse(5);
    if(temp > 900 && temp < 2100)
    {
        servoAverageValues[0][0] = temp;
    }
    temp = ReceiverGetPulse(6);
    if(temp > 900 && temp < 2100)
    {
        servoAverageValues[1][0] = temp;
    }

    if(loadingAverageCnt < 10)
    {
        loadingAverageCnt ++;
    }
    else
    {
        // calc average
        servoAverage[0] = 0;
        servoAverage[1] = 0;
        for(i=0; i<10; i++)
        {
            servoAverage[0] += servoAverageValues[0][i];
            servoAverage[1] += servoAverageValues[1][i];
        }
        servoAverage[0] /= 10;
        servoAverage[1] /= 10;

        // add pitch offset to gimbal
        servoAverage[0] += pitch * PITCH_RATIO;
        if(servoAverage[0] > 2000)
        {
            servoAverage[0] = 2000;
        }
        else if(servoAverage[0] < 1000)
        {
            servoAverage[0] = 1000;
        }

        // set servo
        SetDCOC4PWM((int)(servoAverage[0] * SERVO_SPEED_MULT));
        SetDCOC5PWM((int)(servoAverage[1] * SERVO_SPEED_MULT));
    }
}

void ServoUpdatePitch(double aPitch)
{
    pitch = aPitch;
}