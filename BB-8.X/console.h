/* 
 * File:   console.h
 * Author: tavish
 *
 * Created on February 27, 2014, 2:23 PM
 */

#ifndef CONSOLE_H
#define	CONSOLE_H

#ifdef	__cplusplus
extern "C" {
#endif


void PrintChar(char);
void PrintDec(char);
void ConsolePutROMString(const char* str);
#define Printf(x) ConsolePutROMString((const char*)x)


void debug(char* str, ...);

//#define res(x) Printf(#x);Printf(": ");PrintChar(x);Printf("\r\n")
//#define res(x) printf("%s: %d\r\n", #x, x);
#define res(x) x

#ifdef	__cplusplus
}
#endif

#endif	/* CONSOLE_H */

