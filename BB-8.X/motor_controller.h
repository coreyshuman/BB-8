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

    typedef enum {
        MOTOR_STOP = 0,
        MOTOR_FORWARD,
        MOTOR_BACKWARD
    } MOTOR_STATE;

    void MotorInit(void);


#ifdef	__cplusplus
}
#endif

#endif	/* MOTOR_CONTROLLER_H */

