// This module will handle the LCD numerical graphics and display the alarm clock to the ST7735.


#include <stdint.h>
#include <stdio.h>
#include "./inc/ST7735.h"


#define POINTS 90 // number of points 
#define MAX_16BITS 32768
#define NUMBER_OFFSET 8 // Distance from the circle edge. Adjust as per your preference.


// values come from 90/64 * index = result, sin(result) = angle * 2^15, then repeat this for 
// the array

static const uint16_t SIN_TABLE[POINTS + 1] = {0, 571, 1143, 1714, 2285, 2855, 3425, 3993, 4560, 5126, 5690, 
	6252, 6812, 7371, 7927, 8480, 9032, 9580, 10125, 10668, 11207, 11743, 12275, 12803, 13327, 13848, 14364, 
	14876, 15383, 15886, 16383, 16876, 17364, 17846, 18323, 18794, 19260, 19720, 20173, 20621, 21062, 21497, 
	21926, 22347, 22762, 23170, 23571, 23964, 24351, 24730, 25101, 25465, 25821, 26169, 26509, 26841, 27165, 
	27481, 27788, 28087, 28377, 28659, 28932, 29196, 29451, 29697, 29935, 30163, 30381, 30591, 30791, 30982, 
	31164, 31336, 31498, 31651, 31794, 31928, 32051, 32165, 32270, 32364, 32449, 32523, 32588, 32643, 32688, 
	32723, 32748, 32763, 32768
};

// FUNCTION DECLARATIONS
void DrawCircle(int centerX, int midPointY, int radius, uint16_t color); 
void DrawLine(int startX, int startY, int endX, int endY, int shade);
int32_t fixedPointSine(uint16_t angle);
int32_t fixedPointCosine(uint16_t angle);
void ClockNumbers(uint16_t midX, uint16_t midY, uint16_t radius, uint16_t base);
void drawClockHand(uint16_t angle, uint16_t oldAngle, uint16_t radius, uint16_t color);
////////////////////////
static int32_t X, Y, Radius, OldMilitaryTime;
uint16_t prevHrAngle, prevMinAngle, prevSecAngle;

void LCD_Graphics_Init(int32_t x, int32_t y, int32_t radius, int16_t color) {
	X = x;
	Y = y;
	Radius = radius;
	prevHrAngle = 90;
	prevMinAngle = 90;
	prevSecAngle = 90;
	
	DrawCircle(x, y, radius, color);
	ClockNumbers(x, y, radius, 1);
}

#define PF2   (*((volatile uint32_t *)0x40025010)) // LED
void LCD_Graphics_DrawClock(int32_t hours, int32_t minutes, int32_t seconds, int32_t sixths, int32_t militaryTime) {
	if (militaryTime != OldMilitaryTime) {
		ClockNumbers(X, Y, Radius, militaryTime+1);
		OldMilitaryTime = militaryTime;
	}
	uint16_t base = (militaryTime)? 24 : 12;
	uint16_t secondHandAngle = seconds*6+sixths;		// 60*6 = 360, seconds are smallest unit of precision
	uint16_t minuteHandAngle = minutes * 6 + seconds/10; // 3600 seconds in rotation / 10 to get 360 deg
  uint16_t hourHandAngle = (360*hours)/base + (minuteHandAngle / base); // hour hand moves 0.5 degrees per minute
	
	if (prevHrAngle == hourHandAngle && prevMinAngle == minuteHandAngle && prevSecAngle == secondHandAngle) return;
	
	PF2 = ~PF2;
	PF2 = ~PF2;
	drawClockHand(secondHandAngle, prevSecAngle, Radius-15, ST7735_RED);
	drawClockHand(minuteHandAngle, prevMinAngle, Radius-20, ST7735_BLUE);
	drawClockHand(hourHandAngle, prevHrAngle, Radius-40, ST7735_WHITE);
	PF2 = ~PF2;
	
	prevSecAngle = secondHandAngle;
	prevMinAngle = minuteHandAngle;
	prevHrAngle = hourHandAngle;
}
	

void out2Dec(int32_t n) {
	ST7735_OutChar(n/10 + '0');
	ST7735_OutChar(n%10 + '0');
}

void LCD_Graphics_OutDigital(int32_t hours, int32_t minutes, int32_t seconds, int32_t militaryTime) {
	char* ampmString = "   ";
	
	if (militaryTime == 0) {
		
		if (hours < 12) {
			hours = (hours == 0) ? 12 : hours;
			ampmString = " AM";
		}
		else {
			hours = (hours == 12) ? 12 : hours - 12;
			
			ampmString = " PM";
		}
	}
		
	out2Dec(hours);
	ST7735_OutChar(':');
	out2Dec(minutes);
	ST7735_OutChar(':');
	out2Dec(seconds); 
	ST7735_OutString(ampmString);
}


// Utility Functions
static int absoluteValue(int num) {
    if (num < 0) {
        return -num;
    } else {
        return num;
    }
}

static int signNum(int num) {
    return (num > 0) - (num < 0);
}

// because a circle is symmetric in x and y axes, you can get one point like (x,y) and replicate it for the other
// quadrants thus you get four points, but because the main diagonals of a circle are also symmetrical, 
// you can get another four more points. 

static void plotSymmetricCirclePoints(int horizontalDir, int verticalDir, int midpointX, int midpointY, int shade) {
    ST7735_DrawPixel(midpointX + horizontalDir, midpointY + verticalDir, shade);
    ST7735_DrawPixel(midpointX - horizontalDir, midpointY + verticalDir, shade);
    ST7735_DrawPixel(midpointX + horizontalDir, midpointY - verticalDir, shade);
    ST7735_DrawPixel(midpointX - horizontalDir, midpointY - verticalDir, shade);
    ST7735_DrawPixel(midpointX + verticalDir, midpointY + horizontalDir, shade);
    ST7735_DrawPixel(midpointX - verticalDir, midpointY + horizontalDir, shade);
    ST7735_DrawPixel(midpointX + verticalDir, midpointY - horizontalDir, shade);
    ST7735_DrawPixel(midpointX - verticalDir, midpointY - horizontalDir, shade);
}

// Midpoint Circle Drawing
void DrawCircle(int centerX, int midPointY, int radius, uint16_t color) { // centerX, midPointY is the center of circle
	// radius is radius, and color is color of the circle
    int horizontalDir = 0, verticalDir = radius, point = 1 - radius;
	// horizontalDir is the x-coor from the circle's center, starts at 0 to begin drawing from the most top part of circle
	// verticalDir is the radius since we start at topmost circle, so edge of circle, so radius
	// point variable decides whether next pixel is drawn above the current pixel or is drawn above and one to the left
	
	// based on the equation for a circle: x^2 + y^2 = r^2, so because we start at topmost of circle, then x = 0,
	// y = r, so then it is 0^2 + r^2 = r^2, so it goes to p = 0, but because we need to calculate the next
	// point, so it is either (x, y - 1) or (x+1, y - 1), depending on if point is less than 0 or not respectively.
	// so the midpoint is (x + 1/2)^2 + (y - 1/2)^2 = r^2, so it then becomes x^2 + x + y^2 + y - 1/4 = r^2, 
	// multiply it by 4, so it becomes 4x^2 + 4x + 4y^2 + 4y - 1 = 4r^2, then because initial point is topmost
	// circle (0,r), therefore 4r^2 + 4r - 1 = 4r^2, so it then becomes point = 1-4 * radius, I just chose 1 - radius
	// so it plots the circle sufficiently. 
	plotSymmetricCirclePoints(horizontalDir, verticalDir, centerX, midPointY, color);
	
	// it then calls the function plotSymmetricCirclePoints

    while (horizontalDir < verticalDir) {
        horizontalDir++; 
        if (point <= 0) { // if point is less than 0, then midpoint is inside circle, so you draw pixel above 
					// current one
            point += 2 * horizontalDir + 1; 
        } else { // if not then the midpoint is outside of circle, then you draw pixel above the current pixel
					// and to the left.
            verticalDir = verticalDir - 1; // y - 1
            point += 2 * (horizontalDir - verticalDir) + 1; // update point to draw pixel above current pixel and 
					// to the left.
        }
        plotSymmetricCirclePoints(horizontalDir, verticalDir, centerX, midPointY, color);
    }
}

void DrawLine(int startX, int startY, int endX, int endY, int shade) {
    int deltaX = absoluteValue(endX - startX); // the difference between point x2 and point x1, where
	// startX is closer to beginning of line and endX is farther
    int deltaY = absoluteValue(endY - startY); // same but for y-direction
    int signX = signNum(endX - startX); // determines sign based x2 - x1
    int signY = signNum(endY - startY); // same but for y-dir
    int flagChange = 0, temporary, decide; // set flag to 0, temp and decide also to 0

    if (deltaY > deltaX) { // if the change in y-dir is greater than x-dir, then it means it is 
			// steeper in vertical dir than horizontal dir. So we swap y-dir to x-dir so we only iterate one
			// direction instead of two. 
        temporary = deltaX;
        deltaX = deltaY;
        deltaY = temporary;
        flagChange = 1;
			// the above four lines swap the values so that deltaX holds deltaY and deltaY holds deltaX values
			// then set flag.
    }

    decide = (2 * deltaY) - deltaX; // helps to step in both x and y dir

    for (int i = 0; i <= deltaX; i++) { // depending if the if statement is executed, we will iterate in whichever			
			//  axis, and is less than deltaX or deltaY. 
        ST7735_DrawPixel(startX, startY, shade);
        // while loop to check if decide is negative or non negative, this is because if decide 
			// is not negative, then next pixel should be diagonally placed, so step in both x and y directions
        while (decide >= 0) { 
            if (flagChange) { // if flag is set, then adjust x-coordinate because we switched x
							startX += signX;
						}
            else { // otherwise adjust y-coordinate and update variable to step in x-coord.
							startY += signY;
							decide -= 2 * deltaX;
						}
        }

        if (flagChange) { // increment y-coordinate if iterating over y-axis
					startY += signY;
				}
        else {
					startX += signX; // same here
				}

        decide += 2 * deltaY; // update variable to step in y-coord.
    }
}


// given a circle, find a certain point to determine the angle based on the xy coordinate.

//16-bit fixed point sin matrix
int32_t normalizeAngle(uint16_t angle) {
	while(angle >= 360) {
        angle -= 360;
    }
	return angle;
}

int32_t fixedPointSine(uint16_t angle) {
    angle = normalizeAngle(angle);

    // using look up sine table
    if(angle <= 90) {
        return SIN_TABLE[angle];
    }
    else if(angle > 90 && angle <= 180) { // put logical AND such that it is in quadrant 2
        return SIN_TABLE[180 - angle]; 
    }
    else if(angle > 180 && angle <= 270) { // same but for quadrant 3
        return -SIN_TABLE[angle - 180];
    }
    else { // angle > 270 && angle < 360 // same but for quadrant 4
        return -SIN_TABLE[360 - angle];
    }
}

int32_t fixedPointCosine(uint16_t angle){
	angle = normalizeAngle(angle);
	angle = normalizeAngle(450-angle);
	return fixedPointSine(angle); // using trig complementary equation: cos(theta) = sin(90 - theta)
}


void drawClockHand(uint16_t angle, uint16_t oldAngle, uint16_t handLength, uint16_t color){
		
		uint16_t handEndX = X + ((fixedPointSine(angle) * handLength) >> 15);
    uint16_t handEndY = Y - ((fixedPointCosine(angle) * handLength) >> 15);
		uint16_t oldEndX =  X + ((fixedPointSine(oldAngle) * handLength)>> 15);
    uint16_t oldEndY =  Y - ((fixedPointCosine(oldAngle) * handLength) >> 15);
    
		ST7735_Line(X, Y, oldEndX, oldEndY, ST7735_BLACK);
    ST7735_Line(X, Y, handEndX, handEndY, color);
}

void ClockNumbers(uint16_t midX, uint16_t midY, uint16_t radius, uint16_t base) {
	char numToStr[3] = "  "; // to hold numbers as strings
    int32_t numberX, numberY;
    
    for(int iterative = 1; iterative <= 12; iterative++) {
        int angle = normalizeAngle(450 - (30*iterative));

        int32_t sinVal = fixedPointSine(angle);
        int32_t cosVal = fixedPointCosine(angle);

        // Calculate the position for the number based on sine and cosine values which is written earlier
        // We subtract NUMBER_OFFSET to place the numbers a little inside the clock's circumference.
			// Note: NUMBER_OFFSET is a value that will need to be changed based on clock's
			// parameters, maybe a better way to do this.
        numberX =  midX + (cosVal * (radius - NUMBER_OFFSET) >> 15);
        numberY =  midY - (sinVal * (radius - NUMBER_OFFSET) >> 15);

        // Convert the number to string for display
				uint16_t displayNum = base*iterative;
        if(displayNum < 10) {
            numToStr[0] = '0' + displayNum;
        } else {
            numToStr[0] = '0' + displayNum/10;
            numToStr[1] = '0' + displayNum%10;
        }

        // Now display the number, I believe this function from ST7735.c will help with displaying. 
				for (int i = 0; numToStr[i] != '\0'; i++){
					ST7735_DrawCharS(numberX-4, numberY-4, numToStr[i], ST7735_WHITE, ST7735_BLACK, 1);
					numberX += 6;
				}
       
    }
}



