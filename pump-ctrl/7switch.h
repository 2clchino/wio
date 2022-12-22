#include "Free_Fonts.h"
#include "TFT_eSPI.h"

#define MAX_CH 7
typedef struct _PUMP {
    char *pump_name;
    int state; // Backend -> FrontEnd Packet 0: manual, 1: auto off, 2: auto on
} Pump;

typedef struct _ALARM {
    char *alarm_cron;
    char *state; // FrontEnd -> Backend Packet -1: not change, 0: change->off, 1: change->on
} Alarm;

Pump current_state[MAX_CH] = {0};
int onoff[MAX_CH] = {0};
