/********************************************************************
 FileName:      exception_handler.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 Global expection handler for PIC32 microprocessor.

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
  1.0   Initial release                            Corey Shuman 8/22/15


********************************************************************/
#include <plib.h>
#include "HardwareProfile.h"
#include "motor_controller.h"
#include "OLED_driver.h"
#include "diagnostic.h"

static enum {
      EXCEP_IRQ = 0,            // interrupt
      EXCEP_AdEL = 4,            // address error exception (load or ifetch)
      EXCEP_AdES,                // address error exception (store)
      EXCEP_IBE,                // bus error (ifetch)
      EXCEP_DBE,                // bus error (load/store)
      EXCEP_Sys,                // syscall
      EXCEP_Bp,                // breakpoint
      EXCEP_RI,                // reserved instruction
      EXCEP_CpU,                // coprocessor unusable
      EXCEP_Overflow,            // arithmetic overflow
      EXCEP_Trap,                // trap (possible divide by zero)
      EXCEP_IS1 = 16,            // implementation specfic 1
      EXCEP_CEU,                // CorExtend Unuseable
      EXCEP_C2E                // coprocessor 2
  } _excep_code;

  static unsigned int _epc_code;
  static unsigned int _excep_addr;

  void setErrorLeds(BYTE status)
  {
      mLED_1 = !(status & 0x01);
      mLED_2 = !(status & 0x02);
      mLED_3 = !(status & 0x04);
      mLED_4 = !(status & 0x08);
  }

  void _general_exception_handler(void)
  {
      char buf[20];
      asm volatile("mfc0 %0,$13" : "=r" (_excep_code));
      asm volatile("mfc0 %0,$14" : "=r" (_excep_addr));

      _excep_code = (_excep_code & 0x0000007C) >> 2;

      switch(_excep_code){
        case EXCEP_IRQ: setErrorLeds(1);break;
        case EXCEP_AdEL: setErrorLeds(2);break;
        case EXCEP_AdES: setErrorLeds(3);break;
        case EXCEP_IBE: setErrorLeds(4);break;
        case EXCEP_DBE: setErrorLeds(5);break;
        case EXCEP_Sys: setErrorLeds(6);break;
        case EXCEP_Bp: setErrorLeds(7);break;
        case EXCEP_RI: setErrorLeds(8);break;
        case EXCEP_CpU: setErrorLeds(9);break;
        case EXCEP_Overflow: setErrorLeds(10);break;
        case EXCEP_Trap: setErrorLeds(11);break;
        case EXCEP_IS1: setErrorLeds(12);break;
        case EXCEP_CEU: setErrorLeds(13);break;
        case EXCEP_C2E: setErrorLeds(14);break;
      }

      INTDisableInterrupts();

      // stop timers
      OpenTimer1(T1_OFF | T1_PS_1_1, 1);
      OpenTimer2(T2_OFF | T2_PS_1_1, 1);
      OpenTimer3(T3_OFF | T3_PS_1_1, 1);
      OpenTimer4(T4_OFF | T4_PS_1_1, 1);

      // stop motors
      StopMotors();

      // print exception to screen
      OLED_clear();
      OLED_text(0, 0, "Exception", 1);
      snprintf(buf,20, "Addr:0x%0X",_excep_addr );
      OLED_text(0, 10, buf, 1);
      snprintf(buf,20, "Type:%d",_excep_code );
      OLED_text(0, 20, buf, 1);
      snprintf(buf,20, "Module:%d",debugModule );
      OLED_text(0, 30, buf, 1);
      OLED_write(OLED_ADDR);

      while (1) {
          // Examine _excep_code to identify the type of exception
          // Examine _excep_addr to find the address that caused the exception
      }
  }