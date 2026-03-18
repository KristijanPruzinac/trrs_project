#include "main_task.h"

int current_gps_ring = -1;

void gps_topic_callback(dds_callback_context_t* context) {
    gps_data_t* data = (gps_data_t*)context->message_data.data;

    double distance = haversine_dist(data->lat, data->lng, target_lat, target_lng) * 1000.0;

    //Serial.printf("%f, %f, %f\n", data->lat, data->lng, distance);

    int ring = (int)(distance / GPS_RING_WIDTH);
    if (current_gps_ring == -1) {
        current_gps_ring = ring;
        rgb_led_command_t cmd = {RGB_SIGNAL_GPS_FIX, 255};
        DDS_PUBLISH("/rgb_led", cmd);

        buzzer_command_t b_cmd = {BUZZER_SIGNAL_START, 255};
        DDS_PUBLISH("/buzzer", b_cmd);
        return;
    }

    if (ring != current_gps_ring && ring % 2 == 0) {
        if (ring < current_gps_ring) {
            rgb_led_command_t cmd = {RGB_SIGNAL_CLOSER, 255};
            DDS_PUBLISH("/rgb_led", cmd);

            buzzer_command_t b_cmd = {BUZZER_SIGNAL_CLOSER, 255};
            DDS_PUBLISH("/buzzer", b_cmd);
        }
        else {
            rgb_led_command_t cmd = {RGB_SIGNAL_FURTHER, 255};
            DDS_PUBLISH("/rgb_led", cmd);

            buzzer_command_t b_cmd = {BUZZER_SIGNAL_FURTHER, 255};
            DDS_PUBLISH("/buzzer", b_cmd);
        }

        current_gps_ring = ring;
    }
    else if (ring == 0) {
        rgb_led_command_t cmd = {RGB_SIGNAL_CONSTANT_RGB, 255 - constrain((int) (distance / GPS_RING_WIDTH * 255.0), 0, 255)};
        DDS_PUBLISH("/rgb_led", cmd);

        buzzer_command_t b_cmd = {BUZZER_SIGNAL_NEARBY, 255 - constrain((int) (distance / GPS_RING_WIDTH * 255.0), 0, 255)};
        DDS_PUBLISH("/buzzer", b_cmd);
    }
    else {
        rgb_led_command_t cmd = {RGB_SIGNAL_CONSTANT_BLUE, 255 - constrain((int) (distance / 1000.0 * 255.0), 0, 255)};
        DDS_PUBLISH("/rgb_led", cmd);
    }
}

static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }
void main_task(void* parameter) {
    Serial.printf("Thread task started with handle %p\n", xTaskGetCurrentTaskHandle());

    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(5, sizeof(dds_callback_context_t));
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