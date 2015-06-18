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



#define MOTOR_TIMER_PERIOD  3200    // 25KHz (80000000 / 25000)
#define MOTOR_SPEED_MULT    1.52    // 3200/2100
#define DEADBAND            50      // 0 - 550
#define SPEED_SCALE         2.2

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
    int xPulse;
    int yPulse;
    int rotation;
    WORD speed;
    WORD angle;
    int speedPulse;
    long q[4];
    double qd[4];
    char string[50];
    double pry[3];

    // get RC data, scaled from -500 to +500
    xPulse = ReceiverGetPulse(1) - 1500;
    yPulse = ReceiverGetPulse(2) - 1500;
    rotation = ReceiverGetPulse(4) - 1500;

    // get IMU data, quaternion format
    get_quat(q);

    // send quat values to PC over serial, will use this later
    //sprintf(string, "%08X,%08X,%08X,%08X\r\n", (long)q[0], (long)q[1], (long)q[2], (long)q[3]);
    //debug(string);

    
    // convert quaternion to roll, pitch, yaw
    if(q[0] == 0 && q[1] == 0 && q[2] == 0 && q[3] == 0)
    {
        pry[0] = pry[1] = pry[2] = 0l;
    }
    else
    {
        qd[0] = (double)q[0];
        qd[1] = (double)q[1];
        qd[2] = (double)q[2];
        qd[3] = (double)q[3];
        //sprintf(string, "q: %8f %8f %8f %8f\r\n", qd[0], qd[1], qd[2], qd[3]);
        //debug(string);

        /*
        double w2 = qd[0]*qd[0];
        double x2 = qd[1]*qd[1];
        double y2 = qd[2]*qd[2];
        double z2 = qd[3]*qd[3];
        sprintf(string, "wxyz %08X,%08X,%08X,%08X\r\n", w2, x2, y2, z2);
        debug(string);
        double inner = w2 + x2 + y2 + z2;

        //sprintf(string, "q: %8f %8f %8f %8f\r\n", qd[0], qd[1], qd[2], qd[3]);
        //debug(string);
        // this refuses to work when numbers grow large, seems like PIC can't operate on such a large number with many parts
        //double inner = (double)((double)qd[0]*(double)qd[0] + (double)qd[1]*(double)qd[1] + (double)qd[2]*(double)qd[2] + (double)qd[3]*(double)qd[3]);
        sprintf(string, "inn: %08X\r\n", inner);
        debug(string);
        double mag = sqrt(inner);
         */
        // normalize quaternion
        double mag = sqrt(qd[0]*qd[0] + qd[1]*qd[1] + qd[2]*qd[2] + qd[3]*qd[3]);
        //sprintf(string, "mag: %f\r\n", mag);
        //debug(string);
        qd[0] = qd[0] / mag;
        qd[1] = qd[1] / mag;
        qd[2] = qd[2] / mag;
        qd[3] = qd[3] / mag;


        // adjust signs of quat
        qd[2] *= -1;
        qd[3] *= -1;

        //sprintf(string, "qd: %3f %3f %3f %3f\r\n", qd[0], qd[1], qd[2], qd[3]);
        //debug(string);

        // heavy painful quaternion to euler math here
        pry[1] = atan2(2*qd[0]*qd[1] + 2*qd[2]*qd[3], 1- 2*qd[1]*qd[1] + 2*qd[2]*qd[2]) * M_180_PI;
        pry[0] = asin(2*qd[0]*qd[2] - 2*qd[1]*qd[3]) * M_180_PI;
        pry[2] = atan2(2*qd[0]*qd[3] + 2*qd[1]*qd[2], 1 - 2*qd[2]*qd[2] + 2*qd[3]*qd[3]) * M_180_PI;
    }

    sprintf(string, "roll: %3f pitch: %3f yaw: %3f\r\n", pry[1], pry[0], pry[2]);
    debug(string);

    // calculate motor output from RC and IMU data
#ifdef DIRECT_MOTOR_CONTROL
    // One idea for merging motor control and imu data, all proportional
    // first convert roll and pitch to scale -500 to +500
    for(i=0;i<2;i++)
    {
        if(pry[i] > IMU_DEADBAND)
        {
            pry[i] = (pry[i]-IMU_DEADBAND) * MAX_SPEED / MAX_ANGLE;
        }
        else if(pry[i] < -IMU_DEADBAND)
        {
            pry[i] = (pry[i]+IMU_DEADBAND) * MAX_SPEED / MAX_ANGLE;
        }
        else
        {
            pry[i] = 0;
        }
    }

    // no mixin IMU data with RC data
    xPulse = xPulse + pry[0];
    yPulse = yPulse + pry[1];
#else
        // other option, use a PID controller
        // TODO
#endif

    // convert euler data to polar
    if(xPulse == 0 && yPulse == 0)
    {
        speed = 0;
    }
    else
    {
        speed = (int)sqrt(pow(xPulse,2) + pow(yPulse,2));
    }

    angle = (WORD)((atan2(xPulse, yPulse) * M_180_PI) + 360) % 360;

    //apply polar data to the 3 motors
    for(i=1;i<=3; i++)
    {
        speedPulse = calcSpeed(angle + 120*(i-1), speed);

        // Crappy implementation of rotation. In practice it actually works
        // works pretty well, but there must be a mathematically better
        // way to do it.
        speedPulse -= rotation;

        if(speedPulse > MAX_SPEED)
            speedPulse = MAX_SPEED;
        else if(speedPulse < -MAX_SPEED)
            speedPulse = -MAX_SPEED;

        //sprintf(string, "%d: %d\r\n", i, speedPulse);
        //debug(string);
               
        if(speedPulse > MIN_MOTOR_THRESHOLD )
        {
            if(speedPulse < MIN_MOTOR_SPEED)
            {
                speedPulse = MIN_MOTOR_SPEED;
            }
            MotorState(i, MOTOR_FORWARD, speedPulse*SPEED_SCALE + 1000);
        }
        else if(speedPulse < -MIN_MOTOR_THRESHOLD)
        {
            if(speedPulse > -MIN_MOTOR_SPEED)
            {
                speedPulse = -MIN_MOTOR_SPEED;
            }
            MotorState(i, MOTOR_BACKWARD, -speedPulse*SPEED_SCALE + 1000);
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

    if(speed >= 900 && speed <= 2100)
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