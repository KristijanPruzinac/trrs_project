#include "gps.h"

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

double previous_lat = 0;
double previous_lng = 0;

static dds_thread_context_t thread_context;
static void interrupt_gps_data_received() {
    detachInterrupt(GPS_RX_PIN);
    xTaskNotifyFromISR(thread_context.task, THREAD_NOTIFY_BIT, eSetBits, NULL);
}
void gps_task(void* parameter) {
    Serial.printf("Thread task started with handle %p\n", xTaskGetCurrentTaskHandle());

    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(5, sizeof(dds_callback_context_t));
    thread_context.sync_mutex = xSemaphoreCreateMutex();

    attachInterrupt(GPS_RX_PIN, interrupt_gps_data_received, RISING); // We are using interrupt

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

            while (gpsSerial.available() > 0) {
                gps.encode(gpsSerial.read());
            }

            if ((gps.location.lat() != previous_lat || gps.location.lng() != previous_lng) && gps.location.isValid() && gps.hdop.value() / 100.0 < 10.0) {
                previous_lat = gps.location.lat();
                previous_lng = gps.location.lng();
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
                DDS_PUBLISH("/gps", gps_data);
            }

            attachInterrupt(GPS_RX_PIN, interrupt_gps_data_received, RISING);

            // ------- THREAD LOOP CODE END -------

            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
    }
}