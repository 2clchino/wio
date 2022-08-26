#include <rpcWiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "env.h"

char json_string[255];

void setup() {
 
  Serial.begin(115200);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(500);
    Serial.println("Connecting..");
  }
  Serial.print("Connected to the WiFi network with IP: ");
  Serial.println(WiFi.localIP());
}
 
void loop() {
    // wait for WiFi connection
    if((WiFi.status() == WL_CONNECTED)) {
        StaticJsonDocument<JSON_OBJECT_SIZE(2)> json_array;
        json_array["Name"] = "Player1";
        json_array["Pass"] = "secret";
        serializeJson(json_array, json_string, sizeof(json_string));
        HTTPClient http;
        Serial.print("[HTTP] begin...\n");
        // configure traged server and url
        http.addHeader("Content-Type", "application/json");  
        http.begin("http://192.168.10.114:8082/user/reg"); //HTTP
        Serial.print("[HTTP] POST...\n");
        // start connection and send HTTP header
        int httpCode = http.POST(json_string);
        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] POST... code: %d\n", httpCode);
            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    delay(5000);
}
