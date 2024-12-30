#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// Skytek API 
#include "skytek/skytek.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// Define the GPIO pin to which the NeoPixel data line is connected
#define NEOPIXEL_PIN 17

// todo get free sm
PIO pio;
uint sm;
uint offset;

bool is_red = false;

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) |
         ((uint32_t)(g) << 16) |
         (uint32_t)(b);
}

void test() {   
    if (is_red) {
        put_pixel(pio, sm, urgb_u32(0x00, 0xFF, 0x00)); // Green
    } else {
        put_pixel(pio, sm, urgb_u32(0xFF, 0x00, 0x00)); // Red
    }
    is_red = !is_red;
}

int main() {
    stdio_init_all();

    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, NEOPIXEL_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, NEOPIXEL_PIN, 800000, false);

    put_pixel(pio, sm, urgb_u32(0x00, 0xFF, 0x00)); // Blue

    // // SPI initialisation. This example will use SPI at 1MHz.
    // spi_init(SPI_PORT, 1000*1000);
    // gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    // gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    // gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // // Chip select is active-low, so we'll initialise it to a driven-high state
    // gpio_set_dir(PIN_CS, GPIO_OUT);
    // gpio_put(PIN_CS, 1);

    // Lets blink the LED
    // gpio_set_dir(, GPIO_OUT);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    // // Set up our UART
    // uart_init(UART_ID, BAUD_RATE);
    // // Set the TX and RX pins by using the function select on the GPIO
    // // Set datasheet for more information on function select
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // // Use some the various UART functions to send out data
    // // In a default system, printf will also output via the default UART
    
    // // Send out a string, with CR/LF conversions
    // uart_puts(UART_ID, " Hello, UART!\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    skytek_init();

    // Insert some key-value pairs
    int value1 = 42;
    double value2 = 3.14;
    char *value3 = "Hello, world!";

    insert("key1", &value1);
    insert("key2", &value2);
    insert("key3", value3);
    insert("key4", &test);

    while (true) {
        // // Retrieve and print the values
        // int *retrievedInt = (int *)find("key1");
        // double *retrievedDouble = (double *)find("key2");
        // char *retrievedString = (char *)find("key3");
        // char *retrievedCallbackPointer = (void *)find("key4");

        // if (retrievedInt) printf("key1: %d\n", *retrievedInt);
        // if (retrievedDouble) printf("key2: %.2f\n", *retrievedDouble);
        // if (retrievedString) printf("key3: %s\n", retrievedString);
        // if (retrievedCallbackPointer) {
        //     void (*callback_func)() = (void (*)())retrievedCallbackPointer;
        //     callback_func();
        // }

        skytek_update();
    }

    // Free the hash table
    freeTable();
}