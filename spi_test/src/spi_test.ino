// #define PARTICLE_NO_ARDUINO_COMPATIBILITY

#include "nrf.h"

SerialLogHandler logHandler;

#define SPI1_SS_PIN D5
#define SPI1_SCK_PIN D4
#define SPI1_MISO_PIN D6
#define SPI1_MOSI_PIN D7

#undef NRF_LOG_INFO
#define NRF_LOG_INFO Serial.printf

#define SPI_INSTANCE  0 /**< SPI instance index. */
#define BUFF_LENGTH 2000        /**< Transfer length. */

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static uint8_t       m_tx_buf[BUFF_LENGTH];  /**< TX buffer. */
static uint8_t       m_rx_buf[BUFF_LENGTH];   /**< RX buffer. */

static volatile bool burst_completed = false;
static volatile bool spi_xfer_done = false;  /**< Flag used to indicate that SPI instance completed the transfer. */

// PPI resrources
nrf_ppi_channel_t ppi_channel_spi;
nrf_ppi_channel_t ppi_channel_timer;

// Timer for burst read
const nrf_drv_timer_t timer = NRF_DRV_TIMER_INSTANCE(1);

// void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
//     Serial.printf("Error %i in line %i of file %s\n", error_code, line_num, p_file_name);
// }

void imu_select()
{
    nrf_gpio_pin_clear(SPI1_SS_PIN);
}

void imu_deselect()
{
    nrf_gpio_pin_set(SPI1_SS_PIN);
}

void my_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            burst_transfer_disable();
            break;

        default:
            //Do nothing.
            break;
    }
}

static void burst_setup()
{
    uint32_t spi_end_evt;
    uint32_t timer_count_task;
    uint32_t timer_cc_event;
    ret_code_t err_code;       

    //Configure timer
    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.mode = NRF_TIMER_MODE_COUNTER;
    err_code = nrf_drv_timer_init(&timer, &timer_cfg, my_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    // Compare event after 4 transmissions
    nrf_drv_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 4, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    timer_count_task = nrf_drv_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT);
    timer_cc_event = nrf_drv_timer_event_address_get(&timer,  NRF_TIMER_EVENT_COMPARE0);
    NRF_LOG_INFO("timer configured\n");

    // allocate PPI channels for hardware
    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_spi);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_timer);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("ppi allocated\n");

    spi_end_evt = nrf_drv_spi_end_event_get(&spi);

    // Configure the PPI to count the trasnsactions on the TIMER
    err_code = nrf_drv_ppi_channel_assign(ppi_channel_spi, spi_end_evt, timer_count_task);
    APP_ERROR_CHECK(err_code);

    // Configure another PPI to stop the SPI when 4 transactions have been completed
    err_code = nrf_drv_ppi_channel_assign(ppi_channel_timer, timer_cc_event, NRF_SPIM_TASK_STOP);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("ppi configured\n");
}

static void burst_transfer_enable()
{
    ret_code_t err_code;

    burst_completed = false;

    err_code = nrf_drv_ppi_channel_enable(ppi_channel_spi);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(ppi_channel_timer);
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_enable(&timer);

    // Configure short between spi end event and spi start task
    nrf_spim_shorts_enable(spi.u.spim.p_reg, NRF_SPIM_SHORT_END_START_MASK);

    imu_select();
}

static void burst_transfer_disable()
{
    ret_code_t err_code;

    // Configure short between spi end event and spi start task
    nrf_spim_shorts_disable(spi.u.spim.p_reg, NRF_SPIM_SHORT_END_START_MASK);

    err_code = nrf_drv_ppi_channel_disable(ppi_channel_spi);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_disable(ppi_channel_timer);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_timer_disable(&timer);

    imu_deselect();

    burst_completed = true;
}

void spi_event_handler(nrf_drv_spi_evt_t const * p_event, void * p_context)
{
    if(p_event->type == NRF_DRV_SPI_EVENT_DONE)
    {
    	spi_xfer_done = true;
    }
    else
    {
        NRF_LOG_DEBUG("Wrong Event\n");
        // Something is wrong
    }
}

int8_t bmi160_bus_burst_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t cnt)
{
    m_tx_buf[0] = reg_addr;

    nrf_drv_spi_xfer_desc_t xfer = NRF_DRV_SPI_XFER_TRX(m_tx_buf, 1, reg_data, 255);
    uint32_t flags = NRF_DRV_SPI_FLAG_HOLD_XFER |
                     NRF_DRV_SPI_FLAG_REPEATED_XFER |
                     NRF_DRV_SPI_FLAG_RX_POSTINC  |
                     NRF_DRV_SPI_FLAG_NO_XFER_EVT_HANDLER;
    nrf_drv_spi_xfer(&spi, &xfer, flags);  

    burst_transfer_enable();

    nrf_spim_task_trigger(spi.u.spim.p_reg, NRF_SPIM_TASK_START);

    while(!burst_completed){
        __WFE();
    }
    spi_xfer_done = false;
    burst_completed = false;

    return 0;
}

int8_t spi_init(void)
{
    ret_code_t err_code;
    
    nrf_gpio_cfg_output(SPI1_SS_PIN);
    imu_deselect();
    
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("ppi initialised\n");
    
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = NRF_DRV_SPI_PIN_NOT_USED;		//I control the CS pin 
    spi_config.miso_pin = SPI1_MISO_PIN;
    spi_config.mosi_pin = SPI1_MOSI_PIN;
    spi_config.sck_pin  = SPI1_SCK_PIN;                               
    spi_config.frequency    = NRF_DRV_SPI_FREQ_8M;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
    NRF_LOG_INFO("SPI1 Master initialised.\r\n");

    // Reset rx buffer and transfer done flag
    memset(m_rx_buf, 0, BUFF_LENGTH);
    memset(m_tx_buf, 0, BUFF_LENGTH);
    spi_xfer_done = false; 

    // // To read the Device ID (0xD1)
    // uint8_t data = 0;
    // bmi160_bus_read(0, 0x0, &data, 1);
    // NRF_LOG_INFO("DEVICE ID (0xD1): %#01x\r\n", data);

    return 0;
}

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  SPI.lock();
  SPI.begin(SPI_MODE_MASTER);

    digitalWrite(D7, LOW);

    Serial.begin(115200);
    delay(1400);
    Serial.println("check1");

    spi_init();
    burst_setup();
}

void loop() {
        // Wait for an event.
        __WFE();

        // Clear the event register.
        __SEV();
        __WFE();
        Serial.print('.');
        delay(300);
        bmi160_bus_burst_read(0x55, m_rx_buf, 1000);
}
