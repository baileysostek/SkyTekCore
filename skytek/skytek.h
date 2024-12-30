// SkyTek is a a closed source project created by Bailey Sostek in May of 2023
#define SKYTEK_API_VERSION "1.0"
#define VERSION "0.3" // Board Software Version

// Define the number of entries a SkyTek device is allowed to have keyword callbacks registered for.
// TODO: maybe in the future we dynamically grow this, but feels like a bad idea on a microcontroller with limited resources.
#define MAX_USER_DEFINED_MESSAGE_TYPES 128

// Include all of our SkyTek headers.
#include "message.h"

bool skytek_initialized = false;
void skytek_init () {
  if (!skytek_initialized) {
    // Initialize hash table
    for (int i = 0; i < MAX_USER_DEFINED_MESSAGE_TYPES; i++) {
      hashTable[i] = NULL;
    }
    // Set that we have initialized
    skytek_initialized = true;
  }
}

void skytek_update () {
    parse_serial_command();
}