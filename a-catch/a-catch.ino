#include <rpcWiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include "env.h"
#include "Free_Fonts.h"
#include "SPI.h"
#include "TFT_eSPI.h"

TFT_eSPI tft = TFT_eSPI();
char json_string[255];
WiFiClientSecure client;
int game_state = 0;
int highscore = 0;
int point = 0;
int regterm = 0;
String token;
File myFile;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting..");
  }
  Serial.print("Connected to the WiFi network with IP: ");
  Serial.println(WiFi.localIP());
  client.setCACert(root_ca);
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  tft.begin();
  tft.setRotation(3);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("initialization failed!");
    while (1);
  }
  read_token();
  Serial.println("initialization done.");
}

void read_token(){
  myFile = SD.open("token.txt", FILE_READ);
  if (myFile) {
    Serial.println("token.txt: ");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.println(myFile.read());
      token = String(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    Serial.println("error opening token.txt");
    regterm = 1;
  }
}

int send_score(int score) {
  HTTPClient https;
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> json_array;
  json_array["token"] = token;
  json_array["score"] = score;
  serializeJson(json_array, json_string, sizeof(json_string));
  Serial.print("[HTTPS] begin...\n");
  https.addHeader("Content-Type", "application/json");
  if (https.begin(client, "https://cl2chino.com:8082/wio/score?format=json")) {  // HTTPS
    Serial.print("[HTTPS] POST...\n");
    int httpCode = https.POST(json_string);
    if (httpCode > 0) {
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        Stream* resp = https.getStreamPtr();
        DynamicJsonDocument json_response(255);
        deserializeJson(json_response, *resp);
        int score = json_response["score"];
        return score;
      }
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  return -1;
}

int reg_term () {
  HTTPClient https;
  Serial.print("[HTTPS] begin...\n");
  if (https.begin(client, "https://cl2chino.com:8082/wio/reg?format=json")) {  // HTTPS
    int httpCode = https.GET();
    if (httpCode > 0) {
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        Stream* resp = https.getStreamPtr();
        DynamicJsonDocument json_response(255);
        deserializeJson(json_response, *resp);
        myFile = SD.open("token.txt", FILE_WRITE);
        if (myFile) {
          myFile.println(json_response["token"]);
          myFile.close();
          return 0;
        } else {
          Serial.println("error opening token.txt");
        }
      }
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  return -1;
}

void loop() {
  int m_height = 220;
  int m_width = 320;
  int xpos =  0;
  int ypos = 215;
  int ox = 0;
  int oy = 0;
  int flag = 1;

  switch (game_state) {
    case 0:
      tft.fillRect(0, 30, 320, 210, TFT_BLACK);
      header("Getting your HighScore ...", TFT_NAVY, -1);
      while (1) {
        if (&client) {
          if (regterm == 1){
            highscore = reg_term();
            read_token();
          }else{
            highscore = send_score(point);
          }
          point = 0;
          if (highscore > -1)
            game_state = 1;
          break;
        } else {
          Serial.println("Unable to create client");
        }
        delay(10000);
      }
    case 1:
      header("Your HighScore is ", TFT_NAVY, highscore);
      tft.fillRect(0, 30, 320, 210, TFT_BLACK);
      tft.setCursor(m_width / 8, m_height / 2);
      tft.setFreeFont(FSB9);
      tft.print("Press any button to start game !");
      while (1) {
        delay(10);
        if (digitalRead(WIO_KEY_A) == LOW  || digitalRead(WIO_KEY_B) == LOW  || digitalRead(WIO_KEY_C) == LOW ) {
          game_state = 2;
          break;
        }
      }
    case 2:
      header("Your Point", TFT_NAVY, point);
      for (int i = 0; i < 600; i++) {
        tft.fillRect(0, 30, 320, 210, TFT_BLACK);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(0, 235);
        tft.print(String(60 - i / 10));
        if (flag) {
          flag = 0;
          ox = random(0, 300);
          oy = 45;
        } else {
          oy = oy + 10;
          if (ox < xpos + 10 && ox > xpos - 10) {
            if (oy < ypos + 10 && oy > ypos - 10) {
              flag = 1;
              point += 1;
              header("Your Point", TFT_NAVY, point);
            }
          }
          if (oy > 240) {
            flag = 1;
          }
        }
        if (!flag) {
          tft.setTextColor(TFT_RED);
          tft.setCursor(ox, oy);
          tft.setFreeFont(FSB9);
          tft.print("o");
        }
        tft.setTextColor(TFT_YELLOW);
        if (digitalRead(WIO_KEY_C) == LOW && xpos > 0) {
          xpos = xpos - 10;
        } else if (digitalRead(WIO_KEY_A) == LOW && xpos < 300) {
          xpos = xpos + 10;
        }
        tft.setCursor(xpos, ypos);
        tft.setFreeFont(FSB9);
        tft.print("P");
        delay(10);
      }
      tft.fillRect(0, 30, 320, 210, TFT_BLACK);
      tft.setCursor(m_width / 3, m_height / 2);
      tft.print("Game End !");
      game_state = 0;
      delay(3000);
  }
}

// Print the header for a display screen
void header(const char* string, uint16_t color, int point) {
  tft.fillScreen(color);
  tft.setTextSize(1);
  tft.setTextColor(TFT_MAGENTA, TFT_BLUE);
  tft.fillRect(0, 0, 320, 30, TFT_BLUE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(string, 160, 2, 4);
  if (point != -1) {
    tft.drawString(String(point), 280, 2, 4);
  }
}

// Draw a + mark centred on x,y
void drawDatumMarker(int x, int y) {
  tft.drawLine(x - 5, y, x + 5, y, TFT_GREEN);
  tft.drawLine(x, y - 5, x, y + 5, TFT_GREEN);
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
