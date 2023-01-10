#include "Free_Fonts.h"
#include "TFT_eSPI.h"

#define MAX_CH 7
typedef struct _PUMP {
    String pump_name;
    int state; // Backend -> FrontEnd Packet 0: manual, 1: auto off, 2: auto on
} Pump;

typedef struct _ALARM {
    int awake_time;
    int week_day;   // 0: not change, 1: change binary encoded
    int state;      // 0: not change, 1: change->off, 2: change->on ternary encoded
} Alarm;

Pump current_state[MAX_CH];
int onoff[MAX_CH] = {0};
