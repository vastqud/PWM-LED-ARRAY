#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include "hardware/irq.h"
#include "hardware/pwm.h"

uint LED_PIN = 25;
uint BUTTON_PIN = 16;
uint PWM_CONTROL = 0;
uint display_pins[4] = {13, 12, 11, 10};
uint display_blank = 15;

uint32_t timer = 0;
int delayTime = 300; 

int MODE = 1;
int LASTMODE = 0;
uint slice_num;

const uint DOT_PERIOD_MS = 250;
const char *morse_letters[] = {
        ".-",    // A
        "-...",  // B
        "-.-.",  // C
        "-..",   // D
        ".",     // E
        "..-.",  // F
        "--.",   // G
        "....",  // H
        "..",    // I
        ".---",  // J
        "-.-",   // K
        ".-..",  // L
        "--",    // M
        "-.",    // N
        "---",   // O
        ".--.",  // P
        "--.-",  // Q
        ".-.",   // R
        "...",   // S
        "-",     // T
        "..-",   // U
        "...-",  // V
        ".--",   // W
        "-..-",  // X
        "-.--",  // Y
        "--.."   // Z
};

void put_morse_letter(uint led_pin, const char *pattern) {
    for (; *pattern; ++pattern) {
        if (MODE != 3) {
            break;
        }

        gpio_put(led_pin, 1);
        if (*pattern == '.')
            sleep_ms(DOT_PERIOD_MS);
        else
            sleep_ms(DOT_PERIOD_MS * 3);
        gpio_put(led_pin, 0);
        sleep_ms(DOT_PERIOD_MS * 1);
    }
    if (MODE != 3) {
        return;
    }

    sleep_ms(DOT_PERIOD_MS * 2);
}

void put_morse_str(uint led_pin, const char *str) {
    for (; *str; ++str) {
        if (*str >= 'A' && *str <= 'Z') {
            put_morse_letter(led_pin, morse_letters[*str - 'A']);
        } else if (*str >= 'a' && *str <= 'z') {
            put_morse_letter(led_pin, morse_letters[*str - 'a']);
        } else if (*str == ' ') {
            sleep_ms(DOT_PERIOD_MS * 4);
        }

        if (MODE != 3) {
            return;
        }
    }
}

void intToBCD(int num, int bcdArray[]) {
    int i;

    for (i = 0; i < 4; i++) {
        bcdArray[i] = num % 2;
        num /= 2;
    }
}

void set_divider() {
    float divider = MODE*2;
    pwm_set_clkdiv(slice_num, 14.f);
}

void update_display() {
    int bcd[4];
    intToBCD(MODE, bcd);

    int i;
    for (i = 0; i < 4; i++) {
        gpio_put(display_pins[i], bcd[i]);
    }
}

void disable_pwm() {
    pwm_set_enabled(slice_num, false);
    gpio_set_function(PWM_CONTROL, GPIO_FUNC_SIO);
    gpio_set_dir(PWM_CONTROL, GPIO_OUT);
    gpio_put(PWM_CONTROL, 0);
}

void enable_pwm() {
    pwm_set_enabled(slice_num, true);
    gpio_set_function(PWM_CONTROL, GPIO_FUNC_PWM);
    set_divider();
}

void update_mode() {
    if (MODE == 0) { //off
        gpio_put(display_blank, 0); //disable display
        disable_pwm();
    } else if (MODE == 1) { //solid
        gpio_put(display_blank, 1); //turn display back on
        disable_pwm();
        gpio_put(PWM_CONTROL, 1);
        update_display();
    } else if (MODE == 2) { //breathing
        gpio_put(display_blank, 1);
        enable_pwm();
        update_display();
    } else if (MODE == 3) { //morse
        gpio_put(display_blank, 1);
        disable_pwm();
        update_display();
    }

    LASTMODE = MODE;
}

void debounce(uint gpio, uint32_t events) {
    if ((to_ms_since_boot(get_absolute_time())-timer)>delayTime) {
        timer = to_ms_since_boot(get_absolute_time());

        MODE++;
        if (MODE > 3) {
            MODE = 0;
        }

        update_mode();
    }
}

void on_pwm_wrap() {
    static int fade = 0;
    static bool going_up = true;

    pwm_clear_irq(pwm_gpio_to_slice_num(PWM_CONTROL));

    if (going_up) {
        ++fade;
        if (fade > 255) {
            fade = 255;
            going_up = false;
        }
    } else {
        --fade;
        if (fade < 0) {
            fade = 0;
            going_up = true;
        }
    }

    pwm_set_gpio_level(PWM_CONTROL, fade * fade);
}

int main() {
    //-------------------------------------------------initialize
    timer = to_ms_since_boot(get_absolute_time());
    stdio_init_all();

    //-------------------------------------------------setup onboard led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    //-------------------------------------------------setup display pins
    int i;
    for (i = 0; i < 4; i++) {
        gpio_init(display_pins[i]);
        gpio_set_dir(display_pins[i], GPIO_OUT);
        gpio_put(display_pins[i], 0);
    }

    gpio_init(display_blank);
    gpio_set_dir(display_blank, GPIO_OUT);
    gpio_put(display_blank, 0);

    //--------------------------------------------------setup button
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &debounce);

    //--------------------------------------------------PWM setup
    slice_num = pwm_gpio_to_slice_num(PWM_CONTROL);
    pwm_config config = pwm_get_default_config();
    gpio_set_dir(PWM_CONTROL, GPIO_OUT);

    pwm_set_irq_enabled(slice_num, true);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    pwm_init(slice_num, &config, true);

    update_mode();

    while(1) {
        sleep_ms(1);
        if (MODE == 3) {
            put_morse_str(PWM_CONTROL, "Hello world");
            sleep_ms(3000);
        }
    }
}