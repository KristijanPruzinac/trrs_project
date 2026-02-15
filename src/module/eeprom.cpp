#include "eeprom.h"

// Magic number to validate EEPROM data
const uint8_t EEPROM_MAGIC = 0x42;

bool eeprom_init() {
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Check if EEPROM is initialized
    if (EEPROM.length() == 0) {
        return false;
    }

    if (EEPROM.read(0) != EEPROM_MAGIC) {
        for(int i = 0; i < EEPROM_SIZE; i++) {
            EEPROM.write(i, 0);
        }
        EEPROM.commit();
    }
    
    return true;
}

bool eeprom_save_data(eeprom_data_t data) {
    // Add magic number for validation
    data.magic_number = EEPROM_MAGIC;
    
    // Check if data fits in EEPROM
    if (sizeof(eeprom_data_t) > EEPROM.length()) {
        return false;
    }
    
    // Write data correctly
    EEPROM.put(0, data);
    
    return EEPROM.commit();
}

bool eeprom_load_data(eeprom_data_t *data) {
    if (!data) return false;
    
    // Check if data fits in EEPROM
    if (sizeof(eeprom_data_t) > EEPROM.length()) {
        return false;
    }
    
    // Read data
    EEPROM.get(0, *data);
    
    // Validate data with magic number
    if (data->magic_number != EEPROM_MAGIC) {
        // Data is invalid (first boot or corrupted)
        return false;
    }
    
    return true;
}