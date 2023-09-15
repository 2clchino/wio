#include "Free_Fonts.h"
#include "TFT_eSPI.h"
#include "name_ctrl.h"

#define MAX_CH 7
#define VOL_CH 6
#define CUR_CH MAX_CH + 1
float current_val[CUR_CH] = {0};
