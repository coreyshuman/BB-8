/********************************************************************
 FileName:      mpu_support.h
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Based on work by ntavish.
 Original work: https://github.com/ntavish/mpu9150-pic32

 Ported to work on UBW32 boards, decoupled processes from main.c
        to provide a more library-like functionality.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Description                                Name and date
  ----  -----------------------------------------  ----------------
  1.0   Initial release                            Corey Shuman 5/18/15


********************************************************************/

#ifndef MPU_SUPPORT_H
#define	MPU_SUPPORT_H

#ifdef	__cplusplus
extern "C" {
#endif

void MpuInit(void);
void MpuProcess(void);

void get_quat(long *val);


#ifdef	__cplusplus
}
#endif

#endif	/* MPU_SUPPORT_H */

