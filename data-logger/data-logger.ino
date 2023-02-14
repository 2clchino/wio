#include "SoftWire.h"
#include "AsyncDelay.h"
#include "TimeCtl.h"
#include <WebServer.h>
#include <RPCmDNS.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#define MAX_CH 7
#define INA228 0x40
const String FILE_LIST = "file_list.txt";

int ina_scl[MAX_CH] = {BCM14, BCM18, BCM24, BCM7, BCM12, BCM20, BCM6};
int ina_sda[MAX_CH] = {BCM15, BCM23, BCM25, BCM1, BCM16, BCM21, BCM19};

SoftWire* ina_i2c[MAX_CH];
char swTxBuffer[MAX_CH][16];
char swRxBuffer[MAX_CH][16];
WebServer server(80);
int now_time;
String now_date;
float current_val[MAX_CH] = {0};
String flag_write = "";
File myFile;

void handleRoot() {
    server.send(200, "text/plain", "hello from Wio Terminal!");
}

void handleNotFound() {
    if (server.method() == HTTP_OPTIONS)
    {
        server.sendHeader("Access-Control-Max-Age", "10000");
        server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "*");
        server.send(204);
    }
    else{
        server.send(404, "text/plain", "page not found");
    }
}

void server_setup() {
    if (!MDNS.begin("data-logger")) {
        Serial.println("Error setting up MDNS responder!");
            while(1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");
    server.enableCORS(true);
    server.on("/", handleRoot);
    server.on("/get-current-val", HTTP_GET, []() {
        String data = "";
        myFile = SD.open("current.txt", FILE_READ);
        if (myFile) {
            while (myFile.available()) {
                data = myFile.readString();
            }
            myFile.close();
        }
        server.send(200, "text/plain", data);
    });
    server.on("/get-current-log", HTTP_POST, []() { // receive file name formatted 2006-01-02T03.txt
        String json_txt = server.arg("plain");
        Serial.println(json_txt);
        DynamicJsonDocument recv_body(256);
        DeserializationError error = deserializeJson(recv_body, json_txt);
        String file = recv_body["file"];
        Serial.println(file);
        String data = "";
        myFile = SD.open("current.txt", FILE_READ);
        if (myFile) {
            while (myFile.available()) {
                data = myFile.readString();
            }
            myFile.close();
        }
        Serial.println(data);
        flag_write = file;
        server.send(200, "text/plain", data);
    });
    server.onNotFound(handleNotFound);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("HTTP server started");
    Serial.println("mDNS responder started");
}

void sd_setup() {
    Serial.print("Initializing SD card...");
    if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
        Serial.println("initialization failed!");
        while (1);
    }
    Serial.println("Success");
}

void setup() {
    Serial.begin(9600);
    rtc_setup(60 * 1000);
    sd_setup();
    for (int i=0; i<MAX_CH; i++) {
        ina_i2c[i] = new SoftWire(ina_sda[i], ina_scl[i]);
        ina_i2c[i]->setTxBuffer(swTxBuffer[i], sizeof(swTxBuffer[i]));
        ina_i2c[i]->setRxBuffer(swRxBuffer[i], sizeof(swRxBuffer[i]));
        ina_i2c[i]->setDelay_us(5);
        ina_i2c[i]->setTimeout(1000);
        ina_i2c[i]->setClock(1*1000);
        ina_i2c[i]->begin();
    }
    server_setup();
}

void move_file() {
    if (flag_write == "")
        return;
    String data = "";
    myFile = SD.open("current.txt", FILE_READ);
    if (myFile) {
        while (myFile.available()) {
            data = myFile.readString();
        }
        myFile.close();
    }
    myFile = SD.open(flag_write, FILE_WRITE);
    if (myFile) {
        myFile.print(data);
        myFile.close();
    }
    myFile = SD.open("current.txt", FILE_WRITE);
    if (myFile) {
        myFile.close();
    }
    flag_write = "";
    Serial.println("logs are moved from current.txt");
}

void save_data(){
    if (now_time == now.hour() * 100 + now.minute())   // already triggered
        return;
    move_file();
    now_time = now.hour() * 100 + now.minute();
    String file_name = "current.txt";
    myFile = SD.open(file_name, FILE_APPEND);
    if (myFile) {
        float *current = &current_val[0];
        myFile.print(now_time);
        myFile.print(",");
        for (int i = 0; i < MAX_CH; i++) {
            myFile.print(String(current[i], 2));
            if (i < MAX_CH - 1) {
                myFile.print(",");
            }
        }
        myFile.print("|");
        myFile.close();
    } else {
        Serial.println("error opening " + file_name);
    }
}

void read_ch() {
    char buf[100];
    float *current = &current_val[0];
    for (int i=0; i<MAX_CH; i++) {   
        sprintf(buf, "ch %d :", i);
        ina_i2c[i]->beginTransmission(INA228);
        ina_i2c[i]->write(0x3f); // Device ID
        ina_i2c[i]->endTransmission(false);
        
        int recv, recv2, recv3;
        recv = recv2 = 0;
        ina_i2c[i]->requestFrom(INA228, 2);
        if(ina_i2c[i]->available()){
            recv  = ina_i2c[i]->read();
            if(ina_i2c[i]->available()){
                recv2 = ina_i2c[i]->read();
                //Serial.print((recv <<8 | recv2), HEX);
                Serial.print(recv, HEX);
                Serial.print(" ");
                Serial.print(recv2, HEX);
                Serial.print(" / ");
            }
        }
    
        ina_i2c[i]->beginTransmission(INA228);
        ina_i2c[i]->write(0x05); // Voltage
        ina_i2c[i]->endTransmission(false);
        
        recv = recv2 = recv3 = 0;
        int got = ina_i2c[i]->requestFrom(INA228, 3);
        // sprintf(buf, "Got %d bytes: ", got);
        // Serial.print(buf);
      
        if(ina_i2c[i]->available()){
            recv  = ina_i2c[i]->read();
            recv2 = ina_i2c[i]->read();
            recv3 = ina_i2c[i]->read();
      
            sprintf(buf, "[%x/%x/%x]", recv, recv2, recv3);
            // Serial.print(buf);
        }
        Serial.print(" ");
        // TODO ADD: determine if the value is valid
        current[i] = float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125);
        // Serial.print(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125);
        // Serial.println("");
    }
    // Serial.println("");
}

void loop() {
    rtc_update();
    now = rtc.now();
    server.handleClient();
    read_ch();
    save_data();
    delay(1000);
}
