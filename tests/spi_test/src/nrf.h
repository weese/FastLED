#undef STATIC_ASSERT
#undef SS
#undef SCK
#undef MISO
#undef MOSI
#undef CHG

// #define SPI0_ENABLED 1
#define SPI0_USE_EASY_DMA 1

#include "nrf52840_peripherals.h"
// #include "nrfx.h"
// #include "nrfx_timer.h"
#include "nrf_timer.h"

#include "nrfx_spim.h"
#include "nrfx_spis.h"
#include "nrf_drv_timer.h"
// #include "nrfx_timer.h"
// #include "nrfx_pwm.h"
// #include "nrf_pwm.h"

// #include "app_util_platform.h"
#include "app_error.h"
// #include "nrfx_clock.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_drv_spi.h"
#include "nrf_drv_ppi.h"
#include "spi_hal.h"

#include "concurrent_hal.h"

// Particle defines clash with nRF SDK

inline uint8_t get_nrf_pin_num(uint8_t pin) {
    if (pin >= TOTAL_PINS) {
        return NRFX_SPIM_PIN_NOT_USED;
    }

    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();
    return NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
}
