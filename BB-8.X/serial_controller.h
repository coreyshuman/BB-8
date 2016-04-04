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

    #define SERIAL_BAUD_RATE        115200

    #define SER_TIMEOUT_INTERVAL    TICK_SECOND/10

    #define MAX_COMMAND_LENGTH      15
    #define MAX_ARGUMENT_COUNT      10
    #define MAX_ARGUMENT_LENGTH     20

    #define MAX_HANDLERS            10

    void SerialInit(void);
    void SerialProc(void);
    int SerialAddHandler(char* command, BYTE maxArguments, void (*func)(char**, int));


#ifdef	__cplusplus
}
#endif

#endif	/* SERIAL_CONTROLLER_H */

