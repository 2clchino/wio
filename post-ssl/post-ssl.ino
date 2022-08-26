#include <rpcWiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "env.h"
char json_string[255];
WiFiClientSecure client;
 
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
}
 
void loop() {
  if(&client) {
    {
      HTTPClient https;
      StaticJsonDocument<JSON_OBJECT_SIZE(2)> json_array;
      json_array["Name"] = "Player1";
      json_array["Pass"] = "secret";
      serializeJson(json_array, json_string, sizeof(json_string));
      Serial.print("[HTTPS] begin...\n");
      https.addHeader("Content-Type", "application/json");  
      if (https.begin(client, "https://cl2chino.com:8082/user/reg?format=json")) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        int httpCode = https.POST(json_string);
        if (httpCode > 0) {
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
          }
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }
  } else {
    Serial.println("Unable to create client");
  }
  Serial.println();
  Serial.println("Waiting 10s before the next round...");
  delay(10000);
}
