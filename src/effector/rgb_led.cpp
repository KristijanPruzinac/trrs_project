#include "rgb_led.h"

void rgb_led_topic_callback(dds_callback_context_t* context) {
    rgb_led_data_t* data = (rgb_led_data_t*)context->message_data.data;
    analogWrite(RGB_LED_RED_PIN, 255 - data->r);
    analogWrite(RGB_LED_GREEN_PIN, 255 - data->g);
    analogWrite(RGB_LED_BLUE_PIN, 255 - data->b);
}

static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }
void rgb_led_task(void* parameter) {
    Serial.printf("Thread task started with handle %p\n", xTaskGetCurrentTaskHandle());

    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(20, sizeof(dds_callback_context_t));
    thread_context.sync_mutex = xSemaphoreCreateMutex();
    
    esp_timer_create_args_t timer_args = {
        .callback = &thread_timer_callback,
        .arg = NULL,
    };
    esp_timer_create(&timer_args, &(thread_context.timer));
    esp_timer_start_periodic(thread_context.timer, 10 * 1000); // 10 ms

    // ------- THREAD SETUP CODE START -------

    pinMode(RGB_LED_RED_PIN, OUTPUT);
    pinMode(RGB_LED_GREEN_PIN, OUTPUT);
    pinMode(RGB_LED_BLUE_PIN, OUTPUT);

    digitalWrite(RGB_LED_RED_PIN, HIGH);
    digitalWrite(RGB_LED_GREEN_PIN, HIGH);
    digitalWrite(RGB_LED_BLUE_PIN, HIGH);

    dds_result_t result = DDS_SUBSCRIBE("/rgb_led", rgb_led_topic_callback, &thread_context);
    if (result != DDS_SUCCESS) {
        Serial.println("Failed to subscribe to topic");
        while(1) vTaskDelay(1000);
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