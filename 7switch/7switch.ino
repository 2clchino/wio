#include "Free_Fonts.h"
#include "TFT_eSPI.h"

TFT_eSPI tft = TFT_eSPI();
const int SW_CNT = 7;
const int SW_COLUMN = 4;
const int WIDTH = 320;
const int HEIGHT = 120;
const int Y_OFFSET = 120;
int sw_state = 0;

void SetupDisplay(){
    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    tft.begin();
    tft.setRotation(3);
    sw_state = 10;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
    }
    SetupDisplay();
}

void ChangeBin(int dec, int *bin)
{
    for (int i = 0; i < SW_CNT; i++) {
        bin[i] = dec % 2;
        dec = dec / 2;
    }
}

void ShowPompState(int state){
    int SW_ROW = (SW_CNT + SW_COLUMN - 1) / SW_COLUMN;
    int bin[SW_CNT];
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FSB9);
    for (int i = 0; i < SW_ROW; i++){
        for (int j = 0; j < SW_COLUMN; j++){
            if (((j+1) * (i+1)) > SW_CNT)
                return;
            int x_area = WIDTH / SW_COLUMN;
            int y_area = HEIGHT / SW_ROW;
            int x = x_area * j + x_area / 2;
            int y = y_area * i + y_area / 2 + Y_OFFSET;
            tft.setCursor(x, y);
            ChangeBin(state, &bin[0]);
            tft.fillCircle(x,y,7,TFT_WHITE);
            if (!bin[i*SW_COLUMN + j])
              tft.fillCircle(x,y,5,TFT_BLACK);
            tft.setCursor(x-20, y-35);
            tft.print("pomp");
            tft.setCursor(x-5, y-15);
            tft.print(i*SW_COLUMN + j + 1);
        }
    }
}

void loop() {
    ShowPompState(sw_state);
    delay(100);
}

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
