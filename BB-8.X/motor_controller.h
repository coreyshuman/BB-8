/* 
 * File:   motor_controller.h
 * Author: Corey
 *
 * Created on May 26, 2015, 8:29 PM
 */

#ifndef MOTOR_CONTROLLER_H
#define	MOTOR_CONTROLLER_H

#ifdef	__cplusplus
extern "C" {
#endif

    #define MAX_SPEED               550     // max speed in scaled pulses
    #define DIRECT_MOTOR_CONTROL
    #define MAX_ANGLE               30      // max frame will rotate before countering motion
    #define IMU_DEADBAND            7      // degrees of center deadband
    #define MIN_MOTOR_THRESHOLD     20      // minimum RC input that will trigger motion
    #define MIN_MOTOR_SPEED         50      // smallest motor speed to output to motors

    typedef enum {
        MOTOR_STOP = 0,
        MOTOR_FORWARD,
        MOTOR_BACKWARD
    } MOTOR_STATE;

    void MotorInit(void);
    void MotorProcess(void);
    void SetPRYOffset(void);
    void StopMotors(void);


#ifdef	__cplusplus
}
#endif

#endif	/* MOTOR_CONTROLLER_H */

