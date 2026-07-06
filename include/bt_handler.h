#pragma once
#include <Arduino.h>
#include "display_mgr.h"

void bt_init();
void bt_tick();           // call every loop
bool bt_is_connected();