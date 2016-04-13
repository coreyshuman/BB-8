/* 
 * File:   lighting_controller.h
 * Author: Corey Shuman
 *
 * Created on April 10, 2016, 2:21 AM
 */

#ifndef LIGHTING_CONTROLLER_H
#define	LIGHTING_CONTROLLER_H

#ifdef	__cplusplus
extern "C" {
#endif

    void LightingInit(void);
    void LightingProcess(void);
    void SetLedColor(BYTE n, BYTE r, BYTE g, BYTE b);
    void UpdateLighting(void);
    void SendLightingData(void);



#ifdef	__cplusplus
}
#endif

#endif	/* LIGHTING_CONTROLLER_H */

