#include "TimeCtl.h"
#include <WebServer.h>
 
WebServer server(80);

void handleRoot() {
    server.send(200, "text/plain", "hello from Wio Terminal!");
}
 
void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}
int set_alarm(const char *str){
    int Year, Month, Day, Hour, Minute, Second;
    int res = sscanf(str, "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);
    if (res != 6){
        Serial.println("Fail to book");
        return -1;
    }
    DateTime alarm = DateTime(Year, Month, Day, Hour, Minute, Second);
    rtc.setAlarm(0,alarm); // match after 15 seconds
    rtc.enableAlarm(0, rtc.MATCH_HHMMSS); // match Every Day
    rtc.attachInterrupt(alarmMatch); // callback whlie alarm is match
    return 0;
}

void alarmMatch(uint32_t flag){
    Serial.println("Wake UP!!!!");
}

void setup() {
    rtc_setup(60 * 1000);
    server.on("/", handleRoot);
    server.on("/alarm", []() {
        String str = server.arg("datetime");
        if (set_alarm(str.c_str()) == 0){
            server.send(200, "text/plain", "Success to register alarm !!");
        } else {
            server.send(500, "text/plain", "Format is wrong !!");
        }
    });
   
    server.onNotFound(handleNotFound);
   
    server.begin();
    Serial.println("HTTP server started");
}
 
void loop() {
    rtc_update();
    server.handleClient();
}
