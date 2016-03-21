/* 
 * File:   serial_controller.h
 * Author: Corey
 *
 * Created on March 21, 2016, 3:09 AM
 */

#ifndef SERIAL_CONTROLLER_H
#define	SERIAL_CONTROLLER_H

#ifdef	__cplusplus
extern "C" {
#endif

    #define SERIAL_BAUD_RATE	 115200

    void SerialInit(void);
    void SerialProc(void);


#ifdef	__cplusplus
}
#endif

#endif	/* SERIAL_CONTROLLER_H */

