/* PixelLED.h
 * 
 * 
 * Corey Shuman
 * 1/28/16
 */

#ifndef PixelLED_h
#define PixelLED_h

#include <Adafruit_NeoPixel.h>

class PixelLED
{
  public:
    PixelLED();
    void Init(int pin, int count);
    void Proc(void);
    void SetPixel(int n, int r, int g, int b);
    void Show(void);

  private:
    unsigned long tick;
    int state;
    Adafruit_NeoPixel* pixels;
    int pixelCount;

  
};


#endif
