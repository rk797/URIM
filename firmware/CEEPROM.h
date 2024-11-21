#ifndef CEEPROM_H
#define CEEPROM_H

#include <EEPROM.h>
#include <avr/io.h>

class CEEPROM
{
public:
    // Write a generic type to EEPROM at a given address, returns true if successful
    template <typename T>
    static bool WriteMemory(uint16_t address, T data)
    {
        if (address + sizeof(T) > EEPROM.length()) {
            return false;
        }

        EEPROM.put(address, data);
        return true;
    }

    // Read a generic type from EEPROM at a given address, returns true if successful
    template <typename T>
    static bool ReadMemory(uint16_t address, T& data)
    {
        if (address + sizeof(T) > EEPROM.length()) {
            return false;
        }

        EEPROM.get(address, data);
        return true;
    }

    // Utility function to clear EEPROM by writing 0xFF to all addresses
    static void ClearEEPROM();

    // Check if EEPROM contains a specific value at a given address
    template <typename T>
    static bool IsValueInMemory(uint16_t address, T value)
    {
        T storedValue;
        if (ReadMemory(address, storedValue)) {
            return storedValue == value;
        }
        return false; // Could not read memory
    }

    // Check if EEPROM is uninitialized (all values are 0xFF)
    static bool IsEEPROMEmpty();
};

#endif
