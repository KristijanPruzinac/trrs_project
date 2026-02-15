#ifndef EEPROM_H
#define EEPROM_H

#include <Arduino.h>
#include "esp_dds.h"
#include "esp_timer.h"
#include "../definitions.h"

#include <EEPROM.h>

bool eeprom_init();
bool eeprom_save_data(eeprom_data_t data);
bool eeprom_load_data(eeprom_data_t* data);

#endif // EEPROM_H