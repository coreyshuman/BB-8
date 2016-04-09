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



WORD servoValues[2];     // -1000 to 1000


void ServoInit(void)
{
    // head servos
    OpenOC4(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE,1875,1875);
    OpenOC5(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE,1875,1875);
    OpenTimer2(T2_ON | T2_PS_1_64, 25000);
    servoValues[0] = 0; // center
    servoValues[1] = 0; // center
}

void ServoProcess(void)
{
    SetModule(MOD_SERVO);

    // scale from -1000,1000 to 1000,2000
    WORD s1Value = servoValues[0]/2 + 1500;
    WORD s2Value = servoValues[1]/2 + 1500;

    // set servo
    SetDCOC4PWM((int)(s1Value * SERVO_SPEED_MULT));
    SetDCOC5PWM((int)(s2Value * SERVO_SPEED_MULT));
    
}

void ServoUpdate(WORD s1, WORD s2)
{
    if(s1 <= 1000 && s1 >= -1000)
        servoValues[0] = s1;
    if(s2 <= 1000 && s2 >= -1000)
        servoValues[1] = s2;
}