#ifndef TEST_H
#define TEST_H

#include <Arduino.h>
#include "esp_dds.h"
#include "esp_timer.h"
#include "definitions.h"

void thread_task(void* parameter);

extern double target_lat;
extern double target_lng;

#endif