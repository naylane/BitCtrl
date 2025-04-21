#include "led.h"

void setup_led(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

void toggle_led(uint pin) {
    bool current_state = gpio_get(pin);
    gpio_put(pin, !current_state);
}

void set_led(uint pin, bool state) {
    gpio_put(pin, state);
}

bool get_led_state(uint pin) {
    return gpio_get(pin);
}
