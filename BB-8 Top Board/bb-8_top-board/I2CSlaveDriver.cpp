
/* I2CSlaveDriver.cpp
 * 
 * 
 * Corey Shuman
 * 4/12/16
 */
#include "Arduino.h"
#include "I2CSlaveDriver.h"
#include "Wire.h"
/*
I2CCom::I2CCom(void)
{
  
}

void I2CCom::Init(byte address)
{
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

  Wire.begin(address);                // join i2c bus with address 
  Wire.onReceive(receiveEvent);       // register event
  Wire.onRequest(requestEvent);       // register event

  pixel.Init(PIXEL_PIN, PIXEL_COUNT);
}

void I2CCom::Proc(void)
{
  pixel.Proc();
}

void I2CCom::receiveEvent(int len)
{
  byte data;
  if(len--)
  {
    regPointer = Wire.read();
  }

  if(len)
  {
    if(regPointer >= 0x20)
    {
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
    else if(regPointer >= 0x11)
    {
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
      switch(regPointer)
      {
        case 0x00: // control register
          handleControl();
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

void I2CCom::handleControl(void)
{
  data = Wire.read();
  len --;
  switch(data) {
    case 0x01: // update LEDs
      updateLEDs();
      break;
    case 0x09: // calc CRC
      break;
    case 0x10: // write to EEPROM
      break;
    case 0x11: // read from EEPROM
      break;
    
  }
}

void I2CCom::updateLEDs(void)
{
  int i;
  for(i=0 i<PIXEL_COUNT; i++)
  {
    pixel.SetPixel(i+1, ledRegister[i*3], ledRegister[i*3+1], ledRegister[i*3+2]);
  }
  pixel.Show();
}

void I2CCom::requestEvent(void)
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

unsigned long I2CCom::CRC(unsigned int len) {

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
*/
