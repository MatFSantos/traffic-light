#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"

#include "lib/ws2812b.h"
#include "lib/pwm.h"
#include "lib/led.h"
#include "lib/push_button.h"
#include "lib/ssd1306.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** definições de pinos */
#define BUZZER 21
#define PWM_DIVISER 20
#define PWM_WRAP 2000 /* aprox 3.5kHz freq */
#define GREEN 0
#define YELLOW 1
#define RED 2

    const float ADC_VREF = 3.3f;
static volatile bool view = true;

_ws2812b *ws;                            /* 5x5 matrix */
ssd1306_t ssd;                           /* OLED display */
static volatile uint32_t last_time = 0;  /* debounce variable */
static volatile bool night_mode = false; /* night_mode variable */
static volatile uint16_t state = GREEN;  /* traffic light state */

/**
 * @brief Initialize the SSD1306 display
 *
 */
void init_display() {
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, I2C_ADDRESS, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}

/**
 * @brief Initialize the all GPIOs that will be used in project
 *      - I2C;
 *      - RGB LED;
 *      - Push button A
 *      - Buzzer;
 */
void init_gpio() {
    /** initialize i2c communication */
    int baudrate = 400 * 1000; // 400kHz baud rate for i2c communication
    i2c_init(I2C_PORT, baudrate);

    // set GPIO pin function to I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL);

    /** initialize RGB LED */
    pwm_init_(PIN_BLUE_LED);
    pwm_setup(PIN_BLUE_LED, PWM_DIVISER, PWM_WRAP);
    pwm_start(PIN_BLUE_LED, 0);

    pwm_init_(PIN_RED_LED);
    pwm_setup(PIN_RED_LED, PWM_DIVISER, PWM_WRAP);
    pwm_start(PIN_RED_LED, 0);

    pwm_init_(PIN_GREEN_LED);
    pwm_setup(PIN_GREEN_LED, PWM_DIVISER, PWM_WRAP);
    pwm_start(PIN_GREEN_LED, 0);

    /** initialize button */
    init_push_button(PIN_BUTTON_A);

    /** initialize buzzer */
    pwm_init_(BUZZER);
    pwm_setup(BUZZER, PWM_DIVISER, PWM_WRAP);
    pwm_start(BUZZER, 0);
}

/**
 * @brief Update the display informations
 */
void update_display() {
    char * str;

    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 0, 0, 128, 64, true, false);
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);
    ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);
    ssd1306_line(&ssd, 3, 25, 123, 25, true);
    if(state == GREEN)
        str = "PODE ATRAVESSAR";
    else if (state == YELLOW)
        str = "ATENCAO";
    else if (state == RED)
        str = "PARE";
    ssd1306_draw_string(&ssd, str, 64 - (strlen(str)*8 / 2), 40);
    ssd1306_send_data(&ssd); // update display
}

/**
 * @brief Task to control traffic light behavior, manipulating the 5x5 matriz
 */
void vTrafficLightTask(){
    PIO pio = pio0;
    ws = init_ws2812b(pio, PIN_WS2812B);

    // variaveis de controle
    uint32_t time_green = 0;
    uint32_t time_yellow = 0;
    uint32_t time_red = 0;
    uint32_t time_night = 0;

    while(1){
        if(!night_mode){ /* modo padrão */
            time_night = 0;
            if(state == GREEN){ /* estado verde do semáforo no modo padrão. fica por 15 segundos em verde*/
                if (time_green == 0)
                    ws2812b_plot(ws, &GREEN_TL);
                if (time_green >= 300){ // 15 segundos
                    state = YELLOW;
                    time_green = 0;
                } else
                    time_green++;
            }
            else if (state == YELLOW) { /* estado amarelo do semáforo no modo padrão. fica por 4 segundos em amarelo*/
                if (time_yellow == 0)
                    ws2812b_plot(ws, &YELLOW_TL);
                if (time_yellow >= 80) { // 4 segundos
                    state = RED;
                    time_yellow = 0;
                } else
                    time_yellow++;
            }
            else { /* estado vermelho do semáforo no modo padrão. fica por 11 segundos em vermelho*/
                if (time_red == 0)
                    ws2812b_plot(ws, &RED_TL);
                if (time_red >= 220) { // 11 segundos
                    state = GREEN;
                    time_red = 0;
                }
                else
                    time_red++;
            }
        } else { /* modo noturno */
            time_green = 0;
            time_yellow = 0;
            time_red = 0;
            if (time_night <= 20) { /* pisca a cada 1 segundo o sinal amarelo*/
                if (time_night == 0)
                    ws2812b_plot(ws, &YELLOW_TL);
                time_night++;
            } else if ( time_night <= 40) { // 1 segundos
                if (time_night == 21)
                    ws2812b_turn_off(ws);
                time_night++;
            } else {
                time_night = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Task to control button mode, that change the mode of the traffic light.
 */
void vButtonTask(){
    while(1){
        if(!gpio_get(PIN_BUTTON_A)){
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            if (current_time - last_time >= 300){ // debounce
                night_mode = !night_mode;
                if (night_mode)
                    state = YELLOW;
                else
                    state = GREEN;
                last_time = current_time;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Task to control beep buzzer, that warn people about traffic light state.
 */
void vBeepTask(){
    bool green_beep = false;
    uint32_t time_green = 0;
    uint32_t time_yellow = 0;
    uint32_t time_red = 0;
    uint32_t time_night = 0;
    while(1) {
        if (!night_mode) { /* modo padrão */
            time_night = 0;
            if (state == GREEN && !green_beep) {
                /* no estado verde, o beep apita por 1 segundo, assim como o led RGB acende em branco por 1 segundo.
                depois de 1 segundo, o beep e o led são desligados*/
                if (time_green < 20){
                    if (time_green == 0){
                        pwm_set_gpio_level(BUZZER, PWM_WRAP/4);
                        pwm_set_gpio_level(PIN_BLUE_LED, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_GREEN_LED, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_RED_LED, PWM_WRAP / 4);
                    }
                    time_green++;
                } else if (time_green >= 20){
                    pwm_set_gpio_level(BUZZER, 0);
                    pwm_set_gpio_level(PIN_BLUE_LED, 0);
                    pwm_set_gpio_level(PIN_GREEN_LED, 0);
                    pwm_set_gpio_level(PIN_RED_LED, 0);
                    green_beep = true;
                    time_green = 0;
                }
            }
            else if (state == YELLOW) {
                /* no estado amarelo, o beep apita intermitentemente por 500 ms e desliga por 500ms,
                assim como o led RGB acende em branco e desliga quando o buzzer desliga.*/
                green_beep = false;
                if (time_yellow < 10){
                    if (time_yellow == 0){
                        pwm_set_gpio_level(BUZZER, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_BLUE_LED, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_GREEN_LED, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_RED_LED, PWM_WRAP / 4);
                    }
                    time_yellow++;
                }
                else if (time_yellow < 20){
                    if (time_yellow == 10){
                        pwm_set_gpio_level(BUZZER, 0);
                        pwm_set_gpio_level(PIN_BLUE_LED, 0);
                        pwm_set_gpio_level(PIN_GREEN_LED, 0);
                        pwm_set_gpio_level(PIN_RED_LED, 0);
                    }
                    time_yellow++;
                } else
                    time_yellow = 0;
            }
            else if (state == RED) {
                /* no estado vermelho, o beep apita intermitentemente por 500 ms e desliga por 1.5s,
                assim como o led RGB acende em branco e desliga quando o buzzer desliga.*/
                if (time_red < 10) {
                    if (time_red == 0){
                        pwm_set_gpio_level(BUZZER, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_BLUE_LED, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_GREEN_LED, PWM_WRAP / 4);
                        pwm_set_gpio_level(PIN_RED_LED, PWM_WRAP / 4);
                    }
                    time_red++;
                }
                else if (time_red < 30) {
                    if (time_red == 10){
                        pwm_set_gpio_level(BUZZER, 0);
                        pwm_set_gpio_level(PIN_BLUE_LED, 0);
                        pwm_set_gpio_level(PIN_GREEN_LED, 0);
                        pwm_set_gpio_level(PIN_RED_LED, 0);
                    }
                    time_red++;
                }
                else
                    time_red = 0;
            }
        } else { /* modo noturno */
            time_green = 0;
            time_yellow = 0;
            time_red = 0;
            green_beep = false;
            if (time_night < 10) {
                if (time_night == 0){
                    pwm_set_gpio_level(BUZZER, PWM_WRAP / 4);
                    pwm_set_gpio_level(PIN_BLUE_LED, PWM_WRAP / 4);
                    pwm_set_gpio_level(PIN_GREEN_LED, PWM_WRAP / 4);
                    pwm_set_gpio_level(PIN_RED_LED, PWM_WRAP / 4);
                }
                time_night++;
            }
            else if (time_night < 40) {
                if (time_night == 10){
                    pwm_set_gpio_level(BUZZER, 0);
                    pwm_set_gpio_level(PIN_BLUE_LED, 0);
                    pwm_set_gpio_level(PIN_GREEN_LED, 0);
                    pwm_set_gpio_level(PIN_RED_LED, 0);
                }
                time_night++;
            }
            else
                time_night = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Task to control the OLED display, to show informations about traffic light state
 */
void vDisplayTask(){
    init_display();
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    while(1){
        update_display();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int main() {
    // set clock freq to 128MHz
    bool ok = set_sys_clock_khz(128000, false);
    if (ok)
        printf("clock set to %ld\n", clock_get_hz(clk_sys));

    // init gpios and stdio functions
    stdio_init_all();
    init_gpio();

    sleep_ms(50);
    printf("Programm start...\n");
    xTaskCreate(vTrafficLightTask, "Traffic light control", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vButtonTask, "Button mode control", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBeepTask, "Beep buzzer control", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Display OLED control", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();

    return 0;
}