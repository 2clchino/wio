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

int send_score(int score) {
  HTTPClient https;
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> json_array;
  json_array["Name"] = "Player1";
  json_array["Pass"] = score;
  serializeJson(json_array, json_string, sizeof(json_string));
  Serial.print("[HTTPS] begin...\n");
  https.addHeader("Content-Type", "application/json");
  if (https.begin(client, "https://cl2chino.com:8082/score?format=json")) {  // HTTPS
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

void loop() {
  if (&client) {
    {
      Serial.println(send_score(0));
    }
  } else {
    Serial.println("Unable to create client");
  }
  Serial.println();
  Serial.println("Waiting 10s before the next round...");
  delay(10000);
}
