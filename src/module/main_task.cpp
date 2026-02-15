#include "main_task.h"

int distance_ring = 0;

//TODO: Set these values from webserver
double target_lat = 0;
double target_lng = 0;

void gps_topic_callback(dds_callback_context_t* context) {
    gps_data_t* data = (gps_data_t*)context->message_data.data;

    if (data-> lat != 0 && data->lng != 0) {
        double lat_meters = (target_lat - data->lat) * 111320;
        double lng_meters = (target_lng - data->lng) * 111320 * cos(data->lat * M_PI / 180);
        double distance = sqrt(lat_meters * lat_meters + lng_meters * lng_meters);

        double decimal = (distance / 20.0) - (int)(distance / 20.0);
        rgb_led_data_t rgb_led_data = {
            .r = 0,
            .g = 0,
            .b = 0
        };

        if (decimal >= 0.3 && decimal <= 0.7 && distance_ring < (int)(distance / 20.0)){
            for (int i = 0; i < 3; i++) {
                rgb_led_data.g = 255;
                DDS_PUBLISH("/rgb_led", rgb_led_data);
                vTaskDelay(300);
                rgb_led_data.g = 0;
                DDS_PUBLISH("/rgb_led", rgb_led_data);
                vTaskDelay(300);
            }

            distance_ring = (int)(distance / 20.0);
        }
        else if (decimal >= 0.3 && decimal <= 0.7 && distance_ring > (int)(distance / 20.0)){
            for (int i = 0; i < 3; i++) {
                rgb_led_data.r = 255;
                DDS_PUBLISH("/rgb_led", rgb_led_data);
                vTaskDelay(300);
                rgb_led_data.r = 0;
                DDS_PUBLISH("/rgb_led", rgb_led_data);
                vTaskDelay(300);
            }

            distance_ring = (int)(distance / 20.0);
        }
        else {
            rgb_led_data.b = 255 - constrain(distance / 500.0, 0, 255);
            DDS_PUBLISH("/rgb_led", rgb_led_data);
        }
    }
}

static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }
void main_task(void* parameter) {
    Serial.printf("Thread task started with handle %p\n", xTaskGetCurrentTaskHandle());

    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(20, sizeof(dds_callback_context_t));
    thread_context.sync_mutex = xSemaphoreCreateMutex();
    
    esp_timer_create_args_t timer_args = {
        .callback = &thread_timer_callback,
        .arg = NULL,
    };
    esp_timer_create(&timer_args, &(thread_context.timer));
    esp_timer_start_periodic(thread_context.timer, 100 * 1000); // 100 ms

    // ------- THREAD SETUP CODE START -------

    dds_result_t result = DDS_SUBSCRIBE("/gps", gps_topic_callback, &thread_context);
    if (result != DDS_SUCCESS) {
        Serial.printf("Topic subscribe failed: %s\n", DDS_RESULT_TO_STRING(result));
    }

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

            // ------- THREAD LOOP CODE END -------

            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
    }
}