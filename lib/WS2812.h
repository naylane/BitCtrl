#ifndef WS2812
#define WS2812

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
void set_led_matrix(uint8_t number, PIO pio, uint sm);
void clear_matrix(PIO pio, uint sm);

#endif
