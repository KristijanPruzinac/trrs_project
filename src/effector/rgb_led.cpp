#include "rgb_led.h"

#include "rgb_led.h"

// State machine variables (static inside the file)
static rgb_signal_t current_signal = RGB_SIGNAL_NONE;
static int animation_step = 0;
static int animation_max_steps = 80; // 80 steps * 10ms = 800ms
static unsigned long signal_start_time = 0;
static int brightness = 0;
static bool constant_blue = true;

static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }

void rgb_led_topic_callback(dds_callback_context_t* context) {
    rgb_led_command_t* cmd = (rgb_led_command_t*)context->message_data.data;
    
    // For constant blue, store brightness
    if (cmd->signal == RGB_SIGNAL_CONSTANT_BLUE) {
        brightness = cmd->brightness;
        constant_blue = true;
    }
    else if (cmd->signal == RGB_SIGNAL_CONSTANT_RGB) {
        brightness = cmd->brightness;
        constant_blue = false;
    }
    else {
        current_signal = cmd->signal;
        animation_step = 0;
        signal_start_time = millis();
    }
}

// Animation function called from timer
static void update_led_animation(void) {
    int r = 0, g = 0, b = 0;
    
    switch (current_signal) {
        case RGB_SIGNAL_NONE:
            if (constant_blue) {
                r = 0;
                g = (int) (brightness * 0.35);
                b = brightness;
            } else {
                int hue = (animation_step * 5) % 360; // Cycle through hues
                // Very simple RGB approximation
                if (hue < 60) {
                    r = 255; g = hue * 4; b = 0;
                } else if (hue < 120) {
                    r = 255 - (hue-60)*4; g = 255; b = 0;
                } else if (hue < 180) {
                    r = 0; g = 255; b = (hue-120)*4;
                } else if (hue < 240) {
                    r = 0; g = 255 - (hue-180)*4; b = 255;
                } else if (hue < 300) {
                    r = (hue-240)*4; g = 0; b = 255;
                } else {
                    r = 255; g = 0; b = 255 - (hue-300)*4;
                }

                float gamma = 2.2;  // Typical gamma value for LEDs
                float normalized = brightness / 255.0;
                float gamma_corrected = pow(normalized, gamma);  // This is quadratic when gamma=2

                r = (int)(r * gamma_corrected);
                g = (int)(g * gamma_corrected);
                b = (int)(b * gamma_corrected);
            }
            break;
            
        case RGB_SIGNAL_GPS_FIX:
            // Simple rainbow: cycle through colors
            // Each step changes hue
            {
                int hue = (animation_step * 5) % 360; // Cycle through hues
                // Very simple RGB approximation
                if (hue < 60) {
                    r = 255; g = hue * 4; b = 0;
                } else if (hue < 120) {
                    r = 255 - (hue-60)*4; g = 255; b = 0;
                } else if (hue < 180) {
                    r = 0; g = 255; b = (hue-120)*4;
                } else if (hue < 240) {
                    r = 0; g = 255 - (hue-180)*4; b = 255;
                } else if (hue < 300) {
                    r = (hue-240)*4; g = 0; b = 255;
                } else {
                    r = 255; g = 0; b = 255 - (hue-300)*4;
                }
            }
            break;
            
        case RGB_SIGNAL_CLOSER:
            // Green fade blink - 3 times in 800ms
            {
                int cycle = (animation_step / 13) % 3; // 3 blinks
                int pos = animation_step % 13;
                if (pos < 6) {
                    g = (pos * 255) / 5; // Fade up
                } else {
                    g = ((12 - pos) * 255) / 5; // Fade down
                }
            }
            break;
            
        case RGB_SIGNAL_FURTHER:
            // Red fade blink - 3 times
            {
                int cycle = (animation_step / 13) % 3;
                int pos = animation_step % 13;
                if (pos < 6) {
                    r = (pos * 255) / 5; // Fade up
                } else {
                    r = ((12 - pos) * 255) / 5; // Fade down
                }
            }
            break;
    }
    
    analogWrite(RGB_LED_RED_PIN, r);
    analogWrite(RGB_LED_GREEN_PIN, g);
    analogWrite(RGB_LED_BLUE_PIN, b);
    
    // Increment step and check if animation is done
    animation_step++;
    if (animation_step >= animation_max_steps) {
        current_signal = RGB_SIGNAL_NONE;
    }
}

void rgb_led_task(void* parameter) {
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

            update_led_animation();

            // ------- THREAD LOOP CODE END -------

            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
    }
}