/* 
 * File:   diagnostic.h
 * Author: Corey
 *
 * Created on September 11, 2015, 7:59 PM
 */

#ifndef DIAGNOSTIC_H
#define	DIAGNOSTIC_H

#define DBG_MPU         0x00000001
#define DBG_AUDIO       0x00000002


#define enableDiagFilter(x)      (debugMap |= x)
#define disableDiagFilter(x)     (debugMap &= ~x)
#define isDiagFilterOn(x)        (debugMap & x)

#ifdef	__cplusplus
extern "C" {
#endif

extern DWORD debugMap;

void DiagInit(void);
void DiagProcess(void);
void PrintAccelToOled(double pry[]);


#ifdef	__cplusplus
}
#endif

#endif	/* DIAGNOSTIC_H */

