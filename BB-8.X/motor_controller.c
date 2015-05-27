/********************************************************************
 FileName:      motor_controller.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 PWM motor controller for up to 4 motors.

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
  1.0   Initial release                            Corey Shuman 5/26/15


********************************************************************/

#include <plib.h>
#include "HardwareProfile.h"
#include "motor_controller.h"
#include "receiver.h"

#define MOTOR_TIMER_PERIOD  3200    // 25KHz (80000000 / 25000)
#define MOTOR_SPEED_MULT    1.6    // 3200/2000


void Motor1State(MOTOR_STATE dir, WORD speed);

// setup timers and initialize motors to off
void MotorInit(void)
{
    M1_FORWARD_TRIS = OUTPUT_PIN;
    M1_FORWARD_IO = 0;
    M1_BACKWARD_TRIS = OUTPUT_PIN;
    M1_BACKWARD_IO = 0;

    OpenOC1(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    OpenTimer3(T3_ON | T3_PS_1_1, MOTOR_TIMER_PERIOD);

}

void MotorProcess(void)
{
    WORD pulse = ReceiverGetPulse(1);

    if(pulse > 1550 )
    {
        Motor1State(MOTOR_FORWARD, (pulse - 550)*1.5);
    }
    else if(pulse < 1450)
    {
        Motor1State(MOTOR_BACKWARD, (2450 - pulse)*1.5);
    }
    else
    {
        Motor1State(MOTOR_STOP, 1000);
    }
}

// Update motor 1 state.
//   MOTOR_STATE dir: MOTOR_STOP, MOTOR_FORWARD, MOTOR_BACKWARD
//   uint speed: 1000 - 2000
void Motor1State(MOTOR_STATE dir, WORD speed)
{
    if(dir == MOTOR_FORWARD)
    {
        M1_FORWARD_IO = 1;
        M1_BACKWARD_IO = 0;
    }
    else if(dir == MOTOR_BACKWARD)
    {
        M1_FORWARD_IO = 0;
        M1_BACKWARD_IO = 1;
    }
    else
    {
        M1_FORWARD_IO = 0;
        M1_BACKWARD_IO = 0;
    }

    if(speed >= 1000 && speed <= 2000)
    {
        SetDCOC1PWM((int)(speed * MOTOR_SPEED_MULT));
    }
}


/*
 Motor control routines

function w = calc_speed(a, s)
if(a > 360)
    a = a - 360;
end
if a <= 60
    w = s * (60 - a)/60;
elseif a <= 120
    w = - s * (a - 60)/60;
elseif a <= 180
    w = - s;
elseif a <= 240
    w = - s * (240 - a)/60;
elseif a <= 300
    w = s * (a - 240)/60;
else
    w = s;
end
end
 
[calc_speed(angle, topSpeed) calc_speed(angle+120, topSpeed) calc_speed(angle+240, topSpeed)]





 */