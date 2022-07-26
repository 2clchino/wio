/*
    This example draws fonts (as used by the Adafruit_GFX library) onto the
    screen. These fonts are called the GFX Free Fonts (GFXFF) in this library.

    Other True Type fonts could be converted using the utility within the
    "fontconvert" folder inside the library. This converted has also been
    copied from the Adafruit_GFX library.

    Since these fonts are a recent addition Adafruit do not have a tutorial
    available yet on how to use the utility.   Linux users will no doubt
    figure it out!  In the meantime there are 48 font files to use in sizes
    from 9 point to 24 point, and in normal, bold, and italic or oblique
    styles.

    This example sketch uses both the print class and drawString() functions
    to plot text to the screen.

    Make sure LOAD_GFXFF is defined in the User_Setup.h file within the
    TFT_eSPI library folder.

    --------------------------- NOTE ----------------------------------------
    The free font encoding format does not lend itself easily to plotting
    the background without flicker. For values that changes on screen it is
    better to use Fonts 1- 8 which are encoded specifically for rapid
    drawing with background.
    -------------------------------------------------------------------------

    >>>>>>>>>>>>>>>>>>>>>>>>>>> WARNING <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    As supplied with the default settings the sketch has 11 fonts loaded,
    i.e. GLCD (Font 1), Font 2, Font 4, Font 6, Font 7, Font 8 and five Free Fonts,
    even though they are not all used in the sketch.

    Disable fonts you do not need in User_Setup.h in the library folder.

    #########################################################################
    ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
    #########################################################################
*/

#include "Free_Fonts.h" // Include the header file attached to this sketch

#include "SPI.h"
#include "TFT_eSPI.h"

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

unsigned long drawTime = 0;

void setup(void) {
    Serial.begin(115200);
    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    
    tft.begin();

    tft.setRotation(3);

}

void loop() {

    int xpos =  0;
    int ypos = 220;

    int ox = 0;
    int oy = 0;

    int flag = 1;
    int point = 0;
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    // Select different fonts to draw on screen using the print class
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    header("Your Point", TFT_NAVY, point);
    
    while(1){
        tft.fillRect(0,30,320,210,TFT_BLACK);
        if (flag){
            flag = 0;
            ox = random(0, 300);
            oy = 45;
        }else{
            oy = oy + 10;
            if (ox < xpos + 10 && ox > xpos - 10){
                if (oy < ypos + 10 && oy > ypos - 10){
                    flag = 1;
                    point += 1;
                    header("Your Point", TFT_NAVY, point);
                }
            }
            if (oy > 240){
                flag = 1;
            }
        }
        if(!flag){
            tft.setTextColor(TFT_RED);
            tft.setCursor(ox, oy);
            tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:
            tft.print("o");
        }

        
        
        tft.setTextColor(TFT_YELLOW);
        if (digitalRead(WIO_KEY_C) == LOW && xpos > 0){
            Serial.println("C Key pressed");
            xpos = xpos - 10;
        }else if (digitalRead(WIO_KEY_A) == LOW && xpos < 300){
            Serial.println("A Key pressed");
            xpos = xpos + 10;
        }
        tft.setCursor(xpos, ypos);    // Set cursor near top left corner of screen
        tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:
        tft.print("P");
        
        delay(10);
    }
}

// Print the header for a display screen
void header(const char* string, uint16_t color, int point) {
    tft.fillScreen(color);
    tft.setTextSize(1);
    tft.setTextColor(TFT_MAGENTA, TFT_BLUE);
    tft.fillRect(0, 0, 320, 30, TFT_BLUE);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(string, 160, 2, 4); // Font 4 for fast drawing with background
    tft.drawString(String(point), 240, 2, 4);
}

// Draw a + mark centred on x,y
void drawDatumMarker(int x, int y) {
    tft.drawLine(x - 5, y, x + 5, y, TFT_GREEN);
    tft.drawLine(x, y - 5, x, y + 5, TFT_GREEN);
}


// There follows a crude way of flagging that this example sketch needs fonts which
// have not been enbabled in the User_Setup.h file inside the TFT_HX8357 library.
//
// These lines produce errors during compile time if settings in User_Setup are not correct
//
// The error will be "does not name a type" but ignore this and read the text between ''
// it will indicate which font or feature needs to be enabled
//
// Either delete all the following lines if you do not want warnings, or change the lines
// to suit your sketch modifications.

#ifndef LOAD_GLCD
    //ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_FONT2
    //ERROR_Please_enable_LOAD_FONT2_in_User_Setup!
#endif

#ifndef LOAD_FONT4
    //ERROR_Please_enable_LOAD_FONT4_in_User_Setup!
#endif

#ifndef LOAD_FONT6
    //ERROR_Please_enable_LOAD_FONT6_in_User_Setup!
#endif

#ifndef LOAD_FONT7
    //ERROR_Please_enable_LOAD_FONT7_in_User_Setup!
#endif

#ifndef LOAD_FONT8
    //ERROR_Please_enable_LOAD_FONT8_in_User_Setup!
#endif

#ifndef LOAD_GFXFF
    ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif
