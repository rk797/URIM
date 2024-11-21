#include "CEEPROM.h"

// Clear the EEPROM by writing 0xFF (default uninitialized state) to every address
void CEEPROM::ClearEEPROM()
{
    for (uint16_t i = 0; i < EEPROM.length(); i++)
    {
        EEPROM.write(i, 0xFF);  // Set all addresses to 0xFF
    }
}

// Check if the EEPROM is empty (all bytes set to 0xFF)
bool CEEPROM::IsEEPROMEmpty()
{
    for (uint16_t i = 0; i < EEPROM.length(); i++)
    {
        if (EEPROM.read(i) != 0xFF)
        {
            return false;  // EEPROM is not empty
        }
    }
    return true;  // EEPROM is empty
}
