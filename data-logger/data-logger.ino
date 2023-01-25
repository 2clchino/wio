#include "SoftWire.h"
#include "AsyncDelay.h"
#include "TimeCtl.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <RPCmDNS.h>

#define INA228 0x40

int ina_scl[7] = {BCM14, BCM18, BCM24, BCM7, BCM12, BCM20, BCM6};
int ina_sda[7] = {BCM15, BCM23, BCM25, BCM1, BCM16, BCM21, BCM19};

SoftWire* ina_i2c[7];
char swTxBuffer[7][16];
char swRxBuffer[7][16];
WebServer server(80);

void setup() {
    Serial.begin(9600);
    rtc_setup(60 * 1000);
    
    for (int i=0; i<7; i++) {
        ina_i2c[i] = new SoftWire(ina_sda[i], ina_scl[i]);
        ina_i2c[i]->setTxBuffer(swTxBuffer[i], sizeof(swTxBuffer[i]));
        ina_i2c[i]->setRxBuffer(swRxBuffer[i], sizeof(swRxBuffer[i]));
        ina_i2c[i]->setDelay_us(5);
        ina_i2c[i]->setTimeout(1000);
        ina_i2c[i]->setClock(1*1000);
        ina_i2c[i]->begin();
    }

    if (!MDNS.begin("data-logger")) {
        Serial.println("Error setting up MDNS responder!");
            while(1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");
    server.enableCORS(true);
    server.on("/", handleRoot);
    server.on("/get-data", HTTP_GET, []() {
        server.send(200, "text/plain", "Hello World");
    });
    server.onNotFound(handleNotFound);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("HTTP server started");
    Serial.println("mDNS responder started");
}

void loop() {
    char buf[100];
    rtc_update();
    now = rtc.now();
    // put your main code here, to run repeatedly:
  
    for (int i=0; i<7; i++) {
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
        Serial.print(buf);
      
        if(ina_i2c[i]->available()){
            recv  = ina_i2c[i]->read();
            recv2 = ina_i2c[i]->read();
            recv3 = ina_i2c[i]->read();
      
            sprintf(buf, "[%x/%x/%x]", recv, recv2, recv3);
            Serial.print(buf);
        }
        Serial.print(" ");
        Serial.print(((recv<<16 | recv2<<8 | recv3)>>4) * 0.0001953125);
        
        Serial.println("");
    }
    Serial.println("");
    delay(500);
}

void handleRoot() {
    server.send(200, "text/plain", "hello from Wio Terminal!");
}

void handleNotFound() {
    if (server.method() == HTTP_OPTIONS)
    {
        server.sendHeader("Access-Control-Max-Age", "10000");
        server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS"
);
        server.sendHeader("Access-Control-Allow-Headers", "*");
        server.send(204);
    }
    else{
        server.send(404, "text/plain", "page not found");
    }
}
