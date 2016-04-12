/* PixelLED.cpp
 * 
 * 
 * Corey Shuman
 * 1/28/16
 */
#include "Arduino.h"
#include "PixelLED.h"

PixelLED::PixelLED()
{
  
}

void PixelLED::Init(int pin, int count)
{
  state = 0;
  tick = millis();
  static Adafruit_NeoPixel p = Adafruit_NeoPixel(count, pin, NEO_GRB + NEO_KHZ800);
  pixels = &p;
  pixels->begin();
  pixelCount = count;
  Serial.print("LedInit\n");
}

void PixelLED::Proc()
{
  /*
  long unsigned t = millis();
  if(t - tick > 500)
  {
    Serial.print("LedTick\n");
    tick = t;
    uint32_t c;
    if(state == 0)
    {
      c = pixels->Color(150,0,0);
      state++;
      Serial.print("a\n");
    }
    else if(state == 1)
    {
      c = pixels->Color(0,150,0);
      state++;
      Serial.print("b\n");
    }
    else
    {
      c = pixels->Color(0,0,150);
      state = 0;
      Serial.print("c\n");
    }
    
    pixels->setPixelColor(0, c); 
    pixels->setPixelColor(1, c);
    pixels->show(); 
  }
  */
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

void PixelLED::Show(void)
{
  pixels->show(); 
}

