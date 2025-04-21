#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "lib/ssd1306.h"
#include "lib/led.h"
#include "lib/WS2812.h"
#include "WS2812.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define WS2812_PIN 7

#define BTN_A 5                     // Pino do botão A conectado ao GPIO 5.
#define BTN_B 6                     // Pino do botão B conectado ao GPIO 6.
#define BTN_STICK 22                // Pino do botão do Joystick conectado ao GPIO 22.
#define JOYSTICK 26               

#define DEBOUNCE_TIME 200000        // Tempo para debounce em 200 ms 
static uint32_t last_time_A = 0;    // Tempo da última interrupção do botão A
static uint32_t last_time_B = 0;    // Tempo da última interrupção do botão B

#define LED_R 13
#define LED_G 11
#define LED_B 12

#define BUZZER_PIN 10               // Pino do Buzzer conectado ao GPIO 10.

// --- VARIAVEIS GLOBAIS

bool state, alert = 0;
int potencia = 0;
ssd1306_t ssd;
PIO pio = pio0;
uint sm = 0;

// --- DECLARAÇÃO DE FUNÇÕES

void write_display(ssd1306_t *ssd);
void irq_buttons(uint gpio, uint32_t events);
void beep_buzzer();
void buzzer_tone(int frequency);
void buzzer_off();
void setup();
void setup_display();
void setup_buzzer();
void setup_button(uint pin);
void setup_led(uint pin);


/**
 * @brief Função principal
*/
int main() {
    setup();

    clear_matrix(pio, sm);

    beep_buzzer();
    beep_buzzer();
    beep_buzzer();

    set_led_matrix(17, pio, sm);

    while (1) {
        adc_select_input(0); // ADC0
        uint16_t raw = adc_read();
        potencia = (raw * 100) / 4095; // Escala para 0–100%
        
        write_display(&ssd);
        
        printf("------------------------------------\n");
        printf("Motor: %s\n", state ? "ON" : "OFF");
        printf("Potencia: %d %\n", potencia);
        printf("Alerta: %s\n", alert ? "ON" : "OFF");
        
        sleep_ms(1000);
    }
}


/**
 * @brief Função de interrupção para os botões com debouncing.
 * 
 * @param gpio a GPIO que gerou interrupção.
 * @param events a evento que gerou interrupção.
 */
void irq_buttons(uint gpio, uint32_t events){
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (gpio == BTN_A) {
        if (current_time - last_time_A > DEBOUNCE_TIME) {
            state = !state;
            set_led(LED_G, state);
            last_time_A = current_time;
        }
    }

    else if (gpio == BTN_B) {
        if (current_time - last_time_B > DEBOUNCE_TIME) {
            alert = !alert;
            set_led(LED_R, state);
            last_time_B = current_time;

            if (alert)
                set_led(LED_G, false);
            else
                set_led(LED_G, state);
        }
    }

    else if (gpio == BTN_STICK) {
        reset_usb_boot(0, 0);
    }
}


/**
 * @brief Atualiza o display com o estado dos LEDs.
 *
 * @param ssd Ponteiro para a estrutura do display.
 */
void write_display(ssd1306_t *ssd) {
    // Limpa o display
    ssd1306_fill(ssd, false);

    char msg_motor[20];  // Buffer para armazenar a string formatada
    sprintf(msg_motor, "Motor %s", state ? "ON" : "OFF");
    ssd1306_draw_string(ssd, msg_motor, 0, 0);

    char msg_pot[20];  // Buffer para armazenar a string formatada
    sprintf(msg_pot, "Potencia %d", potencia);
    ssd1306_draw_string(ssd, msg_pot, 0, 15);

    char msg_alert[20];  // Buffer para armazenar a string formatada
    sprintf(msg_alert, "Emergencia %s", alert ? "ON" : "OFF");
    ssd1306_draw_string(ssd, msg_alert, 0, 30);

    // Atualiza o display
    ssd1306_send_data(ssd); 
}


/**
 * @brief Função para tocar um beep no buzzer.
 */
void beep_buzzer() {
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(1000);            
    gpio_put(BUZZER_PIN, 0); 
}


/**
 * @brief Inicialização e configuração geral.
*/
void setup() {
    // Inicializa entradas e saídas
    stdio_init_all();

    // Inicializa o PIO para controlar a matriz de LEDs (WS2812)
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    pio_matrix_program_init(pio, sm, offset, WS2812_PIN);

    // Aguarda a conexão USB
    sleep_ms(2000); 

    // Configura os LEDs RGB
    setup_led(LED_R);
    setup_led(LED_G);
    setup_led(LED_B);

    // Configura botões A e B e botão do joystick
    setup_button(BTN_A);
    setup_button(BTN_B);
    setup_button(BTN_STICK);

    // Configura Buzzer
    gpio_init(BUZZER_PIN); gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    
    // Configura interrupção dos botões
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_RISE, true, &irq_buttons);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_RISE, true, &irq_buttons);
    gpio_set_irq_enabled_with_callback(BTN_STICK, GPIO_IRQ_EDGE_FALL, true, &irq_buttons); 

    // Inicializa I2C com 400 Khz
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line

    // Configura display
    setup_display();

    // Configura ADC
    adc_init();
    adc_gpio_init(JOYSTICK);
}


/**
 * @brief Configura Display ssd1306 via I2C, iniciando com todos os pixels desligados.
*/
void setup_display() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);    
    ssd1306_send_data(&ssd);   
    ssd1306_fill(&ssd, false);
}


/**
 * @brief Configura push button como saída e com pull-up.
 * 
 * @param pin o pino do push button.
 */
void setup_button(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}
