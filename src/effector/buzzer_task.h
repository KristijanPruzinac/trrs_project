#ifndef BUZZER_TASK_H
#define BUZZER_TASK_H

#include <Arduino.h>
#include "esp_dds.h"
#include "esp_timer.h"
#include "../definitions.h"

void buzzer_task(void* parameter);

#endif // BUZZER_TASK_H