#ifndef DEFINITIONS
#define DEFINITIONS

#include <Arduino.h>

#define GPS_BAUD 9600
#define GPS_RX_PIN 18
#define GPS_TX_PIN 19

#define RGB_LED_RED_PIN 25
#define RGB_LED_GREEN_PIN 26
#define RGB_LED_BLUE_PIN 27

#define BUZZER_PIN 23

#define EEPROM_SIZE 512

#define AP_SSID     "ESP32-TREASURE-HUNT"
#define AP_PASSWORD "12345678"

#define GPS_RING_WIDTH 5

typedef enum {
    RGB_SIGNAL_NONE = 0,
    RGB_SIGNAL_GPS_FIX,        // Rainbow pattern
    RGB_SIGNAL_CONSTANT_BLUE,  // Solid blue with brightness
    RGB_SIGNAL_CONSTANT_RGB,  // Solid blue with brightness
    RGB_SIGNAL_CLOSER,         // Green fade blink
    RGB_SIGNAL_FURTHER         // Red fade blink
} rgb_signal_t;

typedef struct {
    rgb_signal_t signal;
    uint8_t brightness;
} rgb_led_command_t;

typedef enum {
    BUZZER_SIGNAL_NONE = 0,
    BUZZER_SIGNAL_START,
    BUZZER_SIGNAL_NEARBY,
    BUZZER_SIGNAL_CLOSER,
    BUZZER_SIGNAL_FURTHER
} buzzer_signal_t;

typedef struct {
    buzzer_signal_t signal;
    uint8_t loudness;
} buzzer_command_t;

typedef struct {
    double lat;
    double lng;
    double speed;
    double altitude;
    float hdop;
    int satellites;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} gps_data_t;

typedef struct {
    uint8_t magic_number;
    double lat;
    double lng;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} eeprom_data_t;

//Haversine formula for distance calculation
#define R 6371
#define TO_RAD (3.1415926536 / 180)
double haversine_dist(double th1, double ph1, double th2, double ph2);

#endif