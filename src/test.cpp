#include "test.h"

static int inc[] = {4, 4, 4, 3, 3, 2, 2, 1, 3, 0, 1, 2, 3, 4, 5, 6, 7, 6};
static int inc_index = 0;

static double inc_value = 1.5;

#define METERS_TO_DEGREES 0.000009

#define METERS_TO_LON_DEG(lat) (0.000008993 / cos((lat) * M_PI / 180.0))

static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }
void thread_task(void* parameter) {
    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(5, sizeof(dds_callback_context_t));
    thread_context.sync_mutex = xSemaphoreCreateMutex();
    
    esp_timer_create_args_t timer_args = {
        .callback = &thread_timer_callback,
        .arg = NULL,
    };
    esp_timer_create(&timer_args, &(thread_context.timer));
    esp_timer_start_periodic(thread_context.timer, 1000 * 1000); // 1s

    // ------- THREAD SETUP CODE START -------

    // ------- THREAD SETUP CODE END -------

    vTaskDelay(500);
    
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

            //if (inc_index < sizeof(inc) / sizeof(inc[0])) {
            if (true) {
                gps_data_t gps_data;
                
                // Calculate diagonal distance for this step
                float diagonal_m = (inc_value) * GPS_RING_WIDTH;  // e.g., 4 * 10m = 40m diagonal
                inc_value -= 0.1;
                if (inc_value < 0) {
                    inc_value = 0;
                }
                
                // Each leg of the 45-degree triangle = diagonal / √2
                // √2 ≈ 1.414, so divide by 1.414 (or multiply by 0.707)
                float leg_m = diagonal_m * 0.707106;  // 40m * 0.7071 = 28.28m each leg
                
                // Convert leg distance to degrees
                gps_data.lat = target_lat + (leg_m * 0.000008993);
                gps_data.lng = target_lng + (leg_m * METERS_TO_LON_DEG(target_lat));
                
                inc_index++;
                DDS_PUBLISH("/gps", gps_data);
            }

            // ------- THREAD LOOP CODE END -------

            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
    }
}