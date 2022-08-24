#include <rpcWiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "env.h"

void setup() {

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting..");
  }
  Serial.print("Connected to the WiFi network with IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://api.ipify.org/?format=json");
    // http.begin("http://54.249.151.239:8082/data?format=json");
    int httpCode = http.GET();
    Serial.println(httpCode);
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);
    }
    http.end();
  }
  delay(10000);
}
