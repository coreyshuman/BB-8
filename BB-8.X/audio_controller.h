/* 
 * File:   audio_controller.h
 * Author: Corey
 *
 * Created on September 30, 2015, 10:29 PM
 */

#ifndef AUDIO_CONTROLLER_H
#define	AUDIO_CONTROLLER_H

#ifdef	__cplusplus
extern "C" {
#endif

#define AUDIO_BAUD_RATE	 38400

void AudioInit(void);
void AudioProcess(void);
void EnableAutoVoice(BOOL enable);

#ifdef	__cplusplus
}
#endif

#endif	/* AUDIO_CONTROLLER_H */

