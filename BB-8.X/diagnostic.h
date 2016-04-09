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
#define DBG_SERIAL      0x00000004
#define DBG_NAV         0x00000008


#define enableDiagFilter(x)      (debugMap |= x)
#define disableDiagFilter(x)     (debugMap &= ~x)
#define isDiagFilterOn(x)        (debugMap & x)

#ifdef	__cplusplus
extern "C" {
#endif
enum DIAG_MOD {
    MOD_INIT = 0,
    MOD_AUD = 1,
    MOD_CONS,
    MOD_DIAG,
    MOD_MOTOR,
    MOD_MPU,
    MOD_OLED,
    MOD_RECV,
    MOD_SERIAL,
    MOD_SERVO,
    MOD_USB,
    MOD_NAV

};
extern enum DIAG_MOD debugModule;
#define SetModule(x) (debugModule = x)

extern DWORD debugMap;


void DiagInit(void);
void DiagProcess(void);
void PrintAccelToOled(double vpry[], BOOL armed, BOOL accelEnabled);
BOOL DiagnosticTestMode(void);



#ifdef	__cplusplus
}
#endif

#endif	/* DIAGNOSTIC_H */

