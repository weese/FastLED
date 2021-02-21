// Read:
// https://devzone.nordicsemi.com/f/nordic-q-a/19891/transfer-more-than-255-bytes-with-spi-master-on-nrf52
// https://devzone.nordicsemi.com/f/nordic-q-a/14193/confusion-with-list-in-easydma
// https://devzone.nordicsemi.com/f/nordic-q-a/44849/best-practices-to-nrf_drv_spi_transfer-sequentially
// 
// This code is based on https://gist.github.com/alessandro-montanari/1708d549be50ee37e62dfc5f493b3fa9

// #define PARTICLE_NO_ARDUINO_COMPATIBILITY

#include "nrf.h"

SerialLogHandler logHandler;

#define SPI1_SCK_PIN D2
#define SPI1_MOSI_PIN D3
#define SPI1_MISO_PIN D4

#undef NRF_LOG_INFO
#define NRF_LOG_INFO Serial.printf

#define BUFF_LENGTH 3840        /**< Transfer length. */

// corresponds to SPI1
// static const nrfx_spim_t m_spim2 = NRFX_SPIM_INSTANCE(2);
// const nrfx_spim_t *spim = &m_spim2;

static uint8_t m_tx_buf[BUFF_LENGTH];  /**< TX buffer. */

static volatile bool spi_xfer_in_progress = false;  /**< Flag used to indicate that SPI instance completed the transfer. */

void spi_dma_user_callback() {
}

// void spi_event_handler(nrfx_spim_evt_t const * p_event, void * p_context)
// {
//     if(p_event->type == NRFX_SPIM_EVENT_DONE)
//     {
//     	spi_xfer_in_progress = false;
//     }
//     else
//     {
//         NRF_LOG_DEBUG("Wrong Event\n");
//         // Something is wrong
//     }
// }

// template <HAL_SPI_Interface Interface>
// HAL_SPI_Interface getInterface(SpiProxy<Interface> const &) {
//     return Interface;
// }

void burst_write(uint8_t reg_addr, uint8_t *reg_data, uint32_t cnt)
{
    for (int i = 0; i<BUFF_LENGTH;++i)
        m_tx_buf[i] = 0x7e;
    m_tx_buf[0] = 0x81;
    m_tx_buf[BUFF_LENGTH-1] = 0x20;

    SPI1.transfer(m_tx_buf, NULL, BUFF_LENGTH, spi_dma_user_callback);

    // nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TRX(m_tx_buf, BUFF_LENGTH, NULL, 0);
    // uint32_t flags = NRF_DRV_SPI_FLAG_NO_XFER_EVT_HANDLER;
    // nrfx_spim_xfer(spim, &xfer, flags);  
    // spi_xfer_in_progress = true;
}

int8_t spi_init(void)
{
    SPI1.begin(SPI_MODE_MASTER);
    SPI1.setClockSpeed(4, MHZ);
    SPI1.setDataMode(SPI_MODE3);
    SPI1.setBitOrder(MSBFIRST);

    // nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
    // spi_config.ss_pin    = NRFX_SPIM_PIN_NOT_USED;		//I control the CS pin 
    // spi_config.miso_pin  = NRFX_SPIM_PIN_NOT_USED;
    // spi_config.mosi_pin  = get_nrf_pin_num(SPI1_MOSI_PIN);
    // spi_config.sck_pin   = get_nrf_pin_num(SPI1_SCK_PIN);
    // spi_config.mode      = NRF_SPIM_MODE_3;
    // spi_config.frequency = NRF_SPIM_FREQ_4M;
    // APP_ERROR_CHECK(nrfx_spim_init(spim, &spi_config, spi_event_handler, NULL));
    NRF_LOG_INFO("SPI1 Master initialised.\r\n");

    // spi_xfer_in_progress = false; 

    return 0;
}

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {

    Serial.begin(115200);
    delay(1400);
    Serial.println("check1");

    spi_init();
}

void loop() {
        // Wait for an event.
        __WFE();

        // Clear the event register.
        __SEV();
        __WFE();
        // Serial.print('.');
        // delay(300);
            burst_write(0x55, NULL, 10);
}
