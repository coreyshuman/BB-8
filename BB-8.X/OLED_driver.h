/* 
 * File:   OLED_driver.h
 * Author: Corey
 *
 * Created on September 4, 2015, 8:39 PM
 */

#ifndef OLED_DRIVER_H
#define	OLED_DRIVER_H

#ifdef	__cplusplus
extern "C" {
#endif

    //#define OLED 0x78
#define OLED_ADDR 0x3C
#define I2C1_CLOCK_FREQ 400000

void OLED_init();
void OLED_clear();
void OLED_logo();
void OLED_write(int address);
void OLED_text(int x, int y, char* textptr, int size);


#ifdef	__cplusplus
}
#endif

#endif	/* OLED_DRIVER_H */

