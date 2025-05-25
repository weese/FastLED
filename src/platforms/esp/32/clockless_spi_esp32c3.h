#ifndef __INC_CLOCKLESS_ARM_ESP32C3_H
#define __INC_CLOCKLESS_ARM_ESP32C3_H

#include <SPI.h>
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_err.h"

FASTLED_NAMESPACE_BEGIN
// Definition for a single channel clockless controller for the Particle family of chips, like that used in the spark core
// See clockless.h for detailed info on how the template parameters are used.

#define FASTLED_HAS_CLOCKLESS 1

#ifndef FASTLED_ESP32C3_MAXIMUM_PIXELS_PER_STRING
#define FASTLED_ESP32C3_MAXIMUM_PIXELS_PER_STRING 256
#endif


template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER = RGB, int XTRA0 = 0, bool FLIP = false, int WAIT_TIME = 9>
class ClocklessController : public CPixelLEDController<RGB_ORDER> {
    typedef typename FastPin<DATA_PIN>::port_ptr_t data_ptr_t;
    typedef typename FastPin<DATA_PIN>::port_t data_t;

    static CMinWait<WAIT_TIME> mWait;

private:
    static const uint32_t SPI_CLOCK_MHZ       = 4000000; // 4 MHz
    static const uint16_t DIVIDER             = F_CPU / SPI_CLOCK_MHZ;

    static const uint8_t  T0H = (uint16_t)((T1       + DIVIDER / 2) / DIVIDER);
    static const uint8_t  T1H = (uint16_t)((T1+T2    + DIVIDER / 2) / DIVIDER);
    static const uint8_t  TOP = (uint16_t)((T1+T2+T3 + DIVIDER / 2) / DIVIDER);

    static const uint8_t  BITS_PER_PIXEL   = (8 + XTRA0) * 3; // NOTE: 3 means RGB only...
    static const uint16_t PWM_BUFFER_SIZE = (uint16_t)((TOP * BITS_PER_PIXEL * FASTLED_ESP32C3_MAXIMUM_PIXELS_PER_STRING + 7) / 8) + 1;

    // may as well be static, as can only attach one LED string per DATA_PIN....
    static uint8_t mBuffer[PWM_BUFFER_SIZE];
    static uint8_t *mLast;
    static uint8_t mLastBit;
    
    static spi_transaction_t transaction;
    static spi_device_handle_t spi;

public:
    virtual void init() {
        spi_bus_config_t buscfg = {
            .mosi_io_num = DATA_PIN,
            .miso_io_num = GPIO_NUM_NC,
            .sclk_io_num = GPIO_NUM_NC,
            .quadwp_io_num = GPIO_NUM_NC,
            .quadhd_io_num = GPIO_NUM_NC,
            .data4_io_num = GPIO_NUM_NC,
            .data5_io_num = GPIO_NUM_NC,
            .data6_io_num = GPIO_NUM_NC,
            .data7_io_num = GPIO_NUM_NC,
            .max_transfer_sz = 4096,
            .flags = SPICOMMON_BUSFLAG_MOSI,
        };

        // Initialize SPI bus (SPI2_HOST is the only usable SPI on ESP32-C3)
        esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
        if (ret != ESP_OK) {
            Serial.println("Failed to init SPI bus");
            return;
        }

        GPSPI2.ctrl.d_pol = 0; // Set hold polarity to low

        spi_device_interface_config_t devcfg = {
            .command_bits = 0,
            .address_bits = 0,
            .dummy_bits = 0,
            .mode = 0,
            .clock_speed_hz = SPI_CLOCK_MHZ,
            .spics_io_num = GPIO_NUM_NC,
            .flags = SPI_DEVICE_NO_DUMMY,
            .queue_size = 1,
        };
        ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
        if (ret != ESP_OK) {
            Serial.println("Failed to add SPI device");
            return;
        }
        
        // send dummy packet to set MOSI to low
        mBuffer[0] = 0;
        transaction.length = 1;
        transaction.tx_buffer = mBuffer;
        ret = spi_device_queue_trans(spi, &transaction, 100);
        if (ret != ESP_OK) {
            Serial.printf("Queue failed: %d\n", ret);
        }
    }

  	virtual uint16_t getMaxRefreshRate() const { return 400; }

protected:
    virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
        mWait.wait();
        showRGBInternal(pixels);
    }

    template<int BITS> __attribute__ ((always_inline)) inline static void writeBits(register uint8_t & b)  {
        for(FASTLED_REGISTER uint32_t i = BITS; i != 0; --i) {
            *mLast |= 0xff >> mLastBit;
            if (b & 0x80) {
                mLastBit += T1H;
                while (mLastBit >= 8) {
                    *++mLast = 0xff;
                    mLastBit -= 8;
                }
                *mLast &= ~(0xff >> mLastBit);
                mLastBit += TOP - T1H;
            } else {
                mLastBit += T0H;
                while (mLastBit >= 8) {
                    *++mLast = 0xff;
                    mLastBit -= 8;
                }
                *mLast &= ~(0xff >> mLastBit);
                mLastBit += TOP - T0H;
            }
            while (mLastBit >= 8) {
                *++mLast = 0;
                mLastBit -= 8;
            }
            b <<= 1;
        }
    }

    // This method is made static to force making register Y available to use for data on AVR - if the method is non-static, then
    // gcc will use register Y for the this pointer.
    static void showRGBInternal(PixelController<RGB_ORDER> pixels) {
        mLast = mBuffer;
        mLastBit = 0;

    #ifdef ASSERT
        ASSERT(!pixels.has(FASTLED_ESP32C3_MAXIMUM_PIXELS_PER_STRING + 1));
    #endif
        // Setup the pixel controller and load/scale the first byte
        pixels.preStepFirstByteDithering();
        FASTLED_REGISTER uint8_t b = pixels.loadAndScale0();

        spi_transaction_t* rtrans;
        spi_device_get_trans_result(spi, &rtrans, 100);
        
        while(pixels.has(1)) {
            pixels.stepDithering();

            // Write first byte, read next byte
            writeBits<8+XTRA0>(b);
            b = pixels.loadAndScale1();

            // Write second byte, read 3rd byte
            writeBits<8+XTRA0>(b);
            b = pixels.loadAndScale2();

            // Write third byte, read 1st byte of next pixel
            writeBits<8+XTRA0>(b);
            b = pixels.advanceAndLoadAndScale0();
        };

        transaction.length = (mLast - mBuffer) * 8u + mLastBit;
        transaction.tx_buffer = mBuffer;
        esp_err_t ret = spi_device_queue_trans(spi, &transaction, 100);
        if (ret != ESP_OK) {
            Serial.printf("Queue failed: %d\n", ret);
        }
    }
};

template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
CMinWait<WAIT_TIME> ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mWait;
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
uint8_t ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mBuffer[PWM_BUFFER_SIZE] = {0};
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
uint8_t* ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mLast = NULL;
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
uint8_t ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mLastBit = 0;
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
spi_device_handle_t ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::spi = NULL;
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
spi_transaction_t ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::transaction = {};

#ifndef FASTLED_WS2812_T1
#define FASTLED_WS2812_T1 250
#endif

#ifndef FASTLED_WS2812_T2
#define FASTLED_WS2812_T2 625
#endif

#ifndef FASTLED_WS2812_T3
#define FASTLED_WS2812_T3 375
#endif

FASTLED_NAMESPACE_END

#endif
