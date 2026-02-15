#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>
#include "esp_dds.h"
#include "esp_timer.h"
#include "../definitions.h"

void rgb_led_task(void* parameter);

#endif // RGB_LED_H