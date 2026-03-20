/***************************************************
  This handles clock display
	
  Timings.h

	Written by Avinash Bhaskaran, Miguel Gonzalez
	
	Last Modified: 09/14/23
 ****************************************************/


#ifndef _LCD_GRAPHICS_
#define _LCD_GRAPHICS_

#include <stdint.h>

	void LCD_Graphics_Init(int32_t x, int32_t y, int32_t radius, uint16_t color);
	
	void drawClockHand(uint16_t angle, uint16_t oldAngle, uint16_t handLength, uint16_t color);
 
	void LCD_Graphics_OutDigital(int32_t hours, int32_t minutes, int32_t seconds, int32_t militaryTime);
	
	void LCD_Graphics_DrawClock(int32_t hours, int32_t minutes, int32_t seconds, int32_t sixths, int32_t militaryTime);
	
	void ClockNumbers(uint16_t midX, uint16_t midY, uint16_t radius, uint16_t base);
	
	int32_t fixedPointCosine(uint16_t angle);
	
	int32_t fixedPointSine(uint16_t angle);
	
	int32_t normalizeAngle(uint16_t angle);
	
	void drawClockHand(uint16_t angle, uint16_t oldAngle, uint16_t handLength, uint16_t color); 
#endif
#include "stdint.h"
#include "./inc/ST7735.h"

