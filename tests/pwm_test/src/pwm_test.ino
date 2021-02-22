#include <stdio.h>
#include <string.h>

#include "nrf.h"
// #include "app_util_platform.h"
// #include "app_error.h"
// #include "nrfx_clock.h"

// #include "FreeRTOS.h"
// #include "fast_pin.h"


static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);

static void demo3()
{

    /*
     * This demo uses only one channel, which is reflected on LED 1.
     * The LED blinks three times (200 ms on, 200 ms off), then it stays off
     * for one second.
     * This scheme is performed three times before the peripheral is stopped.
     */

    nrfx_pwm_config_t const config0 =
    {
        .output_pins =
        {
            44, // channel 0
            NRFX_PWM_PIN_NOT_USED,             // channel 1
            NRFX_PWM_PIN_NOT_USED,             // channel 2
            NRFX_PWM_PIN_NOT_USED,             // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_16MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = 20,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_AUTO
    };
    nrfx_pwm_init(&m_pwm0, &config0, NULL);

    // This array cannot be allocated on stack (hence "static") and it must
    // be in RAM (hence no "const", though its content is not changed).
    static uint16_t /*const*/ seq_values[] =
    {
        4,
        4,
        4,
    };
    nrf_pwm_sequence_t const seq =
    {
        seq_values,
        NRF_PWM_VALUES_LENGTH(seq_values),
        0,
        0
    };

    (void)nrfx_pwm_simple_playback(&m_pwm0, &seq, 1, NRFX_PWM_FLAG_STOP);
}

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup(void) {
    digitalWrite(D7, LOW);

    Serial.begin(115200);
    Serial.println("check1");
    Serial.flush();

    // nrf_pwm_disable();
    delay(1000);
    demo3();
}


void loop(void)
{
        // Wait for an event.
        __WFE();

        // Clear the event register.
        __SEV();
        __WFE();
        Serial.print('.');
        delay(300);
}
