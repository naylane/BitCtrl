#include "WS2812.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "WS2812.pio.h"

#define WS2812_PIN 7

volatile uint8_t current_number = 0;  // Número atual exibido
volatile bool update_num_matrix = false; // Flag utilizada para atualizar a matriz

// Buffer para números de zero a nove.
const uint32_t numbers[][25] = {
    // Número 0
    0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 1, 1, 0,
    
    // 1
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 1, 1, 0, 0,
    0, 0, 1, 0, 0,
    
    // 2
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,

    // 3
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,

    // 4
    0, 1, 0, 0, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,

    // 5
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 0,

    // 6
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 0,
    
    // 7
    0, 0, 0, 1, 0,
    0, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,

    // 8
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,

    // 9
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,

    // rosto feliz
    0, 1, 1, 1, 0,
    1, 0, 0, 0, 1,
    0, 0, 0, 0, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,

    // rosto triste
    1, 0, 0, 0, 1,
    0, 1, 1, 1, 0,
    0, 0, 0, 0, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 

    // 12 - rosto normal
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,

    // simbolo de adição
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    1, 0, 1, 0, 1,
    0, 1, 1, 1, 0,
    0, 0, 1, 0, 0,

    // simbolo de subtração
    0, 0, 1, 0, 0,
    0, 1, 1, 1, 0,
    1, 0, 1, 0, 1,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,

    // 15 - exclamação
    0, 0, 1, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 1, 1, 1, 0,
    1, 1, 1, 1, 1,

    // 16 - E
    1, 1, 1, 1, 0,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 0,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 0,

    // 17 - nave
    0, 0, 1, 0, 0,
    0, 1, 1, 1, 0,
    1, 1, 0, 1, 1,
    0, 1, 1, 1, 0,
    0, 0, 1, 0, 0
};

/**
 * @brief Transforma a cor RGB em um inteiro de 32 bits sem sinal.
 * 
 * @param r cor vermelha
 * @param g cor verde
 * @param b cor azul
 * 
 * @return Inteiro de 32 bits sem sinal.
 */
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}


/**
 * @brief Atualiza a matriz de LEDs com o número especificado
 * @param current_number Número a ser exibido (0-9)
 * 
 * @details Envia os dados para a matriz LED WS2812 usando PIO
 */
void set_led_matrix(uint8_t current_number, PIO pio, uint sm) {
    // Calcula a cor utilizando a função auxiliar baseada em matrix_rgb
    //uint32_t color = get_number_color(current_number);
    uint32_t color = urgb_u32(20, 2, 10); // Cor rosa

    for (int i = 0; i < 25; i++) {
        if (numbers[current_number][i]) {
            pio_sm_put_blocking(pio, sm, color<<8u); // Liga o LED com 1 no buffer
        } else {
            pio_sm_put_blocking(pio, sm, 0); // Desliga os LEDs com 0 no buffer
        }
    }
    sleep_us(50);
}


/**
 * @brief Apaga todos os LEDs da matriz.
 * 
 * @param pio Instância do PIO utilizada.
 * @param sm Número da state machine.
 */
void clear_matrix(PIO pio, uint sm) {
    for (int i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio, sm, 0);
    }
    sleep_us(50);
}