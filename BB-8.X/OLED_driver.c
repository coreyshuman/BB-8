/********************************************************************
 FileName:      motor_controller.c
 Dependencies:  See INCLUDES section
 Processor:		PIC32 USB Microcontrollers
 Hardware:		Designed for use on UBW32 "Bit Whacker"
                           development boards.
 Complier:  	XC32 (for PIC32)

 OLED 128x64 driver for SSD1306 I2C Display

 Derived from driver for PIC16F1788 hardware by superfranz
 Original at: https://www.ccsinfo.com/forum/viewtopic.php?t=52861

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
  1.0   Initial release                            Corey Shuman 9/4/15


********************************************************************/

#include <plib.h>
#include <math.h>
#include "HardwareProfile.h"
#include "TCPIPConfig.h"
#include "OLED_driver.h"
#include "device_i2c.h"

#define bit_test(D,i) (D & (0x01ul << i))




const char TEXT[51][5] =
{

   0x00, 0x00, 0x00, 0x00, 0x00, // SPACE
   0x00, 0x00, 0x5F, 0x00, 0x00, // !
   0x00, 0x03, 0x00, 0x03, 0x00, // "
   0x14, 0x3E, 0x14, 0x3E, 0x14, // #
   0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
   0x43, 0x33, 0x08, 0x66, 0x61, // %
   0x36, 0x49, 0x55, 0x22, 0x50, //&
   0x00, 0x05, 0x03, 0x00, 0x00, // '
   0x00, 0x1C, 0x22, 0x41, 0x00, // (
   0x00, 0x41, 0x22, 0x1C, 0x00, //)
   0x14, 0x08, 0x3E, 0x08, 0x14, // *
   0x08, 0x08, 0x3E, 0x08, 0x08, // +
   0x00, 0x50, 0x30, 0x00, 0x00, //,
   0x08, 0x08, 0x08, 0x08, 0x08, // -
   0x00, 0x60, 0x60, 0x00, 0x00, // .
   0x20, 0x10, 0x08, 0x04, 0x02, // /
   0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
   0x04, 0x02, 0x7F, 0x00, 0x00, // 1
   0x42, 0x61, 0x51, 0x49, 0x46, // 2
   0x22, 0x41, 0x49, 0x49, 0x36, // 3
   0x18, 0x14, 0x12, 0x7F, 0x10, // 4
   0x27, 0x45, 0x45, 0x45, 0x39, // 5
   0x3E, 0x49, 0x49, 0x49, 0x32, // 6
   0x01, 0x01, 0x71, 0x09, 0x07, // 7
   0x36, 0x49, 0x49, 0x49, 0x36, // 8
   0x26, 0x49, 0x49, 0x49, 0x3E, // 9
   0x00, 0x36, 0x36, 0x00, 0x00, // :
   0x00, 0x56, 0x36, 0x00, 0x00, //;

   0x08, 0x14, 0x22, 0x41, 0x00, // <
   0x14, 0x14, 0x14, 0x14, 0x14, // =
   0x00, 0x41, 0x22, 0x14, 0x08, // >
   0x02, 0x01, 0x51, 0x09, 0x06, // ?
   0x3E, 0x41, 0x59, 0x55, 0x5E, // @
   0x7E, 0x09, 0x09, 0x09, 0x7E, // A
   0x7F, 0x49, 0x49, 0x49, 0x36, // B
   0x3E, 0x41, 0x41, 0x41, 0x22, // C
   0x7F, 0x41, 0x41, 0x41, 0x3E, // D
   0x7F, 0x49, 0x49, 0x49, 0x41, // E
   0x7F, 0x09, 0x09, 0x09, 0x01, // F
   0x3E, 0x41, 0x41, 0x49, 0x3A, // G
   0x7F, 0x08, 0x08, 0x08, 0x7F, // H
   0x00, 0x41, 0x7F, 0x41, 0x00, // I
   0x30, 0x40, 0x40, 0x40, 0x3F, // J
   0x7F, 0x08, 0x14, 0x22, 0x41, // K
   0x7F, 0x40, 0x40, 0x40, 0x40, // L
   0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
   0x7F, 0x02, 0x04, 0x08, 0x7F, // N
   0x3E, 0x41, 0x41, 0x41, 0x3E, // O
   0x7F, 0x09, 0x09, 0x09, 0x06, // P
   0x1E, 0x21, 0x21, 0x21, 0x5E, // Q
   0x7F, 0x09, 0x09, 0x09, 0x76  // R
};

const char TEXT2[44][5]=
{
   0x26, 0x49, 0x49, 0x49, 0x32, // S
   0x01, 0x01, 0x7F, 0x01, 0x01, // T
   0x3F, 0x40, 0x40, 0x40, 0x3F, // U
   0x1F, 0x20, 0x40, 0x20, 0x1F, // V
   0x7F, 0x20, 0x10, 0x20, 0x7F, // W
   0x41, 0x22, 0x1C, 0x22, 0x41, // X
   0x07, 0x08, 0x70, 0x08, 0x07, // Y
   0x61, 0x51, 0x49, 0x45, 0x43, // Z
   0x00, 0x7F, 0x41, 0x00, 0x00, // [
   0x02, 0x04, 0x08, 0x10, 0x20, /* \ */
   0x00, 0x00, 0x41, 0x7F, 0x00, // ]
   0x04, 0x02, 0x01, 0x02, 0x04, // ^
   0x40, 0x40, 0x40, 0x40, 0x40, // _
   0x00, 0x01, 0x02, 0x04, 0x00, // `
   0x20, 0x54, 0x54, 0x54, 0x78, // a
   0x7F, 0x44, 0x44, 0x44, 0x38, // b
   0x38, 0x44, 0x44, 0x44, 0x44, // c
   0x38, 0x44, 0x44, 0x44, 0x7F, // d
   0x38, 0x54, 0x54, 0x54, 0x18, // e
   0x04, 0x04, 0x7E, 0x05, 0x05, // f
   0x08, 0x54, 0x54, 0x54, 0x3C, // g
   0x7F, 0x08, 0x04, 0x04, 0x78, // h
   0x00, 0x44, 0x7D, 0x40, 0x00, // i
   0x20, 0x40, 0x44, 0x3D, 0x00, // j
   0x7F, 0x10, 0x28, 0x44, 0x00, // k
   0x00, 0x41, 0x7F, 0x40, 0x00, // l
   0x7C, 0x04, 0x78, 0x04, 0x78, // m
   0x7C, 0x08, 0x04, 0x04, 0x78, // n
   0x38, 0x44, 0x44, 0x44, 0x38, // o
   0x7C, 0x14, 0x14, 0x14, 0x08, // p
   0x08, 0x14, 0x14, 0x14, 0x7C, // q
   0x00, 0x7C, 0x08, 0x04, 0x04, // r
   0x48, 0x54, 0x54, 0x54, 0x20, // s
   0x04, 0x04, 0x3F, 0x44, 0x44, // t
   0x3C, 0x40, 0x40, 0x20, 0x7C, // u
   0x1C, 0x20, 0x40, 0x20, 0x1C, // v
   0x3C, 0x40, 0x30, 0x40, 0x3C, // w
   0x44, 0x28, 0x10, 0x28, 0x44, // x
   0x0C, 0x50, 0x50, 0x50, 0x3C, // y
   0x44, 0x64, 0x54, 0x4C, 0x44, // z
   0x00, 0x08, 0x36, 0x41, 0x41, // {
   0x00, 0x00, 0xFF, 0x00, 0x00, // |
   0x41, 0x41, 0x36, 0x08, 0x00, // }
   0x01, 0x80, 0x01, 0x02, 0x01 // ~
}; 

char DISPLAY [1024];

void OLED_command(INT slave_addr, int com)
{
//   I2CStart(I2C1);
//   I2CSendByte(I2C1,ind);
//   I2CSendByte(I2C1,0x00);
//   I2CSendByte(I2C1,com);
//   I2CStop(I2C1);
    i2c_begin_transmission(I2C1, FALSE);
    i2c_send_address(I2C1, slave_addr, 0);
    i2c_write_byte(I2C1, 0x00);
    i2c_write_byte(I2C1, com);
    i2c_end_transmission(I2C1);

}

void OLED_write(INT slave_addr)
{
    LONG i;
    OLED_command (slave_addr, 0x21) ;
    OLED_command (slave_addr, 0x00);
    OLED_command (slave_addr, 127);
    OLED_command (slave_addr, 0x22) ;
    OLED_command (slave_addr, 0x00);
    OLED_command (slave_addr, 7);
    //I2CStart(I2C1);
    //I2CSendByte(I2C1,ind) ;
    //I2CSendByte(I2C1,0x40) ;
    i2c_begin_transmission(I2C1, FALSE);
    i2c_send_address(I2C1, slave_addr, 0);
    i2c_write_byte(I2C1, 0x40);
    for (i = 0; i < 1024; i++)
    {
        //I2CSendByte(I2C1,DISPLAY[i]);
        i2c_write_byte(I2C1, DISPLAY[i]);
    }

    //I2CStop(I2C1);
    i2c_end_transmission(I2C1);
}

void  OLED_init(void)
{
    // I2CSetFrequency(I2C1, GetPeripheralClock(), I2C1_CLOCK_FREQ);
    I2CConfigure ( I2C1, I2C_ENABLE_SLAVE_CLOCK_STRETCHING);
    //I2CSetFrequency ( I2C1, 48000000, 300000);
    I2CSetFrequency(I2C1, GetPeripheralClock(), I2C1_CLOCK_FREQ);
    I2CEnable(I2C1, TRUE);

    delay(500);
    int add = OLED_ADDR;
   OLED_command (add, 0xae) ;
   OLED_command (add, 0xa8);
   OLED_command (add, 0x3f);
   OLED_command (add, 0xd3);
   OLED_command (add, 0x00);
   OLED_command (add, 0x40);

   OLED_command (add, 0xa0);
   OLED_command (add, 0xa1);
   OLED_command (add, 0xc0);

   OLED_command (add, 0xc8);
   OLED_command (add, 0xda);
   OLED_command (add, 0x12);
   OLED_command (add, 0x81);
   OLED_command (add, 0xfF);
   OLED_command (add, 0xa4);
   OLED_command (add, 0xa6) ;
   OLED_command (add, 0xd5);
   OLED_command (add, 0x80);
   OLED_command (add, 0x8d);
   OLED_command (add, 0x14) ;
   OLED_command (add, 0xAF) ;
   OLED_command (add, 0x20) ;
   OLED_command (add, 0x00) ;
}

void OLED_pixel(LONG x,long y)
{
   LONG nt;
   LONG pagina;
   LONG bit;
   pagina = y /8;
   bit= y-(pagina*8);
   nt= DISPLAY[pagina*128+x];
   nt |= 1 << bit;
   DISPLAY[pagina*128+x] = nt;
}

   VOID OLED_line (int x1, int y1, int x2, int y2)
{
       int x, y, addx, addy, dx, dy;
       long P;
      INT i;
      dx = abs ( ( int) (x2 - x1));
      dy = abs ( ( int) (y2 - y1));
      x = x1;
      y = y1;

      if (x1 > x2)
         addx = -1;

      else
      addx = 1;

      if (y1 > y2)
         addy = -1;

      else
      addy = 1;

      if (dx >= dy)
      {
         P = 2 * dy - dx;
         for (i = 0; i<= dx;  ++i)
         {
            OLED_pixel (x, y);

            if (P < 0)
            {
               P += 2*dy;
               x += addx;
            }

            else
            {
               P += 2*dy - 2 * dx;
               x += addx;
               y += addy;
            }
         }
      }

      else
      {
         P = 2 * dx - dy;
         for (i = 0; i<= dy;  ++i)
         {
            OLED_pixel (x, y);

            if (P < 0)
            {
               P += 2*dx;
               y += addy;
            }

            else
            {
               P += 2*dx - 2 * dy;
               x += addx;
               y += addy;
            }
         }
      }
}

   VOID OLED_circle(int x, int y, int radius, BOOL riemp)
   {
       int a, b, P;
      a = 0;
      b = radius;
      P = 1 - radius;

      do
      {
         if (riemp)
         {
            OLED_line (x - a, y+b, x + a, y + b);
            OLED_line (x - a, y-b, x + a, y - b);
            OLED_line (x - b, y+a, x + b, y + a);
            OLED_line (x - b, y-a, x + b, y - a);
         }

         else
         {
            OLED_pixel (a + x, b+y);
            OLED_pixel (b + x, a+y);
            OLED_pixel (x - a, b+y);
            OLED_pixel (x - b, a+y);
            OLED_pixel (b + x, y-a);
            OLED_pixel (a + x, y-b);
            OLED_pixel (x - a, y-b);
            OLED_pixel (x - b, y-a);
         }

         if (P < 0)
            P += 3 + 2 * a++;

         else
         P += 5 + 2 * (a++ - b--);
      } while (a <= b);
   }

   VOID OLED_text(int x, int y, char* textptr, int size)
   {
      INT i, j, k, l, m;     // Loop counters
      BYTE pixelData[5];     // Stores character data
      for (i = 0; textptr[i] != '\0'; ++i, ++x) // Loop through the passed string
      {
         if (textptr[i] < 'S') // Checks if the letter is in the first text array
            memcpy (pixelData, TEXT[textptr[i] - ' '], 5) ;

         else if (textptr[i] <= '~') // Check if the letter is in the second array
            memcpy (pixelData, TEXT2[textptr[i] - 'S'], 5) ;

         else
         memcpy (pixelData, TEXT[0], 5); // DEFAULT to space
         if (x + 5 * size >= 128)  // Performs character wrapping
         {
            x = 0;       // Set x at far left position
            y += 7*size + 1;    // Set y at next position down
         }

         for (j = 0; j<5; ++j, x += size)  // Loop through character byte data
         {
            for (k = 0; k<7 * size; ++k)  // Loop through the vertical pixels
            {
               if (bit_test (pixelData[j], k)) // Check if the pixel should be set
               {
                  for (l = 0; l<size; ++l) // The next two loops change the
                  {
                     // character's size
                     for (m = 0; m<size; ++m)
                     {
                        OLED_pixel (x + m, y+k * size + l); // Draws the pixel
                     }
                  }
               }
            }
         }
      }
   }

   VOID OLED_rect(int x1, int y1, int x2, int y2, int riemp)
   {
      if (riemp)
      {
         INT y, ymax;      // Find the y min and max
         if (y1 < y2)
         {
            y = y1;
            ymax = y2;
         }

         else
         {
            y = y2;
            ymax = y1;
         }

         for (; y <= ymax; ++y)     // Draw lines to fill the rectangle
            OLED_line (x1, y, x2, y);
      }

      else
      {
         OLED_line (x1, y1, x2, y1); // Draw the 4 sides
         OLED_line (x1, y2, x2, y2);
         OLED_line (x1, y1, x1, y2);
         OLED_line (x2, y1, x2, y2);
      }
   }

   VOID OLED_logo()
   {
//      LONG x;
//      INT c;
//      for (x = 0; x < 1024; x++)
//      {
//         c= LOGOS[x];
//         DISPLAY[x] = c;
//      }

       // draw BB-8 logo
       OLED_circle(30,42,15,FALSE); //body
       OLED_circle(30,20,9,FALSE); //head
       OLED_circle(30,16,2,FALSE);//eye
       OLED_circle(24,50,5,TRUE); //circle in body
       OLED_circle(36,36,5,TRUE);
       OLED_line(36,14,36,6);
       OLED_text(54,10,"BB-8",3);
       OLED_text(54,42,"Corey",2);
   }

   VOID OLED_clear()
   {
      LONG x;
      for (x = 0; x < 1024; x++)
      {
         DISPLAY[x] = 0x00;
      }
   }

//   VOID main()
//   {
//      CHAR txt[100];
//      OLED_init(OLED);
//
//      while (1)
//      {
//
//         OLED_clear();
//         logo();
//         OLED_write(OLED);
//         delay(3000);
//         OLED_clear();
//         sprintf(txt,"superfranz!");
//         OLED_text(0,0,txt,2);
//         OLED_circle(15,40,15,0);
//         OLED_rect(35,25,65,55,1);
//         OLED_circle(80,40,15,1);
//         OLED_rect(90,25,120,55,0);
//         OLED_write(OLED);
//         delay(3000);
//      }
//   }