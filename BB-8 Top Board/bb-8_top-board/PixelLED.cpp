/* PixelLED.cpp
 * 
 * 
 * Corey Shuman
 * 1/28/16
 */
#include "Arduino.h"
#include "PixelLED.h"

#define ANIMATION_STEP    5



PixelLED::PixelLED()
{
  
}

void PixelLED::Init(int pin, int count, byte * nLedReg)
{
  tick = millis();
  static Adafruit_NeoPixel p = Adafruit_NeoPixel(count, pin, NEO_GRB + NEO_KHZ800);
  pixels = &p;
  pixels->begin();
  pixelCount = count;
  sm = SM_INIT;
  ledReg = nLedReg;
  animateReady = false;
  curTransition = 0x00;
  Serial.print("LedInit\n");
}

void PixelLED::Proc(void)
{
  int i;
  switch(sm)
  {
    case SM_INIT:
      sm = SM_IDLE;
      break;
    case SM_IDLE:
      if(animateReady)
      {
        sm = SM_ANIMATE;
        animateReady = false;
      }
      break;
    case SM_ANIMATE:
      if(millis() - tick > ANIMATION_STEP)
      {
        tick = millis();
        if(--timeSteps > 0)
        {
          Update();
        }
        else
        {
          sm = SM_IDLE;
        }
        
      }
      break;
    
  }
}



void PixelLED::SetPixel(int n, int r, int g, int b)
{
  if(n > 0 && n <= pixelCount)
  {
    uint32_t c;
    c = pixels->Color(r,g,b);
    pixels->setPixelColor(n-1, c); 
  }
}

void PixelLED::BeginUpdate(void)
{
  int i;
  if(curTransition > 0)
  {
    timeSteps = (curTransition * 78.125) / ANIMATION_STEP;
    for(i=0; i<pixelCount*3; i++)
    {
      ledStep[i] = (ledReg[i] - curLedVal[i]) / timeSteps;
      Serial.print(ledStep[i]);
      Serial.print(" ");
    }
    Serial.print("time: ");
    Serial.print(timeSteps);
  }
  else
  {
    Serial.print("fast ");
    timeSteps = 2;
    for(i=0; i<pixelCount*3; i++)
    {
      ledStep[i] = ledReg[i] - curLedVal[i];
      Serial.print(ledStep[i]);
      Serial.print(" ");
    }
  }
  Serial.println("");
  animateReady = true;
}

void PixelLED::SetTransition(byte transition)
{
  curTransition = transition;
}

void PixelLED::Update(void)
{
  int i;
  for(i=0; i<pixelCount*3; i++)
  {
    curLedVal[i] = curLedVal[i] + ledStep[i];
  }
  for(i=0; i<pixelCount; i++)
  {
    // grb to rgb ordering
    SetPixel(i+1, (byte)curLedVal[i*3+1], (byte)curLedVal[i*3], (byte)curLedVal[i*3+2]);
  }
  pixels->show();
}

