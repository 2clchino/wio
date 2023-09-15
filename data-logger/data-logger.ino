#include "SoftWire.h"
#include "AsyncDelay.h"
#include <WebServer.h>
#include <RPCmDNS.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include "DispCtl.h"
#include <Adafruit_MAX31855.h>
#define INA228 0x40
const String FILE_LIST = "file_list.txt";
int ina_scl[MAX_CH] = {BCM14, BCM18, BCM24, BCM7, BCM12, BCM20, BCM6};
int ina_sda[MAX_CH] = {BCM15, BCM23, BCM25, BCM1, BCM16, BCM21, BCM19};

#define MAXDO   PIN_SPI_MISO
#define MAXCS   PIN_SPI_SS
#define MAXCLK  PIN_SPI_SCK
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

SoftWire* ina_i2c[MAX_CH];
char swTxBuffer[MAX_CH][16];
char swRxBuffer[MAX_CH][16];
WebServer server(80);
int now_time;
int now_hour;
String now_date;
String time_str = "";
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
    String serviceName = "data-logger-" + String(wio_id);
    Serial.println(serviceName);
    if (!MDNS.begin(serviceName.c_str())) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");
    server.enableCORS(true);
    server.on("/", handleRoot);
    server.on("/get-current-val", HTTP_POST, []() {String json_txt = server.arg("plain");
        Serial.println(json_txt);
        DynamicJsonDocument recv_body(256);
        DeserializationError error = deserializeJson(recv_body, json_txt);
        unsigned long utdate = recv_body["utdate"];
        if (devicetime == 0) {
            devicetime = utdate;
        }
        String data = "";
        myFile = SD.open("current.txt", FILE_READ);
        if (myFile) {
            while (myFile.available()) {
                data = myFile.readString();
            }
            myFile.close();
        }
        server.send(200, "text/plain", data);
        move_file();
    });
    server.on("/get-name", HTTP_GET, handleGetName);
    server.on("/set-name", HTTP_POST, handleSetName);
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
    Serial.print("Initializing sensor...");
    if (!thermocouple.begin()) {
       Serial.println("ERROR.");
       while (1) delay(10);
    }
    Serial.println("DONE.");
    SetupDisplay();
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

String formatDate(DateTime dt) {
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02dT%02d", dt.year(), dt.month(), dt.day(), dt.hour());
    return String(buffer);
}

void move_file() {
    if (now_hour == now.hour())
        return;
    now_hour = now.hour();
    String formattedTime = formatDate(now);
    String data = "";
    myFile = SD.open("current.txt", FILE_READ);
    if (myFile) {
        while (myFile.available()) {
            data = myFile.readString();
        }
        myFile.close();
    }
    myFile = SD.open(formattedTime + ".txt", FILE_APPEND);
    if (myFile) {
        myFile.print(data);
        myFile.close();
    }
    myFile = SD.open("current.txt", FILE_WRITE);
    if (myFile) {
        myFile.close();
    }
    now_hour = now.hour();
    Serial.println("logs are moved from current.txt");
}

void save_data(){
    if (now_time == now.hour() * 100 + now.minute())   // already triggered
        return;
    now_time = now.hour() * 100 + now.minute();
    String file_name = "current.txt";
    myFile = SD.open(file_name, FILE_APPEND);
    if (myFile) {
        float *current = &current_val[0];
        char time_str[5];
        sprintf(time_str, "%04d", now_time);
        myFile.print(time_str);
        myFile.print(",");
        for (int i = 0; i < CUR_CH; i++) {
            if (i < VOL_CH) {  // Save only odd-indexed values for i = VOL_CH
                if (i % 2 == 1) {
                    int pairIndex = i - 1;  // Calculate the index of the even number in the pair
                    if (abs(current[pairIndex]) > abs(current[i])) {
                        myFile.print(String(current[pairIndex], 2));
                    } else {
                        myFile.print(String(current[i], 2));
                    }
                    if (i < CUR_CH - 1) {
                        myFile.print(",");
                    }
                }
            } else {
                myFile.print(String(current[i], 2));
                if (i < CUR_CH - 1) {
                    myFile.print(",");
                }
            }
        }
        myFile.print("|");
        myFile.close();
    } else {
        Serial.println("error opening " + file_name);
    }
}

void setMode(uint8_t mode) {
    uint8_t adc_config_value;
    
    for (int i = 0; i < MAX_CH; i++) {
        ina_i2c[i]->beginTransmission(INA228);
        ina_i2c[i]->write(0x01);
        ina_i2c[i]->endTransmission(false);
        
        ina_i2c[i]->requestFrom(INA228, 1);
        if (ina_i2c[i]->available()) {
            adc_config_value = ina_i2c[i]->read();
        }
    }
    
    adc_config_value &= 0x0F;
    adc_config_value |= (mode << 4);
    
    for (int i = 0; i < MAX_CH; i++) {
        ina_i2c[i]->beginTransmission(INA228);
        ina_i2c[i]->write(0x01);
        ina_i2c[i]->write(adc_config_value);
        ina_i2c[i]->endTransmission();
    }
}

float readCurrent(int deviceAddress) {
    setMode(0x1);
    char buf[100];
    ina_i2c[MAX_CH-1]->beginTransmission(deviceAddress);
    ina_i2c[MAX_CH-1]->write(0x07); // Current register
    ina_i2c[MAX_CH-1]->endTransmission(deviceAddress);
    
    int recv, recv2, recv3;
    recv = recv2 = recv3 = 0;
    int got = ina_i2c[MAX_CH-1]->requestFrom(deviceAddress, 3);
    // sprintf(buf, "Got %d bytes: ", got);
    // Serial.println(buf);
    if(ina_i2c[MAX_CH-1]->available()){
        recv  = ina_i2c[MAX_CH-1]->read();
        recv2 = ina_i2c[MAX_CH-1]->read();
        recv3 = ina_i2c[MAX_CH-1]->read();
        sprintf(buf, "[%x/%x/%x]", recv, recv2, recv3);
        Serial.println(buf);
    }
    
    // Convert the raw data to current value
    float current = 0;
    if (recv != 255 && recv2 != 255 && recv3 != 255 && got > 0) {
        current = float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.15625);
    } else {
        current = -float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.15625);
    }
    return current;
}

void read_ch() {
    setMode(0x1);
    char buf[100];
    float *current = &current_val[0];
    for (int i=0; i<MAX_CH - 1; i++) {   
        sprintf(buf, "ch %d :", i);
        Serial.print(buf);
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
      
            //sprintf(buf, "[%x/%x/%x]", recv, recv2, recv3);
            //Serial.println(buf);
        }
        if (!(recv == 0 && recv2 == 255 && recv3 == 255)) {
            current[i] = i % 2 == 0 ? float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125) : -float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125);
        } else {
            current[i] = 0;
        }  
        Serial.print(current[i]);
        Serial.print(", ");
    }
    // read current
    float val = readCurrent(INA228);
    current[MAX_CH-1] = val > 0 ? val : current[MAX_CH-1];
    Serial.print(current[MAX_CH-1]);
    Serial.print(", ");

    // read thermo
    double internal = thermocouple.readInternal();
    double celcius  = thermocouple.readCelsius();
    /*Serial.print(internal);
    Serial.print(" ");
    Serial.println(celcius);*/
    current[MAX_CH] = isnan(celcius) ? (float)internal : (float)celcius;
    Serial.println(current[MAX_CH]);
}

void loop() {
    rtc_update();
    now = rtc.now();
    time_str = now.timestamp(DateTime::TIMESTAMP_DATE) + "  " + now.timestamp(DateTime::TIMESTAMP_TIME);
    UpdateDisp(&time_str);
    server.handleClient();
    read_ch();
    save_data();
    delay(1000);
}
