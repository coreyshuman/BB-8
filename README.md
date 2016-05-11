# BB-8
Fully functioning BB-8 robotic project.

Work in progress...

Quick notes on this project:

Main Board
- Built on the PIC32 UBW32 "Bit Whacker" platform
- Requires Microchip Legacy Stack for USB drivers
- Memory allocation is setup to use USB bootloader
- Implements MPU-9150 9 DOF driver
- PMW Output to motors (3 wheel platform) and 2 servos for head movement
- Remote control via custom serial protocol over XBEE radio

BB-8 Audio:
- Built on XGS 16-bit PIC development board
- Custom wav audio trigger board software
- Streams wav files from an SD card
- Controlled via UART commands


Top Board:
- Arduino Nano
- Communicates with main board via I2C
- Controls body sphere LEDs (Neopixels)
- Functions as EEPROM storage for main board

Requires:
- MPLABX IDE v1.95 or greater
- Microchip legacy library TCP/IP and USB libraries
- MPLAB XC32 Compiler v1.20
- Arduino IDE

Visit my webiste for build photos and more information: http://technicalwizardry.coreyshuman.com/
