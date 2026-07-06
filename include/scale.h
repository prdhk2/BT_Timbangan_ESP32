#pragma once
#include <Arduino.h>

void  scale_init();
void  scale_tick();           // call every loop — reads available bytes
float scale_get_weight();     // last valid weight reading
bool  scale_has_data();       // false until first valid frame is received