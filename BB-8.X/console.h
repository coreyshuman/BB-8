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

void ConsoleInit(void);
void PrintChar(char);
void PrintDec(char);
void ConsolePutROMString(const char* str);
#define Printf(x) ConsolePutROMString((const char*)x)
#define debug(x) ConsolePutROMString((const char*)x)

void UART_RX_PutByte(char c);
char UART_RX_GetByte(void);
unsigned char UART_RX_GetCount(void);

//#define res(x) Printf(#x);Printf(": ");PrintChar(x);Printf("\r\n")
//#define res(x) printf("%s: %d\r\n", #x, x);
#define res(x) x

#ifdef	__cplusplus
}
#endif

#endif	/* CONSOLE_H */

