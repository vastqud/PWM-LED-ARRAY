#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

void on_pwm_wrap() {
    static int fade = 0;
    static bool going_up = true;

    pwm_clear_irq(pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN));

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

    pwm_set_gpio_level(PICO_DEFAULT_LED_PIN, fade * fade);
}

int main() {
    gpio_set_function(PICO_DEFAULT_LED_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN);

    float init_div = 16;
    float switch_div = 4;

    float current_div = init_div;
    int div_flag = 0;

    stdio_init_all();

    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, current_div);
    pwm_init(slice_num, &config, true);

    while (1) {
        sleep_ms(10000);
        div_flag = !div_flag;
        printf("looping %d\n", div_flag);

        switch (div_flag) {
            case 0 :
                current_div = init_div;
                printf("going slow\n");
                break;
            case 1 :
                current_div = switch_div;
                printf("going fast\n");
                break;
            default :
                current_div = init_div;
                printf("going slow  invalid\n");
                break;
        }

        pwm_set_clkdiv(slice_num, current_div);
    }
}
