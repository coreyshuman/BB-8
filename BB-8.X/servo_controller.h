/* 
 * File:   servo_controller.h
 * Author: Corey
 *
 * Created on August 19, 2015, 11:23 PM
 */

#ifndef SERVO_CONTROLLER_H
#define	SERVO_CONTROLLER_H

#ifdef	__cplusplus
extern "C" {
#endif

    void ServoInit(void);
    void ServoProcess(void);
    void ServoUpdatePitch(double aPitch);


#ifdef	__cplusplus
}
#endif

#endif	/* SERVO_CONTROLLER_H */

