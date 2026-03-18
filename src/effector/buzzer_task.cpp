#include "buzzer_task.h"

void buzzer_callback(dds_callback_context_t* context) {
    buzzer_command_t* cmd = (buzzer_command_t*)context->message_data.data;

    if (cmd->signal == BUZZER_SIGNAL_START) {
        // Uplifting ascending melody (C4 → E4 → G4)
        tone(BUZZER_PIN, 262, 150); vTaskDelay(pdMS_TO_TICKS(180));
        tone(BUZZER_PIN, 330, 150); vTaskDelay(pdMS_TO_TICKS(180));
        tone(BUZZER_PIN, 392, 300); vTaskDelay(pdMS_TO_TICKS(350));
        noTone(BUZZER_PIN);
    }
    else if (cmd->signal == BUZZER_SIGNAL_CLOSER) {
        // Uplifting double beep — two quick rising tones
        tone(BUZZER_PIN, 880, 100); vTaskDelay(pdMS_TO_TICKS(130));
        noTone(BUZZER_PIN);         vTaskDelay(pdMS_TO_TICKS(60));
        tone(BUZZER_PIN, 1047, 100); vTaskDelay(pdMS_TO_TICKS(130));
        noTone(BUZZER_PIN);
    }
    else if (cmd->signal == BUZZER_SIGNAL_FURTHER) {
        // Sad descending beep (G4 → D4)
        tone(BUZZER_PIN, 392, 200); vTaskDelay(pdMS_TO_TICKS(230));
        tone(BUZZER_PIN, 294, 400); vTaskDelay(pdMS_TO_TICKS(450));
        noTone(BUZZER_PIN);
    }
    else if (cmd->signal == BUZZER_SIGNAL_NEARBY) {
        uint8_t loudness = cmd->loudness;
        uint32_t interval_ms = 80 + ((255 - loudness) * 520 / 255); // 80–600ms

        uint32_t elapsed = 0;
        while (elapsed < 1000) {  // fill the ~1s GPS update window
            tone(BUZZER_PIN, 1000, 60);
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
            noTone(BUZZER_PIN);
            elapsed += interval_ms;
        }
    }
}

static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }
void buzzer_task(void* parameter) {
    Serial.printf("Thread task started with handle %p\n", xTaskGetCurrentTaskHandle());

    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(5, sizeof(dds_callback_context_t));
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

    dds_result_t result = DDS_SUBSCRIBE("/buzzer", buzzer_callback, &thread_context);
    if (result != DDS_SUCCESS) {
        Serial.println("Failed to subscribe to buzzer topic");
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