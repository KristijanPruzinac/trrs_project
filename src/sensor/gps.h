#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include "esp_dds.h"
#include "esp_timer.h"
#include "../definitions.h"

#include <TinyGPSPlus.h>

void gps_task(void* parameter);

#endif // GPS_H