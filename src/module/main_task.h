#ifndef MAIN_TASK_H
#define MAIN_TASK_H

#include <Arduino.h>
#include "esp_dds.h"
#include "esp_timer.h"
#include "../definitions.h"

#include "eeprom.h"

void main_task(void* parameter);

#endif // MAIN_TASK_H