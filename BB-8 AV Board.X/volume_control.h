/* 
 * File:   volume_control.h
 * Author: Corey
 *
 * Created on May 10, 2016, 8:11 AM
 */

#ifndef VOLUME_CONTROL_H
#define	VOLUME_CONTROL_H

#ifdef	__cplusplus
extern "C" {
#endif

void Volume_Init(void);
void Volume_Proc(void);
BOOL Volume_Write(BYTE vol);
BOOL Volume_Mute(BOOL mute);


#ifdef	__cplusplus
}
#endif

#endif	/* VOLUME_CONTROL_H */

