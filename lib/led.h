#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "pico/stdlib.h"

// Inicializa os Leds
void setup_led(uint pin);

// Alterna o estado do LED no pino informado.
void toggle_led(uint pin);

// Define o estado do LED no pino informado.
void set_led(uint pin, bool state);

// Retorna o estado atual do LED no pino informado.
bool get_led_state(uint pin);

#endif // LEDS_H
