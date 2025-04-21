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
#define EIXO_X 26                   // Eixo X do JoyStick ADC 0
#define EIXO_Y 27                   // Eixo Y do JoyStick ADC 1            

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
bool cor = true;

int pulso_x;
int pulso_y;
uint16_t adc_value_x;
uint16_t adc_value_y; 

// --- DECLARAÇÃO DE FUNÇÕES

void irq_buttons(uint gpio, uint32_t events);
void converte_joystic(int input);
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

    int x = 63, y = 31; // Valor central do ssd1306

    while (1) {
        converte_joystic(0);
        converte_joystic(1);
        
        adc_select_input(0);
        uint16_t raw = adc_read();
        potencia = (raw * 100) / 4095; // Escala para 0–100%
        
        ssd1306_fill(&ssd, !cor); // Limpa o display

        // Eixo X
        if(adc_value_y == 2048){ // Se estiver no meio 
            x = 63;
        }
        else if (adc_value_y > 2048){ // Se aumentar os valores
            x = ((adc_value_y - 2048) / 32) + 54;
        }
        else{ // Se diminuir os valores
            x = (((adc_value_y - 2048)) / 32) + 67;
        }
        
        // Eixo Y
        if(adc_value_x == 2048)
            y = 31;
        else if(adc_value_x > 2048){
            y =  67 - (adc_value_x / 64);
        }
        else{
            y = 54 - ((adc_value_x) / 64);
        }
        
        ssd1306_rect(&ssd, y, x, 8, 8, cor, cor); // Desenha um retângulo        
        ssd1306_send_data(&ssd); // Atualiza o display
    
        printf("------------------------------------\n");
        printf("Painel de controle da Nave XS9000\n");
        printf("Motor: %s\n", state ? "ON" : "OFF");
        printf("Potencia: %d %\n", potencia);
        printf("Alerta: %s\n", alert ? "ON" : "OFF");
        
        sleep_ms(100);
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
 * @brief Função para ler os valores dos eixos do joystick (X e Y) e e converter valores negativos.
 * 
 * @param input entrada.
 */
void converte_joystic (int input) { 
    // Seleciona 0 ou 1
    adc_select_input(input);
    sleep_us(2);
    if(input == 0){
        adc_value_x = adc_read(); 
        pulso_x = ((adc_value_x -2048)*255)/2048;
        if (pulso_x < 0){
            pulso_x = pulso_x *(-1);
        }
    }
    else if (input == 1){        
        adc_value_y = adc_read();
        pulso_y = ((adc_value_y -2048)*255)/2048;
        if (pulso_y < 0){
            pulso_y = pulso_y *(-1);
        }
    }
    else{
        printf("Valor Invalido!\n");
    }
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

    // Configura ADC para os eixos do joystick
    adc_init();
    adc_gpio_init(EIXO_X);
    adc_gpio_init(EIXO_Y);
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
