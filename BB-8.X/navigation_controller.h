/* 
 * File:   navigation_controller.h
 * Author: Corey
 *
 * Created on April 8, 2016, 1:04 AM
 */

#ifndef NAVIGATION_CONTROLLER_H
#define	NAVIGATION_CONTROLLER_H

#ifdef	__cplusplus
extern "C" {
#endif

    // mode
    #define DIRECT_MOTOR_CONTROL

    #define MAX_ANGLE               30      // max frame will rotate before countering motion
    #define IMU_DEADBAND            7      // degrees of center deadband

    WORD NavigationGetTelemetry(WORD chan);


#ifdef	__cplusplus
}
#endif

#endif	/* NAVIGATION_CONTROLLER_H */

