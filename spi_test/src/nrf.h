#undef STATIC_ASSERT
#undef SS
#undef SCK
#undef MISO
#undef MOSI
#undef CHG

#define SPI0_ENABLED 1
#define SPI0_USE_EASY_DMA 1

#include "nrf52840_peripherals.h"
// #include "nrfx.h"
// #include "nrfx_timer.h"
#include "nrf_timer.h"

#include "nrfx_spim.h"
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


// Particle defines clash with nRF SDK
