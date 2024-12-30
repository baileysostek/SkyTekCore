#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a list of capabilities
#define SKYTEK_CAPABILTIES "[\"gps\", \"altitude\"]"

// Generated from Serial component
// Define our configuration command characters
#define COMMAND_START_CHARACTER '/'
#define COMMAND_END_CHARACTER '\n'
#define COMMAND_BUFFER_SIZE 64
// This buffer stores commands that a user might send to the controller.
char command_buffer[COMMAND_BUFFER_SIZE + 1] = {'\0'};
int command_message_index = 0; 

// UUID for message response handling
#define UUID_COMMAND_DELIMTER ':'
#define UUID_SIZE 32
char query_uuid_buffer[UUID_SIZE + 1] = {'\0'}; // Extra 1 for Null Terminator

// [@SkyTekCore#V1] Store the ID of this device in Memory, Initialize this value on boot from EEPROM
char device_uuid[UUID_SIZE + 1] = {'\0'}; // + 1 here is to Null Terminate our string.

// Define a structure for the hash table entries
typedef struct HashNode {
    char *key;          // String key
    void *value;        // Pointer value
    struct HashNode *next; // For handling collisions (chaining)
} HashNode;

// Define the hash table
HashNode *hashTable[MAX_USER_DEFINED_MESSAGE_TYPES];

// Hash function
unsigned int hash(const char *key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash << 5) + *key++;
    }
    return hash % MAX_USER_DEFINED_MESSAGE_TYPES;
}

// Insert a key-value pair into the hash table
void insert(const char *key, void *value) {
  unsigned int index = hash(key);
  HashNode *newNode = (HashNode *)malloc(sizeof(HashNode));
  newNode->key = strdup(key);
  newNode->value = value;
  newNode->next = hashTable[index];
  hashTable[index] = newNode;
}

// Find a value by its string key
void *find(const char *key) {
  unsigned int index = hash(key);
  HashNode *node = hashTable[index];
  while (node) {
    if (strcmp(node->key, key) == 0) {
      return node->value;
    }
    node = node->next;
  }
  return NULL; // Key not found
}

// Free the hash table
void freeTable() {
  for (int i = 0; i < MAX_USER_DEFINED_MESSAGE_TYPES; i++) {
    HashNode *node = hashTable[i];
    while (node) {
      HashNode *temp = node;
      node = node->next;
      free(temp->key);
      free(temp);
    }
    hashTable[i] = NULL;
  }
}


/**
 * This function determines if there are characters available to read from the USB stdin buffer.
 */
char read_next_character () {
  int character = getchar_timeout_us(0);
  if (character != PICO_ERROR_TIMEOUT && character != EOF) {
    return (char)character;
  }
  // No characters available, return 0
  return 0;
}

// TODO: Replace with Flat buffers.
void send_query_response(const char* message) {
  char prefix[UUID_SIZE + 16] = {'\0'}; 
  // First we want to determine if we need to send a response with an ID or not.
  if (query_uuid_buffer[0] != '\0') {
    // Publish response with the 'id' specified
    sprintf(prefix, "\"id\":\"%s\",", query_uuid_buffer);
  }
  // The message itself, TODO: This should be a flat buffer, sending JSON as a string is inefficient.
  printf("{%s\"uuid\":\"%s\",%s}\n", prefix, device_uuid, message);
}

/**
 * Here we decode the user's request and do the corresponding action.
 */
void process_serial_command () {
  // Temporary buffer for callback data.
  char message[255] = {'\0'};
  // First we check the built in messages
  if (strcmp(command_buffer, "skytek") == 0) {
    // List software Version
    sprintf(message, "\"version\":\"%s\"", SKYTEK_API_VERSION);
    send_query_response(message);
  } else if (strcmp(command_buffer, "capabilities") == 0) {
    // Query response listing all of our capabilities.
    printf("{\"id\":\"%s\",\"uuid\":\"%s\",\"capabilities\":%s}\n", query_uuid_buffer, device_uuid, SKYTEK_CAPABILTIES); // Substitute our capabilities in as a literal array.
  } else if (strcmp(command_buffer, "help") == 0) {
    // Determine all of our keys
    int num_keys = 0;
    char key_name[32] = {'\0'};
    for (int i = 0; i < MAX_USER_DEFINED_MESSAGE_TYPES; i++) {
      HashNode *node = hashTable[i];
      while (node) {
        // If this is NOT the first key we have found, add a ','
        if (num_keys > 0) {
          strcat(message, ',');
        }
        // Append this key to the array of keys we are returning
        strcat(message, sprintf(key_name, "\"%s\"", node->key));
        // Check if we have another node in this bucket.
        node = node->next;
        // Increment the number of found keys
        num_keys++;
      }
    }
    // Append the array of keys
    sprintf(message, "\"messages\":[%s]", message);
    // Send out the response
    send_query_response(message);
  } else {
    // If we get here, the requested command does not correspond to one of our built in messages.
    // We should therefore check if we have a registered handler for this kind of message.
    void* customMessageHandler = find(command_buffer);
    if (customMessageHandler != NULL) {
      // We have a custom message handler for this message, so lets call that.
      void (*callback_func)() = (void (*)())customMessageHandler;
      // Call the callback function. This may issue a response to the caller.
      callback_func();
      // Send the callback data to the caller
      sprintf(message, "\"callback\":true");
      send_query_response(message);
    } else {
      // If we get here, we did not recognise the command that the user issued.
      char message[128 + COMMAND_BUFFER_SIZE] = {'\0'};
      if (command_buffer[0] == '\0') {
        sprintf(message, "\"error\":true,\"msg\":\"Error: No command was specified.\"");
      } else {
        sprintf(message, "\"error\":true,\"msg\":\"Error: Command '%s' was not recognised.\"", command_buffer);
      }
      send_query_response(message);
    }
  }
}

/**
 * Parse the incoming serial command if it exists.
 * 
 */
// Command handler to read and process the incoming serial commands from the SkyTek Flight Software.
void parse_serial_command(){
  // Store when we have found the command start character.
  bool parsing_command = false;
  while (true) {
    // Get the character that is available 
    char command_character = read_next_character();
    // Check if the character is 0
    if (command_character == 0) {
      break;
    }
    // If we are not parsing a command yet, We need to check if the sent character is a command start character.
    if (!parsing_command) {
      // If this is a command start character.
      if (command_character == COMMAND_START_CHARACTER) {
        // We know that we are now either parsing command data or a UUID.
        command_message_index = 0;
        parsing_command = true;

        // Some commands issued to a SkyTek device are prefixed with a UUID, that the edge device re-publishes when a response is ready.
        // Here we need to determine if this is a command that requires a response or not.
        while (true) {
          char uuid_character = read_next_character();
          // Check if the character is 0
          if (uuid_character == 0) {
            break;
          }
          // Check that we have not hit the escape character.
          if (uuid_character == UUID_COMMAND_DELIMTER) {
            // We have a real UUID so clear out our command buffer.
            for(int i = 0; i < command_message_index; i++){
              command_buffer[i] = '\0'; // Clear the command buffer
            }
            command_message_index = 0;
            break; // We have finished parsing our id. Up to 32 characters.
          }
          // Check if we have hit the UUID escape character.
          if (uuid_character == COMMAND_END_CHARACTER) {
            // We did not get a UUID. clear out the UUID buffer
            for(int i = 0; i < command_message_index; i++){
              query_uuid_buffer[i] = '\0'; // Clear the command buffer
            }
            // This is a command without a UUID, process the command, no need to send a result with a UUID.
            return process_serial_command();
          }
          
          // Process the UUID
          if (command_message_index < UUID_SIZE) {
            query_uuid_buffer[command_message_index] = uuid_character;
            command_buffer[command_message_index] = uuid_character;
            command_message_index++;
          } else {
            break;
          }
        }
        continue;
      }
    } else {
      // Here we are parsing a command.
      // Check if the current character is the "COMMAND_END_CHARACTER"
      if(command_character == COMMAND_END_CHARACTER){
        break;
      }
      // We received the start character
      command_buffer[command_message_index] = command_character;
      command_message_index++;
      // Safety check that we have not received a command that is too big.
      if(command_message_index > COMMAND_BUFFER_SIZE){
        // Send a message to indicate that there was an error pasting the command because the command string was too long.
        char message[128 + COMMAND_BUFFER_SIZE] = {'\0'};
        sprintf(message, "\"error\":true,\"msg\":\"Error: Command '%s' was too long to be parsed, because it exceeded the maximum character limit of %i\"", command_buffer, COMMAND_BUFFER_SIZE);
        send_query_response(message);
        // Reset our parsing flags
        parsing_command = false;
        break;
      }
    }
  }
  // Any Components which define messages with a "QUERY" interface will have their query code inserted below.
  // Decode which command the user indicated
  if(parsing_command){
    process_serial_command();
  }
}