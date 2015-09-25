/********************************************************************
 FileName:      exception.h
 Dependencies:  See INCLUDES section
 Processor:		PIC24 Microcontrollers
 Hardware:		Designed for use on XGS game
                           development boards.
 Complier:  	C30

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
  1.0   Initial release                            Corey Shuman 9/19/15


********************************************************************/

#ifndef EXCEPTION_H
#define	EXCEPTION_H

#ifdef	__cplusplus
extern "C" {
#endif

void ThrowException(char * msg);

void ThrowError(char * module, char * msg);


#ifdef	__cplusplus
}
#endif

#endif	/* EXCEPTION_H */

