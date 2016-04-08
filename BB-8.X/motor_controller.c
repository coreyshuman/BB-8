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
#include "mpu_support.h"
#include "console.h"
#include "servo_controller.h"
#include "diagnostic.h"


// defines
#define MOTOR_TIMER_PERIOD  3200    // 25KHz (80000000 / 25000)
#define MOTOR_SPEED_MULT    1.52    // 3200/2100calc
#define DEADBAND            50      // 0 - 550


// function declerations
void MotorState(BYTE motor, MOTOR_STATE dir, WORD speed);

// global variables
int motorSpeed[4];    // motor value -1000 to +1000

// functions

// setup timers and initialize motors to off
void MotorInit(void)
{
    int i;

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
    M4_FORWARD_TRIS = OUTPUT_PIN;
    M4_FORWARD_IO = 0;
    M4_BACKWARD_TRIS = OUTPUT_PIN;
    M4_BACKWARD_IO = 0;

    OpenOC1(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    OpenOC2(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    OpenOC3(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    //OpenOC4(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE,2560,2560);
    OpenTimer3(T3_ON | T3_PS_1_1, MOTOR_TIMER_PERIOD);

}

void MotorProcess(void)
{
    int i;
    for(i = 1; i <= 4; i++)
    {
        WORD speed = motorSpeed[i-1];
        if(speed > MIN_MOTOR_THRESHOLD )
        {
            if(speed < MIN_MOTOR_SPEED)
            {
                speed = MIN_MOTOR_SPEED;
            }
            MotorState(i, MOTOR_FORWARD, speed);
        }
        else if(speed < -MIN_MOTOR_THRESHOLD)
        {
            if(speed > -MIN_MOTOR_SPEED)
            {
                speed = -MIN_MOTOR_SPEED;
            }
            MotorState(i, MOTOR_BACKWARD, -speed);
        }
        else
        {
            MotorState(i, MOTOR_STOP, 0);
        }
    }
}

/* Public function for nav to send motor commands to this driver
 * motor: 1 - 4
 * speed: -1000 to 1000 */
void MotorUpdate(BYTE motor, int speed)
{
    if(motor >= 1 && motor <= 4)
    {
        if(speed <= 1000 && speed >= -1000)
        {
            motorSpeed[motor-1] = speed;
        }
    }
}

// Update motor state.
//   MOTOR_STATE dir: MOTOR_STOP, MOTOR_FORWARD, MOTOR_BACKWARD
//   uint speed: 0 - 1000
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
            //debug("b");
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

    speed += 1000; // code originally setup to run on pwm pulses 1000-2000
    if(speed >= 900 && speed <= 2100)
    {
        if(motor == 1)
            SetDCOC1PWM((int)(speed * MOTOR_SPEED_MULT));
        else if(motor == 2)
            SetDCOC2PWM((int)(speed * MOTOR_SPEED_MULT));
        else if(motor == 3)
            SetDCOC3PWM((int)(speed * MOTOR_SPEED_MULT));
        //else if(motor == 4)
        //    SetDCOC4PWM((int)(speed * MOTOR_SPEED_MULT));
    }
}

void MotorsStop(void)
{
    MotorState(1, MOTOR_STOP, 0);
    MotorState(2, MOTOR_STOP, 0);
    MotorState(3, MOTOR_STOP, 0);
    //MotorState(4, MOTOR_STOP, 0);
}



/*
 Motor control routines mockup

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