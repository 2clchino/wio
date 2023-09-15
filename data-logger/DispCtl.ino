TFT_eSPI tft = TFT_eSPI();
const int WIDTH = 320;
const int HEIGHT = 240;

void SetupDisplay() {
    pinMode(WIO_KEY_A, INPUT_PULLUP);
    pinMode(WIO_KEY_B, INPUT_PULLUP);
    pinMode(WIO_KEY_C, INPUT_PULLUP);
    tft.begin();
    tft.setRotation(3);
}

void UpdateDisp(String *now_time) {
    DispFill();
    CurrentVal();
    ShowTime(+now_time);
    if (wio_name == "" || wio_name == "Wio 0")
        get_name(0);
    WioStatus(&wio_name);
}

void WioStatus(String *state){
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FSB9);
    tft.setCursor(10, 50);
    tft.print(*state);
}

void CurrentVal() {
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FSB9);
    float* current = &current_val[0];
    int temp = 0;
    for (int i = 0; i < CUR_CH; i++) {
        tft.setCursor(10, 25 * (temp + 5));
        tft.print("CH");
        tft.print(temp + 1);
        tft.print(": ");
        if (i < VOL_CH) {  // Show only odd-indexed values for i = VOL_CH
            if (i % 2 == 1) {
                int pairIndex = i - 1;  // Calculate the index of the even number in the pair
                String val;
                if (abs(current[pairIndex]) > abs(current[i])) {
                    val = String(current[pairIndex], 2);
                } else {
                    val = String(current[i], 2);
                }
                tft.print(val);
                temp++;
            }
        } else {
            tft.print(String(current[i], 2));
            temp++;
        }
    }
}

void DispFill() {
    tft.fillScreen(TFT_BLACK);
}

void ShowTime(String *now_time) {
    tft.setCursor(10, 25);
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FSB9);
    tft.print(*now_time);
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
