#include "TimeCtl.h"
#include <WebServer.h>
#include <RPCmDNS.h>
#include <Seeed_FS.h>
#include "SD/Seeed_SD.h"

WebServer server(80);
Alarm *almptr;
int almcnt = 0;
char return_buf[100*256];
Alarm alarms[256];
int now_time;
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

void encode_json(){
    DynamicJsonDocument body(100*256);   // 32*3*256(24576) + 7*96(672) + a
    Pump *pumptr = &current_state[0];
    JsonArray pumps = body.createNestedArray("states");
    for (int i = 0; i < MAX_CH; i++){
        JsonObject pump = pumps.createNestedObject();
        pump["name"] = pumptr[i].pump_name;
        pump["state"] = pumptr[i].state;
    }
    almptr = &alarms[0];
    JsonArray alarms = body.createNestedArray("alarms");
    for (int i = 0; i < almcnt; i++){
        JsonArray alarm = alarms.createNestedArray();
        alarm.add(almptr[i].awake_time);
        alarm.add(almptr[i].week_day);
        alarm.add(almptr[i].state);
    }
    serializeJson(body, return_buf);
    Serial.println(return_buf);
}

void decode_state(const char *state_text){
    Serial.println(state_text);
    Pump *current = &current_state[0];
    StaticJsonDocument<7*96> pump;
    DeserializationError error = deserializeJson(pump, state_text);
    for (int i = 0; i < MAX_CH; i++){
        String tmp = pump[i];
        StaticJsonDocument<96> cur;
        deserializeJson(cur, tmp);
        String _name = cur["name"];
        current[i].state = digitalRead(rstat[i]) ? cur["state"] : 0;
        current[i].pump_name = _name;
    }
}

void decode_json(const char *json_txt, int from_sd_flag = 0){
    DynamicJsonDocument body(100*256);   // 32*3*256(24576) + 7*96(672) + a
    almptr = &alarms[0];
    DeserializationError error = deserializeJson(body, json_txt);
    String alarm_text = body["alarm"];
    almcnt = body["cnt"];
    if (!from_sd_flag) {  // when data from sd, don't call decode_state
        String state_text = body["state"];
        decode_state(state_text.c_str());
    }
    /* ------------------------------------
     *  Decode Pump Schedule
     ------------------------------------ */
    Serial.println(alarm_text);
    error = deserializeJson(body, alarm_text);
    for (int i = 0; i < almcnt; i++){
        Alarm _alm;
        _alm.awake_time = body[i][0];
        _alm.week_day = body[i][1];
        _alm.state = body[i][2];
        almptr[i] = _alm;
    }
}

void sd_setup() {
    Serial.print("Initializing SD card...");
    if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
        Serial.println("initialization failed!");
        while (1);
    }
    Serial.println("Success");
    String data = "";
    myFile = SD.open("schedule.txt", FILE_READ);
    if (myFile) {
        while (myFile.available()) {
            data = myFile.readString();
            Serial.println(data);
            if (data != "") {
                decode_json(data.c_str(), 1);
            }
        }
        myFile.close();
    }
}

void setup() {
    SetupDisplay();
    rtc_setup(60 * 1000);
    if (!MDNS.begin("pump-ctrl")) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");
    server.enableCORS(true);
    sd_setup();
    server.on("/", handleRoot);
    server.on("/set-schedule", HTTP_POST, []() {
        String json_txt = server.arg("plain");
        myFile = SD.open("schedule.txt", FILE_WRITE);
        if (myFile) {
            myFile.print(json_txt);
            myFile.close();
        }
        decode_json(json_txt.c_str());
        encode_json();
        server.send(200, "text/plain", return_buf);
    });
    server.on("/set-state", HTTP_POST, []() {
        String json_txt = server.arg("plain");
        DynamicJsonDocument body(7*96);   // 32*3*256(24576) + 7*96(672) + a
        DeserializationError error = deserializeJson(body, json_txt);
        String state_text = body["state"];
        decode_state(state_text.c_str());
        encode_json();
        server.send(200, "text/plain", return_buf);
    });
    server.on("/get-data", HTTP_GET, []() {
        encode_json();
        server.send(200, "text/plain", return_buf);
    });
    server.on("/post-utdate", HTTP_POST, []() {
        String json_txt = server.arg("plain");
        Serial.println(json_txt);
        DynamicJsonDocument recv_body(256);
        DeserializationError error = deserializeJson(recv_body, json_txt);
        unsigned long utdate = recv_body["utdate"];
        if (devicetime == 0) {
            devicetime = utdate + tzOffset;
            Serial.print("RTC time is: ");
            Serial.println(now.timestamp(DateTime::TIMESTAMP_FULL));
            // adjust time using ntp time
            rtc.adjust(DateTime(devicetime));
            // print boot update details
            Serial.println("RTC (boot) time updated.");
            // get and print the adjusted rtc time
            now = rtc.now();
            Serial.print("Adjusted RTC (boot) time is: ");
            Serial.println(now.timestamp(DateTime::TIMESTAMP_FULL));
        }
        server.send(200, "text/plain", "Success");
    });
    server.onNotFound(handleNotFound);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("HTTP server started");
}

void cron_delay(){
    if (now_time == now.hour() * 100 + now.minute())   // already triggered
        return;
    now_time = now.hour() * 100 + now.minute();
    almptr = &alarms[0];
    for(int i = 0; i < almcnt; i++){
        if (now_time == almptr[i].awake_time){
            alarm_handler(i);
        }
    }
}

int bin_buf[7];
void ChangeBin(int n, int dec, int *bin)
{
    for (int i = 0; i < MAX_CH; i++) {
        bin[i] = dec % n;
        dec = dec / n;
    }
}

void alarm_handler(int id){
    int *ptr = &bin_buf[0];
    ChangeBin(2, almptr[id].week_day, ptr);
    if (ptr[now.dayOfTheWeek()]){
        ChangeBin(3, almptr[id].state, ptr);   // Reuse Buffer
        Pump *current = &current_state[0];
        for (int i = 0; i < MAX_CH; i++){
            if (ptr[i] > 0){
                current[i].state = ptr[i];            // Change State
            }
        }
    }
}
 
void loop() {
    rtc_update();
    now = rtc.now();
    time_str = now.timestamp(DateTime::TIMESTAMP_DATE) + "  " + now.timestamp(DateTime::TIMESTAMP_TIME);
    ShowText(&time_str);
    ShowPompState();
    server.handleClient();
    cron_delay();
    delay(1000);
}
