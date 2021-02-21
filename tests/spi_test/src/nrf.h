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


typedef struct {
    const nrfx_spim_t                   *master;
    const nrfx_spis_t                   *slave;
    app_irq_priority_t                  priority;
    uint8_t                             ss_pin;
    uint8_t                             sck_pin;
    uint8_t                             mosi_pin;
    uint8_t                             miso_pin;

    SPI_Mode                            spi_mode;
    uint8_t                             data_mode;
    uint8_t                             bit_order;
    uint32_t                            clock;

    volatile uint8_t                    spi_ss_state;
    void                                *slave_tx_buf;
    void                                *slave_rx_buf;
    uint32_t                            slave_buf_length;

    HAL_SPI_DMA_UserCallback            spi_dma_user_callback;
    HAL_SPI_Select_UserCallback         spi_select_user_callback;

    volatile bool                       enabled;
    volatile bool                       transmitting;
    volatile uint16_t                   transfer_length;

    os_mutex_recursive_t                mutex;
} nrf5x_spi_info_t;

extern nrf5x_spi_info_t m_spi_map[TOTAL_SPI];

// Particle defines clash with nRF SDK
