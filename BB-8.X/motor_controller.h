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

    
    
    
    #define MIN_MOTOR_THRESHOLD     20      // minimum RC input that will trigger motion
    #define MIN_MOTOR_SPEED         50      // smallest motor speed to output to motors

    typedef enum {
        MOTOR_STOP = 0,
        MOTOR_FORWARD,
        MOTOR_BACKWARD
    } MOTOR_STATE;

    void MotorInit(void);
    void MotorProcess(void);
    void MotorsStop(void);
    void MotorUpdate(BYTE motor, int speed);


#ifdef	__cplusplus
}
#endif

#endif	/* MOTOR_CONTROLLER_H */

