#pragma once
#include <Arduino.h>

enum NotificationType { NOTIF_NONE, NOTIF_SEND_SUCCESS, NOTIF_SEND_FAILED };

void display_init();
void display_set_weight(float w);
void display_set_battery(float pct);
void display_set_bt_connected(bool connected);
void display_show_notification(NotificationType notif);
void display_tick();   // call every loop — throttled internally