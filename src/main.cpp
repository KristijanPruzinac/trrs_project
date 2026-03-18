#include <Arduino.h>

#include "./module/eeprom.h"

#include "esp_dds.h"
#include "esp_timer.h"

#include "./module/main_task.h"
#include "./module/webserver.h"
#include "./sensor/gps.h"
#include "./effector/rgb_led.h"
#include "./effector/buzzer_task.h"

//#include "./test.h"

void setup() {

    Serial.begin(115200);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

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
    else {
        Serial.println("EEPROM initialization success");
        Serial.printf("EEPROM data: lat=%f, lng=%f\n", eeprom_data.lat, eeprom_data.lng);

        target_lat = eeprom_data.lat;
        target_lng = eeprom_data.lng;
    }
    
    // Initialize DDS system
    DDS_INIT();

    //Create new task
    xTaskCreate(
        main_task,          // Task function
        NULL,                 // Name of task
        4096,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);

    //Create new task
    xTaskCreate(
        gps_task,          // Task function
        NULL,                 // Name of task
        4096,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);
    
    //Create new task
    xTaskCreate(
        rgb_led_task,         // Task function
        NULL,                 // Name of task
        4096,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);

    xTaskCreate(
        buzzer_task,         // Task function
        NULL,                 // Name of task
        4096,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);

    xTaskCreate(
        webServerTask,         // Task function
        NULL,                 // Name of task
        4096,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);

    //Test
    /*
    xTaskCreate(
        thread_task,         // Task function
        NULL,                 // Name of task
        4096,                // Stack size in words
        NULL,                 // Task input parameter
        1,                    // Priority of the task
        NULL);
        */
        

    vTaskDelay(200);
}

void loop(){ vTaskDelay(100); }