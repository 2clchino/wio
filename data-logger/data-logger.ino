#include "SoftWire.h"
#include "AsyncDelay.h"
#include <WebServer.h>
#include <RPCmDNS.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"
#include "DispCtl.h"
#include <Adafruit_MAX31855.h>
#define INA226 0x40
const String CH_NAMES = "ch_names.txt";
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
int last_moved;
String now_date;
String time_str = "";
String flag_write = "";
const char* ch_names;
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
    server.on("/get-current-val", HTTP_GET, []() {
        String data = "";
        myFile = SD.open("current.txt", FILE_READ);
        if (myFile) {
            while (myFile.available()) {
                data = myFile.readString();
            }
            myFile.close();
        }
        move_file();
        server.send(200, "text/plain", data);
    });
    server.on("/get-name", HTTP_GET, handleGetName);
    server.on("/set-name", HTTP_POST, handleSetName);
    server.on("/set-chname", HTTP_POST, []() {
        String json_txt = server.arg("plain");
        Serial.println(json_txt);
        DynamicJsonDocument recv_body(256);
        DeserializationError error = deserializeJson(recv_body, json_txt);
        if (ch_names != recv_body["ch_names"]) {
            ch_names = recv_body["ch_names"];
            setChName();
        }
        String data = "";
        myFile = SD.open("current.txt", FILE_READ);
        if (myFile) {
            while (myFile.available()) {
                data = myFile.readString();
            }
            myFile.close();
        }
        move_file();
        server.send(200, "text/plain", data);
    });
    server.onNotFound(handleNotFound);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("HTTP server started");
    Serial.println("mDNS responder started");
}

void setChName() {
    File dataFile = SD.open(CH_NAMES, FILE_WRITE);
    if (dataFile) {
        dataFile.println(ch_names);
        dataFile.close();
        Serial.println("Name saved successfully.");
        readChannelNames(&channelNames[0]);
    } else {
        Serial.println("Error opening name.txt for writing.");
    }
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
    String state = get_name(0);
    processWioData(state);
    last_moved = now.hour() * 100 + now.minute();
    readChannelNames(&channelNames[0]);
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
    sprintf(buffer, "%04d-%02d-%02d", dt.year(), dt.month(), dt.day());
    return String(buffer);
}

void move_file() {
    myFile = SD.open("current.txt", FILE_WRITE);
    if (myFile) {
        myFile.close();
    }
    last_moved = now_time;
    Serial.println("logs are moved from current.txt");
}

void writeDataToFile(const String& fileName, const String& data) {
    File file = SD.open(fileName, FILE_APPEND);
    if (!file) {
        file = SD.open(fileName, FILE_WRITE);
        
        if (!file) {
            Serial.println("error creating " + fileName);
            return;
        }
    }
    file.print(data);
    file.close();
}

void save_data() {
    if (now_time == now.hour() * 100 + now.minute())   // already triggered
        return;
    Serial.println("Saved val");
    now_time = now.hour() * 100 + now.minute();
    if (now_time > last_moved + 100) {                 // API is not called
        move_file();
    }
    char time_str[5];
    sprintf(time_str, "%04d", now_time);
    String data = time_str;
    data += ",";
    
    float *current = &current_val[0];
    for (int i = 0; i < CUR_CH; i++) {
        data += String(current[i], 2);
        if (i < CUR_CH - 1)
            data += ",";
    }
    data += "|";
    
    String file_name = "current.txt";
    writeDataToFile(file_name, data);
    String formatted = formatDate(now) + "log.txt";
    writeDataToFile(formatted, data);
}

void read_ch() {
    char buf[100];
    float *current = &current_val[0];
    for (int i=0; i<MAX_CH - 1; i++) {   
        sprintf(buf, "ch %d :", i);
        Serial.print(buf);
        ina_i2c[i]->beginTransmission(INA226);
        ina_i2c[i]->write(0xff);
        ina_i2c[i]->endTransmission(false);
        int recv, recv2, recv3;
        recv = recv2 = 0;
        ina_i2c[i]->requestFrom(INA226, 2);
        if(ina_i2c[i]->available()){
            recv  = ina_i2c[i]->read();
            if(ina_i2c[i]->available()){
                recv2 = ina_i2c[i]->read();
                Serial.print((recv <<8 | recv2), HEX);
                Serial.print(recv, HEX);
                Serial.print(" ");
                Serial.print(recv2, HEX);
                Serial.print(" / ");
            }
        }
    
        ina_i2c[i]->beginTransmission(INA226);
        ina_i2c[i]->write(0x02); // Voltage
        ina_i2c[i]->endTransmission(false);
        
        recv = recv2 = recv3 = 0;
        int got = ina_i2c[i]->requestFrom(INA226, 3);
        if(ina_i2c[i]->available()){
            recv  = ina_i2c[i]->read();
            recv2 = ina_i2c[i]->read();
            recv3 = ina_i2c[i]->read();
        }
        current[i] = float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.000125);
        Serial.print(current[i]);
        Serial.print(", ");
    }
    current[MAX_CH - 1] = readCurrent();
    // read thermo
    double internal = thermocouple.readInternal();
    double celcius  = thermocouple.readCelsius();
    current[MAX_CH] = isnan(celcius) ? (float)internal : (float)celcius;
    Serial.println(current[MAX_CH]);
}

float readCurrent() {
    char buf[100];
    ina_i2c[MAX_CH-1]->beginTransmission(INA226);
    ina_i2c[MAX_CH-1]->write(0x01); // Current register
    ina_i2c[MAX_CH-1]->endTransmission(INA226);
    
    int recv, recv2, recv3;
    recv = recv2 = recv3 = 0;
    int got = ina_i2c[MAX_CH-1]->requestFrom(INA226, 3);
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
        current = float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.1);
    } else {
        current = -float(((recv<<16 | recv2<<8 | recv3)>>4) * 0.1);
    }
    return current;
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
