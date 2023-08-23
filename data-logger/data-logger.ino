#include "SoftWire.h"
#include "AsyncDelay.h"
#include "name_ctrl.h"
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
    });
    server.on("/get-name", HTTP_GET, handleGetName);
    server.on("/set-name", HTTP_POST, handleSetName);
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
        char time_str[5];
        sprintf(time_str, "%04d", now_time);
        myFile.print(time_str);
        myFile.print(",");
        for (int i = 0; i < CUR_CH; i++) {
            myFile.print(String(current[i], 2));
            if (i < CUR_CH - 1) {
                myFile.print(",");
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
    Wire.beginTransmission(deviceAddress);
    Wire.write(0x04); // Current register
    Wire.endTransmission(false);
    
    int recv, recv2, recv3;
    recv = recv2 = recv3 = 0;
    int got = Wire.requestFrom(deviceAddress, 3);
    
    if(Wire.available()){
        recv  = Wire.read();
        recv2 = Wire.read();
        recv3 = Wire.read();
    }
    
    // Convert the raw data to current value
    float current = float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.15625);
    
    return current;
}

void read_ch() {
    setMode(0x1);
    char buf[100];
    float *current = &current_val[0];
    for (int i=0; i<MAX_CH; i++) {   
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
                // Serial.print((recv <<8 | recv2), HEX);
                // Serial.print(recv, HEX);
                // Serial.print(" ");
                // Serial.print(recv2, HEX);
                // Serial.print(" / ");
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
        // Serial.print(" ");
        // TODO ADD: determine if the value is valid
        if (!(recv == 0 && recv2 == 255 && recv3 == 255)) {
            current[i] = i % 2 == 0 ? float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125) : -float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125);
        } else {
            current[i] = 0;
        }
        if (i == MAX_CH - 1) {
            current[i] = readCurrent(INA228);
        }
        // Serial.print(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125);
        Serial.println(current[i]);
    }
    double internal = thermocouple.readInternal();
    double celcius  = thermocouple.readCelsius();
    Serial.print(internal);
    Serial.print(" ");
    Serial.println(celcius);
    current[MAX_CH] = isnan(celcius) ? (float)internal : (float)celcius;
    // Serial.println("");
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
