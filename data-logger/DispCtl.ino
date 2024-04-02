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
    displayWioInfo();
}

// 表示処理を行う関数
void displayWioInfo() {
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FSB9);
    tft.setCursor(10, 45);
    tft.print("Name: ");
    tft.print(wio_name);
    tft.setCursor(10, 70);
    tft.print("Number: ");
    tft.println(wio_number);
    tft.setCursor(10, 95);
    tft.print("IP Address: ");
    tft.print(wio_ip);
}


void readChannelNames(String* channelNames) {
    for (int i = 0; i < CUR_CH; i++){
        channelNames[i] = "ch " + String(i + 1);
    }
    File file = SD.open("ch_names.txt", FILE_READ);
    if (!file) {
        return;
    }
    String fileContent = file.readString();
    file.close();
    splitString(fileContent);
}

void splitString(String input) {
  int commaIndex = 0;
  int lastIndex = 0;
  input.trim();
  for (int i = 0; i < CUR_CH; i++) {
    commaIndex = input.indexOf(',', lastIndex);
    if (commaIndex != -1) {
      channelNames[i] = input.substring(lastIndex, commaIndex);
      lastIndex = commaIndex + 1;
    } else {
      channelNames[i] = input.substring(lastIndex);
      break;
    }
  }
  for (int i = 0; i < CUR_CH; i++) {
    if (channelNames[i].length() == 0) {
      channelNames[i] = "ch " + String(i + 1);
    }
  }
}

void CurrentVal() {
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FSB9);
    String* names = &channelNames[0];
    float* current = &current_val[0];
    int temp = 0;
    for (int i = 0; i < CUR_CH; i++) {
        if (i % 2 == 1) {
            tft.setCursor(130, 20 + 25 * (temp + 5));
            temp++;
        } else {
            tft.setCursor(10, 20 + 25 * (temp + 5));
        }
        tft.print(names[i] + ": ");
        if (sizeof(names) / sizeof(names[0])) {
            
        }
        tft.print(String(current[i], 2));
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
