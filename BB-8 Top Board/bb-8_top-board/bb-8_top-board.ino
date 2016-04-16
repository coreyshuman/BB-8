/* bb-8_top-board.ino
 * 
 * 
 * Corey Shuman
 * 4/12/16
 */

#include <Adafruit_NeoPixel.h>
#include "PixelLED.h"
#include <Wire.h>
#include <EEPROM.h>

#define SCRATCH_SIZE   32
#define PIXEL_COUNT   5
#define PIXEL_PIN     2

const long interval = 500;           // interval at which to blink (milliseconds)
const int statusLed =  13;      // the number of the LED pin
const int pixelPin = 2;         // the pin number for neopixel led
const byte i2cAddress = 0x33;

int ledState = LOW;             // ledState used to set the LED
unsigned long previousMillis = 0;        // will store last time LED was updated
byte regPointer = 0;
byte statusRegister = 0;
byte settingsLengthH = 0;
byte settingsLengthL = 0;
byte ledRegister[15];
byte scratchRAM[SCRATCH_SIZE];

PixelLED pixel;

void receiveEvent(int len);
void requestEvent(void);
void handleControl(int len);
void updateLEDs(void);


void setup() {
  // init pins
  pinMode(1, INPUT);
  pinMode(pixelPin, OUTPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(11, INPUT);
  pinMode(12, INPUT);
  pinMode(statusLed, OUTPUT);
  pinMode(14, INPUT);
  pinMode(15, INPUT);
  pinMode(16, INPUT);
  pinMode(17, INPUT);
  pinMode(18, INPUT);
  pinMode(19, INPUT);
  pinMode(20, INPUT);
  pinMode(21, INPUT);

  
  
  int i;

  regPointer = 0;
  statusRegister = 0;
  settingsLengthH = 0;
  settingsLengthL = 0;
  
  for(i=0; i<15; i++)
  {
    ledRegister[i] = 0;
  }
  
  for(i=0; i<SCRATCH_SIZE; i++)
  {
    scratchRAM[i] = 0;
  }

  Wire.begin(i2cAddress);                // join i2c bus with address 
  Wire.onReceive(receiveEvent);       // register event
  Wire.onRequest(requestEvent);       // register event

  pixel.Init(PIXEL_PIN, PIXEL_COUNT, ledRegister);

  Serial.begin(115200);
  
}

void loop() {
  unsigned long currentMillis = millis();
  // keep track of time passed
  if(currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(statusLed, ledState);
    Serial.print("a\n");
  }

  pixel.Proc();
}

void receiveEvent(int len)
{
  byte data;
  if(len--)
  {
    regPointer = Wire.read();
  }
  Serial.print("rx: ");
  Serial.print(regPointer);
  Serial.print(" - ");
  if(len)
  {
    if(regPointer >= 0x20)
    {
      Serial.print("write scratch\n");
      while(Wire.available())
      {
        data = Wire.read();
        if(regPointer <= 0x40)
        {
          scratchRAM[regPointer-0x20] = data;
          regPointer++;
        }
      }
    }
    else if(regPointer == 0x10)
    {
      Serial.print("set tran\n");
      data = Wire.read();
      pixel.SetTransition(data);
    }
    else if(regPointer >= 0x11)
    {
      Serial.print("write led\n");
      while(Wire.available())
      {
        data = Wire.read();
        if(regPointer <= 0x1F)
        {
          ledRegister[regPointer-0x11] = data;
          regPointer++;
        }
      }
    }
    else
    {
      Serial.print("command\n");
      switch(regPointer)
      {
        case 0x00: // control register
          handleControl(len);
          break;
        case 0x01:
          break;
        case 0x02:
          break;
        case 0x03:
          break;
        default:
          regPointer = 0;
          break;
      }
    }
  }
}

void handleControl(int len)
{
  byte data = Wire.read();
  len --;
  switch(data) {
    case 0x01: // update LEDs
      Serial.print("LED Update\n");
      pixel.BeginUpdate();
      break;
    case 0x02: // led effect
      ledEffect();
      break;
    case 0x09: // calc CRC
      break;
    case 0x10: // write to EEPROM
      break;
    case 0x11: // read from EEPROM
      break;
    
  }
}

void requestEvent(void)
{
  if(regPointer == 0x00){
    Wire.write(0x00);
  } else if(regPointer == 0x01) {
    Wire.write(statusRegister);
  } else if(regPointer == 0x02) {
    Wire.write(settingsLengthH);
  } else if(regPointer == 0x03) {
    Wire.write(settingsLengthL);
  } else if(regPointer >= 0x11 && regPointer <= 0x1F) {
    Wire.write(ledRegister[regPointer-0x11]);
  } else if(regPointer >= 0x20 && regPointer <= 0x40) {
    Wire.write(scratchRAM[regPointer-0x20]);
  } else {
    regPointer = 0;
    Wire.write(0x00);
  }
  regPointer++;
}

void ledEffect(void)
{
  
}

unsigned long CRC(unsigned int len) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (int index = 0; index < len; ++index) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;

}
