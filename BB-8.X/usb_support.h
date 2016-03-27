/* 
 * File:   usb_support.h
 * Author: Corey
 *
 * Created on May 17, 2015, 5:26 PM
 */

#ifndef USB_SUPPORT_H
#define	USB_SUPPORT_H

#include "usb_config.h"
#include "./USB/usb.h"
#include "./USB/usb_function_cdc.h"

#ifdef	__cplusplus
extern "C" {
#endif

void InitializeUSB(void);
void ProcessUSB(void);


#ifdef	__cplusplus
}
#endif

#endif	/* USB_SUPPORT_H */

