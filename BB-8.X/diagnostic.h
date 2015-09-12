/* 
 * File:   diagnostic.h
 * Author: Corey
 *
 * Created on September 11, 2015, 7:59 PM
 */

#ifndef DIAGNOSTIC_H
#define	DIAGNOSTIC_H

#ifdef	__cplusplus
extern "C" {
#endif

    void DiagInit(void);
void DiagProcess(void);
void PrintAccelToOled(double pry[]);


#ifdef	__cplusplus
}
#endif

#endif	/* DIAGNOSTIC_H */

