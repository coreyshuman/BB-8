/********************************************************************
 FileName:      navigation_controller.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Takes navigation commands and IMU data to produce motor and servo
 control signals.

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
  1.0   Initial release                            Corey Shuman 4/8/16


********************************************************************/
#include <plib.h>
#include <math.h>
#include "HardwareProfile.h"
#include "lib/time/Tick.h"
#include "motor_controller.h"
#include "receiver.h"
#include "mpu_support.h"
#include "console.h"
#include "servo_controller.h"
#include "diagnostic.h"
#include "serial_controller.h"
#include "navigation_controller.h"

// defines
#define M_180_PI            57.2958 // 180/pi
#define NAV_SCALE           500/0xFFFF // FFFF to 500
#define NAV_NEUTRAL         0x8000
#define SPEED_SCALE         2 // 500 to 1000
#define MAX_SPEED           500     // max speed in nav scale
#define NAV_DATA_TIMEOUT    TICK_SECOND/5 // data is only valid for 200ms
#define PITCH_RATIO         5.56    // (2000us - 1000us) / 180 degrees
                                    // used to convert 1000 resolution to 180 degrees

// global variables
// nav command values are range 0 to FFFF, where 8000 is neutral
WORD navData[8]; // x,y,yaw,headpitch,headyaw,---,---,digital
BOOL navArmed;
BOOL navAccelArmed; // allow accelerometer data to influence motors
BOOL autoVoice; // BB-8 speaking by itself
BOOL autoHeadMovement; // BB-8 looks around by itself
BOOL sleepMode; // disable all other modes
BOOL enableProjector; // BB-8 head projector
DWORD lastNavTick;
BOOL newNavData;
double pry_offset[3];
double pry[3];
int headAngleValues[2][10];
int headAngleAverage[2];
BYTE loadingAverageCnt;

// function definitions
WORD calcSpeed(WORD angle, WORD speed);
void ReadTelemetry(char arg[][MAX_ARGUMENT_LENGTH], int argc);
void ReadNavArm(char arg[][MAX_ARGUMENT_LENGTH], int argc);
void ReadAccelArm(char arg[][MAX_ARGUMENT_LENGTH], int argc);
void SetPRYOffset(void);

// functions

/* init */
void NavigationInit(void)
{
    int i, j;
    navArmed = FALSE;
    lastNavTick = 0;
    newNavData = FALSE;
    navAccelArmed = FALSE;
    autoVoice = FALSE;

    for(i = 0; i < 8; i++)
    {
        navData[i] = NAV_NEUTRAL;
    }

    for(i = 0; i < 3; i++)
    {
        pry[i] = 0;
        pry_offset[i] = 0;
    }

    loadingAverageCnt = 0;
    for(i=0; i<2; i++)
    {
        for(j=0; j<10; j++)
        {
            headAngleValues[i][j] = 1500;
        }
    }

    SerialAddHandler("tel", 8, ReadTelemetry);
    //SerialAddHandler("navarm", 1, ReadNavArm);
    //SerialAddHandler("navacl", 1, ReadAccelArm);
}

/* process */
void NavigationProcess(void)
{
    int i;
    int xSpeed;
    int ySpeed;
    int rotation;
    WORD speed;
    WORD angle;
    int motorSpeed;
    BOOL accelEnabled = FALSE;
    BOOL accelLastState = FALSE;
    long q[4];
    double qd[4];
    //char string[50];

    SetModule(MOD_NAV);

    if(newNavData)
    {
        lastNavTick = TickGet();
        newNavData = FALSE;

        // update digital states
        if(navData[7] & 0x8000) {
            navArmed = TRUE;
        } else {
            navArmed = FALSE;
        }
        if(navData[7] & 0x4000) {
            navAccelArmed = TRUE;
        } else {
            navAccelArmed = FALSE;
        }
        if(navData[7] & 0x2000 && !autoVoice) {
            autoVoice = TRUE;
            EnableAutoVoice(TRUE);
        } else if(!(navData[7] & 0x2000) && autoVoice) {
            autoVoice = FALSE;
            EnableAutoVoice(FALSE);
        }
        if(navData[7] & 0x1000) {
            autoHeadMovement = TRUE;
        } else {
            autoHeadMovement = FALSE;
        }
        if(navData[7] & 0x0800) {
            sleepMode = TRUE; // todo: implement
        } else {
            sleepMode = FALSE;
        }
        if(navData[7] & 0x0400) {
            enableProjector = TRUE; // todo: implement
        } else {
            enableProjector = FALSE;
        }
    }

    // if last nav data is expired, reset to neutral
    if(TickGet() - lastNavTick > NAV_DATA_TIMEOUT)
    {
        for(i = 0; i < 7; i++)
        {
            navData[i] = NAV_NEUTRAL;
        }
        navData[7] = 0; // digital
    }

    // get nav control data, scaled from -500 to +500
    xSpeed = (navData[1]-NAV_NEUTRAL) * NAV_SCALE;
    ySpeed = (navData[0]-NAV_NEUTRAL) * NAV_SCALE;
    rotation = (navData[2]-NAV_NEUTRAL) * NAV_SCALE;

    if(navArmed)
    {
        accelEnabled = navAccelArmed;
        mLED_4_On();
    }
    else
    {
        accelEnabled = FALSE; // override accel enable
        mLED_4_Off();
    }

    // set IMU offsets when accel is armed
    if(accelEnabled && !accelLastState)
    {
        accelLastState = TRUE;
        // reset accel level
        SetPRYOffset();
        mLED_3_On();
    }
    else if (!accelEnabled && accelLastState)
    {
        accelLastState = FALSE;
        mLED_3_Off();
    }

    // get IMU data, quaternion format
    get_quat(q);

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

        // normalize quaternion
        double mag = sqrt(qd[0]*qd[0] + qd[1]*qd[1] + qd[2]*qd[2] + qd[3]*qd[3]);
        //sprintf(string, "mag: %f\r\n", mag);
        //debug(string);
        if(mag > 0)
        {
            qd[0] = qd[0] / mag;
            qd[1] = qd[1] / mag;
            qd[2] = qd[2] / mag;
            qd[3] = qd[3] / mag;
        }

        // adjust signs of quat
        qd[2] *= -1;
        qd[3] *= -1;

        //sprintf(string, "qd: %3f %3f %3f %3f\r\n", qd[0], qd[1], qd[2], qd[3]);
        //debug(string);

        // heavy painful quaternion to euler math here
        pry[0] = atan2(2*qd[0]*qd[1] + 2*qd[2]*qd[3], 1- 2*qd[1]*qd[1] + 2*qd[2]*qd[2]) * M_180_PI; // pitch
        pry[1] = asin(2*qd[0]*qd[2] - 2*qd[1]*qd[3]) * M_180_PI; // roll
        pry[2] = atan2(2*qd[0]*qd[3] + 2*qd[1]*qd[2], 1 - 2*qd[2]*qd[2] + 2*qd[3]*qd[3]) * M_180_PI; // yaw

    }

    if(isDiagFilterOn(DBG_IMU)) {
        debug("pitch: %3f roll: %3f yaw: %3f\r\n", pry[0], pry[1], pry[2]);
    }

    // process servo values for head stalk
    // shift averages
    for(i=10; i>1; i--)
    {
        headAngleValues[0][i-1] = headAngleValues[0][i-2];
        headAngleValues[1][i-1] = headAngleValues[1][i-2];
    }

    // get values, scaled from -500 to 500
    headAngleValues[0][0] = (navData[3]-NAV_NEUTRAL) * NAV_SCALE;
    headAngleValues[1][0] = (navData[4]-NAV_NEUTRAL) * NAV_SCALE;
    if(headAngleValues[0][0] > 500)
        headAngleValues[0][0] = 500;
    else if(headAngleValues[0][0] < -500)
        headAngleValues[0][0] = -500;
    if(headAngleValues[1][0] > 500)
        headAngleValues[1][0] = 500;
    else if(headAngleValues[1][0] < -500)
        headAngleValues[1][0] = -500;

    // CTS TEST - try scaling user input -90,90 deg to -45,45 deg
    headAngleValues[0][0] /= 2;
    headAngleValues[1][0] /= 2;

    if(loadingAverageCnt < 10)
    {
        loadingAverageCnt ++;
    }
    else
    {
        // calc average
        headAngleAverage[0] = 0;
        headAngleAverage[1] = 0;
        for(i=0; i<10; i++)
        {
            headAngleAverage[0] += headAngleValues[0][i];
            headAngleAverage[1] += headAngleValues[1][i];
        }
        headAngleAverage[0] /= 10;
        headAngleAverage[1] /= 10;

        // adjust for pitch if accel is armed
        if(accelEnabled)
        {
            headAngleAverage[0] += pry[0] * PITCH_RATIO;
        }
        
        if(headAngleAverage[0] > 500)
        {
            headAngleAverage[0] = 500;
        }
        else if(headAngleAverage[0] < -500)
        {
            headAngleAverage[0] = -500;
        }

        // send value to servos
        ServoUpdate(headAngleAverage[0]*SPEED_SCALE, headAngleAverage[1]*SPEED_SCALE);
    }

    // calculate motor output from RC and IMU data
#ifdef DIRECT_MOTOR_CONTROL
    // One idea for merging motor control and imu data, all proportional
    // first convert roll and pitch to scale -500 to +500
    double val[3];
    double imuSpeed[3];
    for(i=0;i<=2;i++)
    {

        val[i] = pry[i] - pry_offset[i];
        if(val[i] > IMU_DEADBAND)
        {
            imuSpeed[i] = (val[i]-IMU_DEADBAND) * MAX_SPEED / MAX_ANGLE;
        }
        else if(val[i] < -IMU_DEADBAND)
        {
            imuSpeed[i] = (val[i]+IMU_DEADBAND) * MAX_SPEED / MAX_ANGLE;
        }
        else
        {
            imuSpeed[i] = 0;
        }
    }
    PrintAccelToOled(val, navArmed, accelEnabled, autoVoice);

    // don't run motors if not armed
    if(!navArmed)
    {
        MotorsStop();
        return;
    }

    if(navAccelArmed)
    {
        // now mixin IMU data with RC data
        xSpeed = xSpeed + imuSpeed[1];
        ySpeed = ySpeed + imuSpeed[0];
    }
#else
        // other option, use a PID controller
        // TODO
#endif

    // convert euler data to polar
    if(xSpeed == 0 && ySpeed == 0)
    {
        speed = 0;
    }
    else
    {
        speed = (int)sqrt(xSpeed*xSpeed + ySpeed*ySpeed);
    }

    angle = (WORD)((atan2(xSpeed, ySpeed) * M_180_PI) + 360) % 360;

    //apply polar data to the 3 motors
    for(i=1; i<=3; i++)
    {
        motorSpeed = calcSpeed(angle + 120*(i-1), speed);

        // Crappy implementation of rotation. In practice it actually
        // works pretty well, but there must be a mathematically better
        // way to do it.
        motorSpeed -= rotation;

        if(motorSpeed > MAX_SPEED)
            motorSpeed = MAX_SPEED;
        else if(motorSpeed < -MAX_SPEED)
            motorSpeed = -MAX_SPEED;

        //sprintf(string, "%d: %d\r\n", i, speedPulse);
        //debug(string);

        // send motor data to motor controller
        MotorUpdate(i, motorSpeed*SPEED_SCALE);
    }

}

/* Public function to get nav telemetry data */
WORD NavigationGetTelemetry(WORD chan)
{
    if(chan < 1 || chan > 8)
        return 0;

    return navData[chan-1];
}

/* Get Navigation Commands from wireless receiver*/
void ReadTelemetry(char arg[][MAX_ARGUMENT_LENGTH], int argc)
{
    int i;
    if(isDiagFilterOn(DBG_NAV)) {
        debug("READ TELEMETRY\r\n");
    }

    if(argc == 8)
    {
        for (i=0; i<8; i++)
        {
            navData[i] = strtol(arg[i], NULL, 16);
        }
        newNavData = TRUE;
        if(isDiagFilterOn(DBG_NAV)) {
            debug("throttle=%d, yaw=%d, pitch=%d, roll=%d\r\n", navData[0], navData[1], navData[2], navData[3]);
        }
    }
}

/* Get Navigation Arm/Disarm Command from wireless receiver*/
/*
void ReadNavArm(char arg[][MAX_ARGUMENT_LENGTH], int argc)
{
    if(isDiagFilterOn(DBG_NAV)) {
        debug("READ ARM\r\n");
    }

    if(argc == 1)
    {
        if(arg[0][0] == '1')
            navArmed = TRUE;
        else
            navArmed = FALSE;
        debug("arm=%d\r\n", navArmed);
    }
}
*/
/* Get Navigation Arm/Disarm Command from wireless receiver*/
/*
void ReadAccelArm(char arg[][MAX_ARGUMENT_LENGTH], int argc)
{
    if(isDiagFilterOn(DBG_NAV)) {
        debug("READ ACCEL ARM\r\n");
    }

    if(argc == 1)
    {
        if(arg[0][0] == '1')
            navAccelArmed = TRUE;
        else
            navAccelArmed = FALSE;
        debug("accelArm=%d\r\n", navArmed);
    }
}
*/
/* convert polar angle/magnitude to indvidual motor scaled speed */
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

void SetPRYOffset(void)
{
    int i;

    for(i=0; i<3; i++)
    {
        pry_offset[i] = pry[i];
    }
}