#include "Free_Fonts.h"
#include "TFT_eSPI.h"

TFT_eSPI tft = TFT_eSPI();
const int SW_COLUMN = 4;
const int WIDTH = 320;
const int HEIGHT = 120;
const int Y_OFFSET = 120;

#define MAX_CH 7
int relay[MAX_CH] = {BCM4, BCM17, BCM27, BCM22, BCM10, BCM9, BCM11};
int rstat[MAX_CH] = {BCM0, BCM5, BCM6, BCM13, BCM19, BCM26, BCM21};
int onoff[MAX_CH] = {0};

void SetupDisplay(){
    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    for (int ch=0; ch<MAX_CH; ch++){
        pinMode(relay[ch], OUTPUT);
        pinMode(rstat[ch], INPUT);
    }
    tft.begin();
    tft.setRotation(3);
}
/*
void setup() {
    Serial.begin(115200);
    while (!Serial) {
    }
    SetupDisplay();
}
*/

void ChangeBin(int dec, int *bin)
{
    for (int i = 0; i < MAX_CH; i++) {
        bin[i] = dec % 2;
        dec = dec / 2;
    }
}

void toggle_relay(){
    for (int ch=0; ch<MAX_CH; ch++)
        digitalWrite(relay[ch], onoff[ch]);
}

void ShowPompState(int state){
    int SW_ROW = (MAX_CH + SW_COLUMN - 1) / SW_COLUMN;
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FSB9);
    ChangeBin(state, &onoff[0]);
    for (int i = 0; i < SW_ROW; i++){
        for (int j = 0; j < SW_COLUMN; j++){
            if (((j+1) * (i+1)) > MAX_CH)
                return;
            int ch = i * SW_COLUMN + j;
            int m = digitalRead(rstat[ch]);
            char mode[10];
            
            int x_area = WIDTH / SW_COLUMN;
            int y_area = HEIGHT / SW_ROW;
            int x = x_area * j + x_area / 2;
            int y = (y_area + 5) * i + y_area / 2 + Y_OFFSET;
            tft.setTextColor(TFT_WHITE);
            if(m==0){
                strcpy(mode, "Manual");
                tft.setCursor(x-28, y-5);
            } else {
                strcpy(mode, "Auto");
                tft.setCursor(x, y);
                tft.fillCircle(x,y,7,TFT_WHITE);
                tft.fillCircle(x,y,5,TFT_BLACK);
                if (onoff[ch])
                    tft.fillCircle(x,y,3,TFT_WHITE);
                tft.setCursor(x-18, y-15);
            }
            tft.print(mode);
            tft.setCursor(x-15, y-35);
            tft.print("ch");
            tft.setCursor(x+10, y-35);
            tft.print(ch + 1);
        }
    }
    toggle_relay();
}
/*
void loop() {
    ShowPompState(sw_state);
    delay(100);
}
*/

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
