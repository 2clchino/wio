#include "Free_Fonts.h"
#include "TFT_eSPI.h"

#define MAX_CH 7
typedef struct _PUMP {
    String pump_name;
    int state; // manual: 0, auto off: 1, auto on: 2 
} Pump;

typedef struct _ALARM {
    int awake_time;
    int week_day;   // not change: 0, change: 1                       ( binary encoded )
    int state;      // not change: 0, change->off: 1 , change->on: 2  ( ternary encoded )
} Alarm;

Pump current_state[MAX_CH];
int onoff[MAX_CH] = {0};
int rstat[MAX_CH] = {BCM0, BCM5, BCM6, BCM13, BCM19, BCM26, BCM21};
