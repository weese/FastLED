#ifndef __INC_CLOCKLESS_ARM_PARTICLE_H
#define __INC_CLOCKLESS_ARM_PARTICLE_H

FASTLED_NAMESPACE_BEGIN
// Definition for a single channel clockless controller for the Particle family of chips, like that used in the spark core
// See clockless.h for detailed info on how the template parameters are used.

#define FASTLED_HAS_CLOCKLESS 1

#ifndef FASTLED_PARTICLE_CLOCKLESS_SPI
#define FASTLED_PARTICLE_CLOCKLESS_SPI SPI1
#endif

#ifndef FASTLED_PARTICLE_MAXIMUM_PIXELS_PER_STRING
#define FASTLED_PARTICLE_MAXIMUM_PIXELS_PER_STRING 256
#endif

template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER = RGB, int XTRA0 = 0, bool FLIP = false, int WAIT_TIME = 9>
class ClocklessController : public CPixelLEDController<RGB_ORDER> {
    typedef typename FastPin<DATA_PIN>::port_ptr_t data_ptr_t;
    typedef typename FastPin<DATA_PIN>::port_t data_t;

    static CMinWait<WAIT_TIME> mWait;

private:
    static const bool     INITIALIZE_PIN_HIGH = (FLIP ? 1 : 0);
    static const uint16_t POLARITY_BIT        = (FLIP ? 0 : 0x8000);

    static const uint8_t  T0H = ((uint16_t)(T1      ));
    static const uint8_t  T1H = ((uint16_t)(T1+T2   ));
    static const uint8_t  TOP = ((uint16_t)(T1+T2+T3));

    static const uint8_t  BITS_PER_PIXEL   = (8 + XTRA0) * 3; // NOTE: 3 means RGB only...
    static const uint16_t PWM_BUFFER_SIZE = (TOP * BITS_PER_PIXEL * FASTLED_PARTICLE_MAXIMUM_PIXELS_PER_STRING + 7) / 8;

    // may as well be static, as can only attach one LED string per DATA_PIN....
    static uint8_t mBuffer[PWM_BUFFER_SIZE];
    static uint8_t *mLast;
    static uint8_t mLastBit;
    static volatile bool mBusy;

public:
    virtual void init() {
        FASTLED_PARTICLE_CLOCKLESS_SPI.begin(SPI_MODE_MASTER);
        FASTLED_PARTICLE_CLOCKLESS_SPI.setClockSpeed(4, MHZ);
        FASTLED_PARTICLE_CLOCKLESS_SPI.setDataMode(SPI_MODE3);
        FASTLED_PARTICLE_CLOCKLESS_SPI.setBitOrder(MSBFIRST);
        mBusy = false;
    }

  	virtual uint16_t getMaxRefreshRate() const { return 400; }

protected:
    virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
        mWait.wait();
        showRGBInternal(pixels);
    }

    template<int BITS> __attribute__ ((always_inline)) inline static void writeBits(register uint8_t & b)  {
        for(register uint32_t i = BITS; i != 0; --i) {
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

    static void spiDMACallback() { mWait.mark(); mBusy = false; }

    // This method is made static to force making register Y available to use for data on AVR - if the method is non-static, then
    // gcc will use register Y for the this pointer.
    static void showRGBInternal(PixelController<RGB_ORDER> pixels) {
        mLast = mBuffer;
        mLastBit = 0;

    #ifdef ASSERT
        ASSERT(!pixels.has(FASTLED_PARTICLE_MAXIMUM_PIXELS_PER_STRING + 1));
    #endif
        // Setup the pixel controller and load/scale the first byte
        pixels.preStepFirstByteDithering();
        register uint8_t b = pixels.loadAndScale0();

        while (mBusy) {};
        mBusy = true;

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

        // FASTLED_PARTICLE_CLOCKLESS_SPI
        FASTLED_PARTICLE_CLOCKLESS_SPI.transfer(mBuffer, NULL, mLast - mBuffer + ((mLastBit + 7) >> 3), spiDMACallback);
    }
};

template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
CMinWait<WAIT_TIME> ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mWait;
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
uint8_t ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mBuffer[PWM_BUFFER_SIZE];
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
uint8_t* ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mLast = NULL;
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
uint8_t ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mLastBit = 0;
template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER, int XTRA0, bool FLIP, int WAIT_TIME>
volatile bool ClocklessController<DATA_PIN, T1, T2, T3, RGB_ORDER, XTRA0, FLIP, WAIT_TIME>::mBusy = false;

FASTLED_NAMESPACE_END

#endif
