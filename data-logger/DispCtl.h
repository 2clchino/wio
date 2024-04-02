#include "Free_Fonts.h"
#include "TFT_eSPI.h"
#include "name_ctrl.h"

#define MAX_CH 7
#define CUR_CH MAX_CH + 1
float current_val[CUR_CH] = {0};
String channelNames[CUR_CH];
