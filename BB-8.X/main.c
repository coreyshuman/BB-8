/********************************************************************
 FileName:      main.c
 Dependencies:  See INCLUDES section
 Processor:		PIC18, PIC24, and PIC32 USB Microcontrollers
 Hardware:		This demo is natively intended to be used on Microchip USB demo
 				boards supported by the MCHPFSUSB stack.  See release notes for
 				support matrix.  This demo can be modified for use on other hardware
 				platforms.
 Complier:  	Microchip C18 (for PIC18), C30 (for PIC24), C32 (for PIC32)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the ?Company?) for its PIC? Microcontroller is intended and
 supplied to you, the Company?s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN ?AS IS? CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Description
  ----  -----------------------------------------
  1.0   Initial release

  2.1   Updated for simplicity and to use common
        coding style

  2.6a  Added button debouncing using Start-of-Frame packets

  2.7   Updated demo to place the PIC24F devices into sleep when the
        USB is in suspend.

  2.7b  Improvements to USBCBSendResume(), to make it easier to use.
********************************************************************/

/** CONFIGURATION **************************************************/
#pragma config UPLLEN   = ON        // USB PLL Enabled
#pragma config FPLLMUL  = MUL_15        // PLL Multiplier
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
 

/** I N C L U D E S **********************************************************/

#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "HardwareProfile.h"

#include "usb_support.h"
#include "TCPIP Stack/Tick.h"
#include "console.h"
#include "receiver.h"
#include "motor_controller.h"
#include "servo_controller.h"
#include "OLED_driver.h"
#include "diagnostic.h"
#include "audio_controller.h"

/** V A R I A B L E S ********************************************************/
#if defined(__18CXX)
    #pragma udata
#endif

BOOL stringPrinted;
volatile BOOL buttonPressed;
volatile BYTE buttonCount;

/** P R I V A T E  P R O T O T Y P E S ***************************************/
void InitializeSystem(void);
void IOInit(void);
void ProcessIO(void);
void USBDeviceTasks(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void USBCBSendResume(void);
void BlinkUSBStatus(void);
void UserInit(void);
void BlinkStatusLED(void);


/******************************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *****************************************************************************/

int main(void)
{
    InitializeSystem();

    while(1)
    {
        #if defined(USB_INTERRUPT)
            if(USB_BUS_SENSE && (USBGetDeviceState() == DETACHED_STATE))
            {
                USBDeviceAttach();
            }
        #endif

        #if defined(USB_POLLING)
		// Check bus status and service USB interrupts.
        USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
        				  // this function periodically.  This function will take care
        				  // of processing and responding to SETUP transactions
        				  // (such as during the enumeration process when you first
        				  // plug in).  USB hosts require that USB devices should accept
        				  // and process SETUP packets in a timely fashion.  Therefore,
        				  // when using polling, this function should be called
        				  // regularly (such as once every 1.8ms or faster** [see
        				  // inline code comments in usb_device.c for explanation when
        				  // "or faster" applies])  In most cases, the USBDeviceTasks()
        				  // function does not take very long to execute (ex: <100
        				  // instruction cycles) before it returns.
        #endif

   
		// Application-specific tasks.
		// Application related code may be added here, or in the ProcessIO() function.
        ProcessIO();
        BlinkStatusLED();
        MpuProcess();
        MotorProcess();
        ServoProcess();
        DiagProcess();
        AudioProcess();
    }//end while
}//end main


/********************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.
 *
 * Note:            None
 *******************************************************************/
void InitializeSystem(void)
{
    #if defined(__32MX460F512L__)|| defined(__32MX795F512L__)
    // Configure the PIC32 core for the best performance
    // at the operating frequency. The operating frequency is already set to
    // 60MHz through Device Config Registers
    SYSTEMConfigPerformance(SYS_FREQ);
    #endif

    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();

    IOInit();
    MotorInit();
    ConsoleInit();
    debug("\r\n********\r\nInitializing Board... \r\n");
    TickInit();
    debug("Tick_OK");
    UserInit();
    debug(" User_OK");
    OLED_init();
    debug(" OLED_INIT");
    OLED_clear();
    debug(" OLED_CLR");
    OLED_logo();
    debug(" OLED_LOGO");
    OLED_write(OLED_ADDR);
    debug(" OLED_WRITE");
    DiagInit();
    
    
    InitializeUSB();
    debug(" USB_OK");
    MpuInit();
    debug(" MPU_OK");
    ReceiverInit();
    debug(" RX_OK");
    ServoInit();
    debug( "SRV_OK");
    AudioInit();
    debug( "AIO_OK");


    debug("\r\nInit Complete.\r\n");

}//end InitializeSystem



/******************************************************************************
 * Function:        void UserInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine should take care of all of the demo code
 *                  initialization that is required.
 *
 * Note:
 *
 *****************************************************************************/
void UserInit(void)
{
    //Initialize all of the debouncing variables
    buttonCount = 0;
    buttonPressed = FALSE;
    stringPrinted = TRUE;

    //Initialize all of the LED pins
    mInitAllLEDs();
    

    //Initialize the pushbuttons
    mInitAllSwitches();
}//end UserInit

void IOInit(void)
{
    PORT_G7_TRIS = INPUT_PIN;
    PORT_G8_TRIS = OUTPUT_PIN;
}

/********************************************************************
 * Function:        void ProcessIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user
 *                  routines. It is a mixture of both USB and
 *                  non-USB tasks.
 *
 * Note:            None
 *******************************************************************/
void ProcessIO(void)
{
    BYTE numBytesRead;

    //Blink the LEDs according to the USB device status
    //BlinkUSBStatus();

    //cts test
    if(ReceiverGetPulse(6) > 1500)
    {
        mLED_3_On();
    }
    else
    {
        mLED_3_Off();
    }

    if(ReceiverGetPulse(8) > 1500)
    {
        mLED_4_On();
    }
    else
    {
        mLED_4_Off();
    }

//    // setup level after 3 seconds of power
//    DWORD currTick = TickGet();
//    static DWORD calibrateTick = 0l;
//    static int calibratedState = 0;
//    if(calibratedState == 0)
//    {
//        calibratedState = 1;
//        calibrateTick = currTick;
//    }
//    else if(calibratedState == 1 && (currTick - calibrateTick >= TICK_SECOND*3))
//    {
//        SetPRYOffset();
//        mLED_2_Toggle();
//        calibratedState = 2;
//    }
//
//    if(!buttonPressed && !sw2)
//    {
//        buttonPressed = 1;
//        SetPRYOffset();
//        mLED_2_Toggle();
//        debug("calibrate ");
//    }
    
    // User Application USB tasks
    if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1)) return;

    

    if(buttonPressed)
    {
        
        if(stringPrinted == FALSE)
        {
            if(USBUSARTIsTxTrfReady())
            {
                putrsUSBUSART("Button Pressed -- \r\n");
                stringPrinted = TRUE;
            }
        }
    }
    else
    {
        stringPrinted = FALSE;
    }

    ProcessUSB();
} //end ProcessIO

void BlinkStatusLED(void)
{
    static DWORD ledTick = 0L;

    DWORD currTick = TickGet();
    if(currTick - ledTick >= TICK_SECOND/4)
    {
        mLED_1_Toggle();
        ledTick = currTick;
        // CTS DEBUG
        if(USBUSARTIsTxTrfReady())
        {
            char string[30];
            //WORD val = 0;
            //int i;

                //val = Receiver_GetPulse(i+1);
                //sprintf(string, "RX: %u %u %u %u\r\n", Receiver_GetPulse(1), Receiver_GetPulse(2), Receiver_GetPulse(3), Receiver_GetPulse(4));
                //debug(string);
                //putrsUSBUSART(string);
                //putrsUSBUSART("corey shuman this is a long string to test if it has issues...\r\n");
           
        }
    }
}



/********************************************************************
 * Function:        void BlinkUSBStatus(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        BlinkUSBStatus turns on and off LEDs
 *                  corresponding to the USB device state.
 *
 * Note:            mLED macros can be found in HardwareProfile.h
 *                  USBDeviceState is declared and updated in
 *                  usb_device.c.
 *******************************************************************/
void BlinkUSBStatus(void)
{
    static WORD led_count=0;

    if(led_count == 0)led_count = 10000U;
    led_count--;

    #define mLED_Both_Off()         {mLED_3_Off();mLED_4_Off();}
    #define mLED_Both_On()          {mLED_3_On();mLED_4_On();}
    #define mLED_Only_1_On()        {mLED_3_On();mLED_4_Off();}
    #define mLED_Only_2_On()        {mLED_3_Off();mLED_4_On();}

    if(USBSuspendControl == 1)
    {
        if(led_count==0)
        {
            mLED_3_Toggle();
            if(mGetLED_3())
            {
                mLED_4_On();
            }
            else
            {
                mLED_4_Off();
            }
        }//end if
    }
    else
    {
        if(USBDeviceState == DETACHED_STATE)
        {
            mLED_Both_Off();
        }
        else if(USBDeviceState == ATTACHED_STATE)
        {
            mLED_Both_On();
        }
        else if(USBDeviceState == POWERED_STATE)
        {
            mLED_Only_1_On();
        }
        else if(USBDeviceState == DEFAULT_STATE)
        {
            mLED_Only_2_On();
        }
        else if(USBDeviceState == ADDRESS_STATE)
        {
            if(led_count == 0)
            {
                mLED_3_Toggle();
                mLED_4_Off();
            }//end if
        }
        else if(USBDeviceState == CONFIGURED_STATE)
        {
            if(led_count==0)
            {
                mLED_3_Toggle();
                if(mGetLED_3())
                {
                    mLED_4_Off();
                }
                else
                {
                    mLED_4_On();
                }
            }//end if
        }//end if(...)
    }//end if(UCONbits.SUSPND...)

}//end BlinkUSBStatus





//CTS TODO: need to decouple and move
/********************************************************************
 * Function:        void USBCB_SOF_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB host sends out a SOF packet to full-speed
 *                  devices every 1 ms. This interrupt may be useful
 *                  for isochronous pipes. End designers should
 *                  implement callback routine as necessary.
 *
 * Note:            None
 *******************************************************************/
void USBCB_SOF_Handler(void)
{
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.

    //This is reverse logic since the pushbutton is active low
    if(buttonPressed == sw2)
    {
        if(buttonCount != 0)
        {
            buttonCount--;
        }
        else
        {
            //This is reverse logic since the pushbutton is active low
            buttonPressed = !sw2;

            //Wait 100ms before the next press can be generated
            buttonCount = 100;
        }
    }
    else
    {
        if(buttonCount != 0)
        {
            buttonCount--;
        }
    }
}


/** EOF main.c *************************************************/