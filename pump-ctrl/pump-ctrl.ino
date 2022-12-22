#include "TimeCtl.h"
#include "7switch.h"
#include <CronAlarms.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <RPCmDNS.h>

WebServer server(80);
Alarm *almptr;
int almidx = 0;
CronId id;
String now_time = "";
char json_buf[256];
char return_json_buf[256*256];

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

void alarmMatchCron(){
    id = Cron.getTriggeredCronId();
    int *sw = &onoff[0];
    DynamicJsonDocument states(128);
    deserializeJson(states, almptr[id].state);
    for (int i = 0; i < MAX_CH; i++){
        String tmp = states[i];
        int state = 0;
        int res = sscanf(tmp.c_str(), "%d", &state);
        if (state != -1){
            sw[i] = state;
        }
    }
    Serial.print("Edit state by Cron ");
    Serial.println(id);
}

int set_cron(int cid, const char *tm, const char *st){
    if (almidx > cid)     // free last crons
        Cron.free(cid);
    if (st[0] != '[')     // check st == null
        return -1;
    int res = Cron.create(tm, alarmMatchCron, false);
    if (res == -1) {
        Serial.print("Error on register Cron");
    }
    Alarm alarm;
    alarm.alarm_cron = strdup(tm);
    //alarm.state = strdup(st);
    almptr[cid] = alarm;
    return cid;
}

int decode_packets(const char *_current, const char *_cron){
    DynamicJsonDocument curs(MAX_CH*128);
    deserializeJson(curs, _current);
    Pump *pumptr = &current_state[0];
    int almcnt = 0;
    for (int i = 0; i < MAX_CH; i++){
        String tmp = curs[i];
        StaticJsonDocument<JSON_OBJECT_SIZE(10) + 32> cur;
        deserializeJson(cur, tmp);
        String _name = cur["name"];
        String _sw = cur["state"];
        _name.toCharArray(pumptr[i].pump_name, 50);
        int res = sscanf( _sw.c_str(), "%d", &onoff[i]);
    }
    Serial.println(_cron);
    StaticJsonDocument<JSON_OBJECT_SIZE(2*256) + 32> crons;     // TODO: increase memory
    deserializeJson(crons, _cron);
    for (int i = 0; i < 256; i++){
        String tmp = crons[i];
        DynamicJsonDocument cron(256);
        deserializeJson(cron, tmp);
        String tm = cron["cron"];
        String st = cron["state"];
        if (set_cron(i, tm.c_str(), st.c_str()) != -1) {
            almcnt++;
            Serial.println(tm);
            Serial.println(st);
        }
    }
    return almidx = almcnt;
}


char *show_alarm(Alarm *alarm, int index){
    StaticJsonDocument<JSON_OBJECT_SIZE(12) + 32> json_obj;
    json_obj["id"] = index;
    json_obj["cron"] = alarm->alarm_cron;
    json_obj["state"] = alarm->state;
    serializeJson(json_obj, json_buf);
    Serial.println(json_buf);
    return json_buf;
}

char *show_alarms(){
    Serial.println(almidx);
    StaticJsonDocument<JSON_OBJECT_SIZE(64*256) + 32> json_array;
    for (int i = 0; i < almidx; i++){
        json_array[i] = show_alarm(&almptr[i], i);
    }
    serializeJson(json_array, return_json_buf);
    return return_json_buf;
}

char *show_states(){
    StaticJsonDocument<128> doc;
    JsonArray json_obj = doc.createNestedArray("pumps");
    Pump *p_state = &current_state[0];
    for (int i = 0; i < MAX_CH; i++){
        json_obj.add(p_state[i].state);
    }
    serializeJson(doc, json_buf);
    return json_buf;
}

char *return_data(){
    StaticJsonDocument<JSON_OBJECT_SIZE(256) + 32> json_response;
    json_response["state"] = show_states();
    json_response["alarm"] = show_alarms();
    serializeJson(json_response, return_json_buf, sizeof(return_json_buf));
    Serial.println(return_json_buf);
    return return_json_buf;
}

void setup() {
    Alarm alarm[256];
    almptr = &alarm[0];
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
    server.on("/", handleRoot);
    server.on("/set-data", HTTP_POST, []() {
        String json_txt = server.arg("plain");
        StaticJsonDocument<100*256> body;       // 7*96+168*128 = 22176
        DeserializationError error = deserializeJson(body, json_txt);
        Pump *pumptr = &current_state[0];
        String tp = body["state"];
        StaticJsonDocument<7*96> pump;
        error = deserializeJson(pump, tp);
        Serial.println(tp);
        for (int i = 0; i < MAX_CH; i++){
            String tmp = pump[i];
            Serial.println(tmp);
            StaticJsonDocument<96> cur;
            deserializeJson(cur, tmp);
            String _name = cur["name"];
            String _sw = cur["state"];
            _name.toCharArray(pumptr[i].pump_name, sizeof(char)*12);
            int res = sscanf( _sw.c_str(), "%d", &onoff[i]);
        }
        
        int almcnt = 0;
        String al = body["alarm"];
        error = deserializeJson(body, al);
        Serial.println(al);
        for (int i = 0; i < 256; i++){
            String tmp = body[i];
            StaticJsonDocument<168> cron;
            deserializeJson(cron, tmp);
            String tm = cron["cron"];
            String st = cron["state"];
            if (set_cron(i, tm.c_str(), st.c_str()) != -1) {
                almcnt++;
                Serial.println(tm);
                Serial.println(st);
            }
        }
        almidx = almcnt;
        server.send(200, "text/plain", show_states());
    });
    server.on("/get-pumps", HTTP_GET, [] () {
        server.send(200, "text/plain", show_states());
    });
    server.on("/get-alarms", HTTP_GET, []() {
        server.send(200, "text/plain", show_alarms());
    });
    server.onNotFound(handleNotFound);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("HTTP server started");
}
 
void loop() {
    rtc_update();
    now = rtc.now();
    now_time = now.timestamp(DateTime::TIMESTAMP_DATE) + "  " + now.timestamp(DateTime::TIMESTAMP_TIME);
    ShowTime(&now_time);
    ShowPompState();
    server.handleClient();
    Cron.delay(500);
}
