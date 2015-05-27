/* 
 * File:   receiver.h
 * Author: Corey
 *
 * Created on May 19, 2015, 12:18 AM
 */

#ifndef RECEIVER_H
#define	RECEIVER_H

#ifdef	__cplusplus
extern "C" {
#endif

void ReceiverInit(void);
WORD ReceiverGetPulse(WORD chan);


#ifdef	__cplusplus
}
#endif

#endif	/* RECEIVER_H */

