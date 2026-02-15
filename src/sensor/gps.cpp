#include "gps.h"

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }
void gps_task(void* parameter) {
    Serial.printf("Thread task started with handle %p\n", xTaskGetCurrentTaskHandle());

    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(20, sizeof(dds_callback_context_t));
    thread_context.sync_mutex = xSemaphoreCreateMutex();
    
    esp_timer_create_args_t timer_args = {
        .callback = &thread_timer_callback,
        .arg = NULL,
    };
    esp_timer_create(&timer_args, &(thread_context.timer));
    esp_timer_start_periodic(thread_context.timer, 1000 * 1000); // 1000 ms

    // ------- THREAD SETUP CODE START -------

    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    gpsSerial.setTimeout(10);

    // ------- THREAD SETUP CODE END -------

    vTaskDelay(100);
    
    while(1) {
        // Wait for any notification (message or timer)
        uint32_t notification_value;
        xTaskNotifyWait(0x00, 0xFF, &notification_value, portMAX_DELAY);
        
        if (notification_value & DDS_NOTIFY_BIT) { // DDS message notification
            DDS_TAKE_MUTEX(&thread_context);
            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
        if (notification_value & THREAD_NOTIFY_BIT) { // Timer tick notification
            DDS_TAKE_MUTEX(&thread_context);

            // ------- THREAD LOOP CODE START -------

            if (gpsSerial.available() > 0) {
                while (gpsSerial.available() > 0) {
                    gps.encode(gpsSerial.read());
                }

                gps_data_t gps_data = {
                    .lat = gps.location.lat(),
                    .lng = gps.location.lng(),
                    .speed = gps.speed.kmph(),
                    .altitude = gps.altitude.meters(),
                    .hdop = gps.hdop.value() / 100.0,
                    .satellites = gps.satellites.value(),
                    .year = gps.date.year(),
                    .month = gps.date.month(),
                    .day = gps.date.day(),
                    .hour = gps.time.hour(),
                    .minute = gps.time.minute(),
                    .second = gps.time.second()
                };
                DDS_PUBLISH("/gps", gps_data); // IMPORTANT: Check hdop value before using GPS data, if hdop is too high, the data may be inaccurate
            }

            // ------- THREAD LOOP CODE END -------

            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
    }
}