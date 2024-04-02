#include "TimeCtl.h"
void handleGetName();
void handleSetName();

void set_name(const char *name);
String get_name(int num);
void processWioData(String* state);

const int jsonCapacity = 512;
String wio_ip;
String wio_name;
int wio_number;
