#include <Arduino.h>
#include "./module/eeprom.h"

#include "esp_dds.h"
#include "esp_timer.h"

#include "./module/main_task.h"
#include "./sensor/gps.h"
#include "./effector/rgb_led.h"

void setup() {

    Serial.begin(115200);

    while (!Serial) {
        vTaskDelay(100);
    }
    

    if (!eeprom_init()) {
        Serial.println("EEPROM initialization failed");
    }

    eeprom_data_t eeprom_data;
    if (!eeprom_load_data(&eeprom_data)) {
        Serial.println("EEPROM data not found, initializing...");
        eeprom_data = (eeprom_data_t){
            .magic_number = NULL,
            .lat = 0,
            .lng = 0,
            .year = 0,
            .month = 0,
            .day = 0,
            .hour = 0,
            .minute = 0,
            .second = 0
        };
        if (!eeprom_save_data(eeprom_data)) {
            Serial.println("EEPROM initialization failed");
        }
    }
    
    // Initialize DDS system
    DDS_INIT();

    //Create new task
    xTaskCreate(
        main_task,          // Task function
        NULL,                 // Name of task
        16384,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);

    //Create new task
    xTaskCreate(
        gps_task,          // Task function
        NULL,                 // Name of task
        16384,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);
    
    //Create new task
    xTaskCreate(
        rgb_led_task,         // Task function
        NULL,                 // Name of task
        16384,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);
    
    vTaskDelay(200);
}

void loop(){ vTaskDelay(100); }