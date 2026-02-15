#ifndef DEFINITIONS
#define DEFINITIONS

#define GPS_BAUD 9600
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

#define RGB_LED_RED_PIN 25
#define RGB_LED_GREEN_PIN 26
#define RGB_LED_BLUE_PIN 27

#define EEPROM_SIZE 512

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} rgb_led_data_t;

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
    uint32_t magic_number;
    double lat;
    double lng;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} eeprom_data_t;

#endif