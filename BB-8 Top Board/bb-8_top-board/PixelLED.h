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
    void Init(int pin, int count, byte * nLedReg);
    void Proc(void);
    void SetPixel(int n, int r, int g, int b);
    void BeginUpdate(void);
    void SetTransition(byte transistion);
    void Update(void);
    //void Show(void);

  private:
    unsigned long tick;
    Adafruit_NeoPixel* pixels;
    int pixelCount;
    enum PIXEL_STATE {
      SM_INIT = 0,
      SM_IDLE,
      SM_ANIMATE
    };
    
    enum PIXEL_STATE sm;
    float curLedVal[15];
    float ledStep[15];
    byte *ledReg;
    unsigned int timeSteps;
    byte curTransition;
    bool animateReady;

  
};


#endif
