/* 
 * File:   console.h
 * Author: Corey
 *
 * Created on March 27, 2016, 2:59 AM
 */

#ifndef CONSOLE_H
#define	CONSOLE_H

#ifdef	__cplusplus
extern "C" {
#endif

void ConsoleInit(void);
void ConsoleProcess(void);
void ConsoleSend(const char *buf, unsigned int length);
void ConsolePut(const char c);
void debug(const char* str, ...);


#define res(x) x

#ifdef	__cplusplus
}
#endif

#endif	/* CONSOLE_H */

