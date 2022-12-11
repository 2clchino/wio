#include "TimeCtl.h"
#include "7switch.h"
#include <CronAlarms.h>
#include <WebServer.h>
#include <ArduinoJson.h>
 
WebServer server(80);
Alarm *almptr;
int almidx = 0;
int sw_state = 0;
CronId id;
char alm_json[256];
char pump_json[256];
char alms_json[256*256];

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
    sw_state = almptr[id].state;
    Serial.print("Edit state by Cron");
    Serial.println(id);
}

int set_cron(const char *nm, const char *tm, const char *st){
    int state;
    int res = sscanf(st, "%d", &state);
    if (!(state < 128 && res))
        return -1;
    int cid = Cron.create(tm, alarmMatchCron, false);
    if (cid < 255){
        Alarm alarm;
        alarm.alarm_name = strdup(nm);
        alarm.alarm_cron = strdup(tm);
        alarm.state = state;
        almptr[cid] = alarm;
        show_alarm(&alarm, cid);
        if (cid > almidx-1)
          almidx++;
        return cid;
    } else {
        return -1;
    }
}

int unset_cron(const char *_cid){
    int cid;
    int res = sscanf(_cid, "%d", &cid);
    if (!(cid < 255 && res))
        return -1;
    Cron.free(cid);
    Alarm alarm;
    alarm.alarm_cron = "";
    alarm.alarm_name = "";
    alarm.state = -1;
    almptr[cid] = alarm;
    return cid;
}

int edit_state(const char *_state){
    int state_num = 0;
    int res = sscanf(_state, "%d", &state_num);
    if (!(state_num < 255 && res))
        return -1;
    sw_state = state_num;
    return sw_state;
}

char *show_alarm(Alarm *alarm, int index){
    Serial.println(alarm->alarm_cron);
    StaticJsonDocument<JSON_OBJECT_SIZE(4) + 32> json_obj;
    json_obj["id"] = index;
    json_obj["name"] = alarm->alarm_name;
    json_obj["cron"] = alarm->alarm_cron;
    json_obj["state"] = alarm->state;
    serializeJson(json_obj, alm_json, sizeof(alm_json));
    Serial.println(alm_json);
    return alm_json;
}

char *show_alarms(){
    StaticJsonDocument<JSON_OBJECT_SIZE(256)> json_array;
    for (int i = 0; i < almidx; i++){
        json_array[i] = show_alarm(&almptr[i], i);
    }
    //json_array[almidx] = sw_state;
    serializeJson(json_array, alms_json);
    return alms_json;
}

char *show_states(){
    StaticJsonDocument<128> doc;
    JsonArray json_obj = doc.createNestedArray("pumps");
    int *p_state = &pump_state[0];
    for (int i = 0; i < MAX_CH; i++){
        json_obj.add(p_state[i]);
    }
    serializeJson(doc, pump_json);
    Serial.println(pump_json);
    return pump_json;
}

void setup() {
    Alarm alarm[256];
    almptr = &alarm[0];
    SetupDisplay();
    rtc_setup(60 * 1000);
    server.enableCORS(true);
    server.on("/", handleRoot);
    server.on("/set-cron", HTTP_POST, []() {
        Serial.print("Catch Request");
        String json_txt = server.arg("plain");
        DynamicJsonDocument json_response(255);
        deserializeJson(json_response, json_txt);
        String tm = json_response["cron"];  // server.arg("cron");
        String nm = json_response["name"];  // server.arg("name");
        String st = json_response["state"]; // server.arg("state");
        int cid = set_cron(nm.c_str(), tm.c_str(), st.c_str());
        if (cid != -1){
            server.send(200, "text/plain", "Success to register alarm !!");
        } else {
            server.send(500, "text/plain", "Format is wrong !!");
        }
    });
    server.on("/unset-cron", HTTP_POST, []() {
        String json_txt = server.arg("plain");
        DynamicJsonDocument json_response(255);
        deserializeJson(json_response, json_txt);
        String _cid = json_response["id"];  // server.arg("id");
        if (unset_cron(_cid.c_str()) != -1){
            server.send(200, "text/plain", "Success to remove alarm !!");
        } else {
            server.send(500, "text/plain", "Format is wrong !!");
        }
    });
    server.on("/edit-pumps", HTTP_POST, []() {
        String json_txt = server.arg("plain");
        DynamicJsonDocument json_response(255);
        deserializeJson(json_response, json_txt);
        String state = json_response["state"];  // server.arg("id");
        if (edit_state(state.c_str()) != -1){
            server.send(200, "text/plain", "Success to edit state !!");
        } else {
            server.send(500, "text/plain", "Format is wrong !!");
        }
    });
    server.on("/get-pumps", HTTP_GET, [] () {
        server.send(200, "text/plain", show_states());
    });
    server.on("/get-alarms", HTTP_GET, []() {
        server.send(200, "text/plain", show_alarms());
    });
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}
 
void loop() {
    ShowTime(rtc_update());
    Cron.delay();
    ShowPompState(sw_state);
    delay(1000);
    server.handleClient();
}
