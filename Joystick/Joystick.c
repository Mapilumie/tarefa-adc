#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

// Definições dos pinos
const int VRX = 26; // Pino do eixo X do joystick (conectado ao ADC)
const int VRY = 27; // Pino do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal do ADC para o VRX
const int ADC_CHANNEL_1 = 1; // Canal do ADC para o VRY
const int SW = 22; // Pino do botão do joystick
const int LED_B = 13; // Pino do LED azul
const int LED_R = 12; // Pino do LED vermelho
const int LED_G = 11; // Pino do LED verde
const int BUTTON_A = 5; // Pino do botão A
const float DIVIDER_PWM = 16.0; // Divisor fracional do clock para o PWM
const uint16_t PERIOD = 4096; // Período do PWM (valor máximo do contador)
const uint16_t CENTER_VALUE = 2048; // Valor central do joystick
const uint16_t THRESHOLD = 100; // Tolerância para considerar o joystick "solto"

// Variáveis globais
uint16_t led_b_level, led_r_level; // Níveis de PWM para os LEDs Azul e Vermelho
uint slice_led_b, slice_led_r; // Slices de PWM dos LEDs Azul e Vermelho
bool led_g_state = false; // Estado do LED Verde (ligado/desligado)
bool pwm_enabled = true; // Estado do PWM (ativo/inativo)

// Protótipos das funções
void setup_joystick();
void setup();
void setup_pwm_led(uint led, uint *slice, uint16_t level);
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value);
void button_irq_handler(uint gpio, uint32_t events);
void button_a_irq_handler(uint gpio, uint32_t events);

int main()
{
    uint16_t vrx_value, vry_value;
    setup();
/*  gpio_put(LED_G, 1);
    sleep_ms(1000);
    gpio_put(LED_G, 0); */

    printf("Joystick - PWM e LEDs RGB\n");

    while (true) {
        joystick_read_axis(&vrx_value, &vry_value);

        // Ajuste do brilho do LED azul (eixo Y)
        if (vry_value < CENTER_VALUE - THRESHOLD) {
            led_b_level = (CENTER_VALUE - vry_value) * (PERIOD / CENTER_VALUE); // Mapeia para 0 a 4096
        } else if (vry_value > CENTER_VALUE + THRESHOLD) {
            led_b_level = (vry_value - CENTER_VALUE) * (PERIOD / CENTER_VALUE); // Mapeia para 0 a 4096
        } else {
            led_b_level = 0; // Joystick solto
        }

        // Ajuste do brilho do LED vermelho (eixo X)
        if (vrx_value < CENTER_VALUE - THRESHOLD) {
            led_r_level = (CENTER_VALUE - vrx_value) * (PERIOD / CENTER_VALUE); // Mapeia para 0 a 4096
        } else if (vrx_value > CENTER_VALUE + THRESHOLD) {
            led_r_level = (vrx_value - CENTER_VALUE) * (PERIOD / CENTER_VALUE); // Mapeia para 0 a 4096
        } else {
            led_r_level = 0; // Joystick solto
        }

        // Atualiza os níveis de PWM dos LEDs Azul e Vermelho
        if (pwm_enabled) {
            pwm_set_gpio_level(LED_B, led_b_level);
            pwm_set_gpio_level(LED_R, led_r_level);
        }

        sleep_ms(100);
    }
}

void setup_joystick() {
    // Inicializa o ADC e os pinos de entrada
    adc_init();
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);

    // Inicializa o pino do botão do joystick
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);

    // Configura interrupção para o botão do joystick
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);

    // Inicializa o botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    // Configura interrupção para o botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_a_irq_handler);
}

void setup_pwm_led(uint led, uint *slice, uint16_t level) {
    gpio_set_function(led, GPIO_FUNC_PWM);
    *slice = pwm_gpio_to_slice_num(led);
    pwm_set_clkdiv(*slice, DIVIDER_PWM);
    pwm_set_wrap(*slice, PERIOD);
    pwm_set_gpio_level(led, level);
    pwm_set_enabled(*slice, true);
}

void setup() {
    stdio_init_all();
    setup_joystick();
    setup_pwm_led(LED_B, &slice_led_b, led_b_level);
    setup_pwm_led(LED_R, &slice_led_r, led_r_level);

    // Inicializa o LED Verde como saída digital
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, led_g_state); // Define o estado inicial do LED Verde
}

void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value) {
    // Leitura do valor do eixo X
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vrx_value = adc_read();

    // Leitura do valor do eixo Y
    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vry_value = adc_read();
}

void button_irq_handler(uint gpio, uint32_t events) {
    /*// Debouncing
    static uint32_t last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time < 200) return;
    last_time = current_time;*/

    // Alterna o estado do LED Verde (ligado/desligado)
    led_g_state = !led_g_state;
    if(led_g_state){
        gpio_put(LED_G, 1);
    } else {
        gpio_put(LED_G, 0);
    }
    //gpio_put(LED_G, led_g_state);
}

void button_a_irq_handler(uint gpio, uint32_t events) {
    // Debouncing
    static uint32_t last_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_time < 200) return;
    last_time = current_time;

    // Ativa ou desativa o PWM dos LEDs Azul e Vermelho
    pwm_enabled = !pwm_enabled;
    if (!pwm_enabled) {
        pwm_set_gpio_level(LED_B, 0);
        pwm_set_gpio_level(LED_R, 0);
    }
}