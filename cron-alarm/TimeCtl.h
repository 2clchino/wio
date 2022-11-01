#include "rpcWiFi.h"
#include <millisDelay.h>
#include <Wire.h>
#include "RTC_SAMD51.h"
#include "DateTime.h"
#include "env.h"
millisDelay updateDelay; // the update delay object. used for ntp periodic update.
 
unsigned int localPort = 2390;      // local port to listen for UDP packets
 
// switch between local and remote time servers
// comment out to use remote server
//#define USELOCALNTP
 
#ifdef USELOCALNTP
    char timeServer[] = "n.n.n.n"; // local NTP server 
#else
    char timeServer[] = "time.nist.gov"; // extenral NTP server e.g. time.nist.gov
#endif
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
 
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
 
// declare a time object
DateTime now;
 
// define WiFI client
WiFiClient client;
 
//The udp library class
WiFiUDP udp;
 
// localtime
unsigned long devicetime;
 
RTC_SAMD51 rtc;
 
// for use by the Adafuit RTClib library
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

typedef struct _ALARM {
    char *alarm_name;
    char *alarm_cron;
    int state;
} Alarm;
