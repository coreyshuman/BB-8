/********************************************************************
 FileName:      HardwareProfile - UBW32.h
 Dependencies:  See INCLUDES section
 Processor:     PIC32 USB Microcontrollers
 Hardware:      PIC32MX460F512L UWB32
 Compiler:      Microchip C32 (for PIC32)
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “Company”) for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  1.0   11/19/2004   Initial release
  2.1   02/26/2007   Updated for simplicity and to use common
                     coding style
  2.3   09/15/2008   Broke out each hardware platform into its own
                     "HardwareProfile - xxx.h" file
  2.4   05/17/2015   "Made UBW32 Profile"
********************************************************************/

#ifndef HARDWARE_PROFILE_UBW32_H
#define HARDWARE_PROFILE_UBW32_H

/*******************************************************************/
/******** USB stack hardware selection options *********************/
/*******************************************************************/
//This section is the set of definitions required by the MCHPFSUSB
//  framework.  These definitions tell the firmware what mode it is
//  running in, and where it can find the results to some information
//  that the stack needs.
//These definitions are required by every application developed with
//  this revision of the MCHPFSUSB framework.  Please review each
//  option carefully and determine which options are desired/required
//  for your application.

//#define USE_SELF_POWER_SENSE_IO
#define tris_self_power     TRISAbits.TRISA2    // Input
#define self_power          1

//#define USE_USB_BUS_SENSE_IO
#define tris_usb_bus_sense  TRISBbits.TRISB5    // Input
#define USB_BUS_SENSE       1

/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/******** Application specific definitions *************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/

/** Board definition ***********************************************/
//These defintions will tell the main() function which board is
//  currently selected.  This will allow the application to add
//  the correct configuration bits as wells use the correct
//  initialization functions for the board.  These defitions are only
//  required in the stack provided demos.  They are not required in
//  final application design.
//#define DEMO_BOARD PIC32MX795F512L_PIM
#define DEMO_BOARD UBW32

#define SYS_FREQ 80000000L
#define BAUD_RATE	 115200

/** LED ************************************************************/
#define mInitAllLEDs()      LATE = 0xFFF0; TRISE = 0xFFF0;

#define mLED_1              LATEbits.LATE3
#define mLED_2              LATEbits.LATE2
#define mLED_3              LATEbits.LATE1
#define mLED_4              LATEbits.LATE0

#define mGetLED_1()         mLED_1
#define mGetLED_2()         mLED_2
#define mGetLED_3()         mLED_3
#define mGetLED_4()         mLED_4

#define mLED_1_On()         mLED_1 = 1;
#define mLED_2_On()         mLED_2 = 1;
#define mLED_3_On()         mLED_3 = 1;
#define mLED_4_On()         mLED_4 = 1;

#define mLED_1_Off()        mLED_1 = 0;
#define mLED_2_Off()        mLED_2 = 0;
#define mLED_3_Off()        mLED_3 = 0;
#define mLED_4_Off()        mLED_4 = 0;

#define mLED_1_Toggle()     mLED_1 = !mLED_1;
#define mLED_2_Toggle()     mLED_2 = !mLED_2;
#define mLED_3_Toggle()     mLED_3 = !mLED_3;
#define mLED_4_Toggle()     mLED_4 = !mLED_4;

/** SWITCH *********************************************************/
#define mInitSwitch2()      TRISEbits.TRISE7=1;
#define mInitSwitch3()      TRISEbits.TRISE6=1;
#define mInitAllSwitches()  mInitSwitch2();mInitSwitch3();
#define sw2                 PORTEbits.RE7
#define sw3                 PORTEbits.RE6

/** I/O pin definitions ********************************************/
#define INPUT_PIN 1
#define OUTPUT_PIN 0

#define PORT_G7_TRIS         TRISGbits.TRISG7
#define PORT_G8_TRIS         TRISGbits.TRISG8

/** HARDWARE *******************************************************/
#define MPU_I2C I2C1A
// for empl; use global define
#ifndef FOOTSENSE_TARGET_PIC32
#define FOOTSENSE_TARGET_PIC32
#endif
#ifndef MPU9150
#define MPU9150
#endif

// Radio Receiver Definitions
#define RX_INPUT_COUNT 5

#define RX_INPUT_1_IO      PORTCbits.RC3
#define RX_INPUT_1_TRIS     TRISCbits.TRISC3
#define RX_INPUT_2_IO      PORTCbits.RC2
#define RX_INPUT_2_TRIS     TRISCbits.TRISC2
#define RX_INPUT_3_IO      PORTCbits.RC1
#define RX_INPUT_3_TRIS     TRISCbits.TRISC1
#define RX_INPUT_4_IO      PORTEbits.RE5
#define RX_INPUT_4_TRIS     TRISEbits.TRISE5
#define RX_INPUT_5_IO      PORTGbits.RG15
#define RX_INPUT_5_TRIS     TRISGbits.TRISG15
#define RX_INPUT_6_IO      PORTEbits.RE4
#define RX_INPUT_6_TRIS     TRISEbits.TRISE4
#define RX_INPUT_7_IO      PORTEbits.RE3
#define RX_INPUT_7_TRIS     TRISEbits.TRISE3
#define RX_INPUT_8_IO      PORTEbits.RE2
#define RX_INPUT_8_TRIS     TRISEbits.TRISE2



#define TEST_LED_IO     LATAbits.LATA7
#define TEST_LED_TRIS   TRISAbits.TRISA7

// Motor Controller Definitions
#define M1_FORWARD_IO           LATCbits.LATC13
#define M1_FORWARD_TRIS         TRISCbits.TRISC13
#define M1_BACKWARD_IO          LATDbits.LATD11
#define M1_BACKWARD_TRIS        TRISDbits.TRISD11
#define M2_FORWARD_IO           LATCbits.LATC14
#define M2_FORWARD_TRIS         TRISCbits.TRISC14
#define M2_BACKWARD_IO          LATDbits.LATD12
#define M2_BACKWARD_TRIS        TRISDbits.TRISD12
#define M3_FORWARD_IO           LATDbits.LATD7
#define M3_FORWARD_TRIS         TRISDbits.TRISD7
#define M3_BACKWARD_IO          LATDbits.LATD5
#define M3_BACKWARD_TRIS        TRISDbits.TRISD5

#endif  //HARDWARE_PROFILE_UBW32_H
