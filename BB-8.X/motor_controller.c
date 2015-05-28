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
#include <math.h>
#include "HardwareProfile.h"
#include "motor_controller.h"
#include "receiver.h"
#include "console.h"

#define MOTOR_TIMER_PERIOD  3200    // 25KHz (80000000 / 25000)
#define MOTOR_SPEED_MULT    1.6    // 3200/2000

#define M_180_PI            57.2958 // 180/pi


void MotorState(BYTE motor, MOTOR_STATE dir, WORD speed);
WORD calcSpeed(WORD angle, WORD speed);

// setup timers and initialize motors to off
void MotorInit(void)
{
    M1_FORWARD_TRIS = OUTPUT_PIN;
    M1_FORWARD_IO = 0;
    M1_BACKWARD_TRIS = OUTPUT_PIN;
    M1_BACKWARD_IO = 0;
    M2_FORWARD_TRIS = OUTPUT_PIN;
    M2_FORWARD_IO = 0;
    M2_BACKWARD_TRIS = OUTPUT_PIN;
    M2_BACKWARD_IO = 0;
    M3_FORWARD_TRIS = OUTPUT_PIN;
    M3_FORWARD_IO = 0;
    M3_BACKWARD_TRIS = OUTPUT_PIN;
    M3_BACKWARD_IO = 0;

    OpenOC1(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    OpenOC2(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    OpenOC3(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    OpenTimer3(T3_ON | T3_PS_1_1, MOTOR_TIMER_PERIOD);

}

void MotorProcess(void)
{
    int i;
    int xPulse = ReceiverGetPulse(1) - 1500;
    int yPulse = ReceiverGetPulse(2) - 1500;
    WORD speed;
    WORD angle;
    WORD speedPulse;

    if(xPulse == 0 && yPulse == 0)
    {
        speed = 0;
    }
    else
    {
        speed = (int)sqrt(pow(xPulse,2) + pow(yPulse,2));
    }

    angle = (WORD)((atan2(xPulse, yPulse) * M_180_PI) + 360) % 360;

    char string[50];
    sprintf(string, "x y s a p: %5d %5d %5u %5u\r\n", xPulse, yPulse, speed, angle);
    debug(string);

    for(i=1;i<=3; i++)
    {
        speedPulse = calcSpeed(angle + 120*(i-1), speed) + 1500;
        if(speedPulse > 1550 )
        {
            MotorState(i, MOTOR_FORWARD, (speedPulse - 550)*1.5);
        }
        else if(speedPulse < 1450)
        {
            MotorState(i, MOTOR_BACKWARD, (2450 - speedPulse)*1.5);
        }
        else
        {
            MotorState(i, MOTOR_STOP, 1000);
        }
    }

    
}

// Update motor 1 state.
//   MOTOR_STATE dir: MOTOR_STOP, MOTOR_FORWARD, MOTOR_BACKWARD
//   uint speed: 1000 - 2000
void MotorState(BYTE motor, MOTOR_STATE dir, WORD speed)
{

    if(dir == MOTOR_FORWARD)
    {
        if(motor == 1)
        {
            M1_FORWARD_IO = 1;
            M1_BACKWARD_IO = 0;
        }
        else if(motor == 2)
        {
            M2_FORWARD_IO = 1;
            M2_BACKWARD_IO = 0;
        }
        else if(motor == 3)
        {
            M3_FORWARD_IO = 1;
            M3_BACKWARD_IO = 0;
        }
    }
    else if(dir == MOTOR_BACKWARD)
    {
        if(motor == 1)
        {
            M1_FORWARD_IO = 0;
            M1_BACKWARD_IO = 1;
        }
        else if(motor == 2)
        {
            M2_FORWARD_IO = 0;
            M2_BACKWARD_IO = 1;
        }
        else if(motor == 3)
        {
            M3_FORWARD_IO = 0;
            M3_BACKWARD_IO = 1;
        }
    }
    else
    {
        if(motor == 1)
        {
            M1_FORWARD_IO = 0;
            M1_BACKWARD_IO = 0;
        }
        else if(motor == 2)
        {
            M2_FORWARD_IO = 0;
            M2_BACKWARD_IO = 0;
        }
        else if(motor == 3)
        {
            M3_FORWARD_IO = 0;
            M3_BACKWARD_IO = 0;
        }
    }

    if(speed >= 1000 && speed <= 2000)
    {
        if(motor == 1)
            SetDCOC1PWM((int)(speed * MOTOR_SPEED_MULT));
        else if(motor == 2)
            SetDCOC2PWM((int)(speed * MOTOR_SPEED_MULT));
        else if(motor == 3)
            SetDCOC3PWM((int)(speed * MOTOR_SPEED_MULT));
    }
}

WORD calcSpeed(WORD angle, WORD speed)
{
    WORD w;
    
    if(angle > 360)
    {
        angle -= 360;
    }
    
    if (angle <= 60)
        w = speed * (60 - angle)/60;
    else if (angle <= 120)
        w = - speed * (angle - 60)/60;
    else if (angle <= 180)
        w = - speed;
    else if (angle <= 240)
        w = - speed * (240 - angle)/60;
    else if (angle <= 300)
        w = speed * (angle - 240)/60;
    else
        w = speed;
    
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