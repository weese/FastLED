#ifndef __INC_FASTPIN_PARTICLE_H
#define __INC_FASTPIN_PARTICLE_H

FASTLED_NAMESPACE_BEGIN

#if defined(FASTLED_FORCE_SOFTWARE_PINS)
#warning "Software pin support forced, pin access will be slightly slower."
#define NO_HARDWARE_PIN_SUPPORT
#undef HAS_HARDWARE_PIN_SUPPORT

#else

/// Class definition for a Pin where we know the port registers at compile time for said pin.  This allows us to make
/// a lot of optimizations, as the inlined hi/lo methods will devolve to a single io register write/bitset.
template<uint8_t PIN, uint8_t _MASK, typename _PORT, typename _DDR, typename _PIN> class _PARTICLEPIN {
public:
	typedef volatile uint8_t * port_ptr_t;
	typedef uint8_t port_t;

	inline static void setOutput() { pinMode(PIN, OUTPUT); }
	inline static void setInput() { pinMode(LED, INPUT); }

	inline static void hi() __attribute__ ((always_inline)) { pinSetFast(pin); }
	inline static void lo() __attribute__ ((always_inline)) { pinResetFast(pin); }
	inline static void set(register uint8_t val) __attribute__ ((always_inline)) { digitalWriteFast(pin, val); }

	inline static void strobe() __attribute__ ((always_inline)) { toggle(); toggle(); }

	inline static void toggle() __attribute__ ((always_inline)) { _PIN::r() = _MASK; }

	inline static void hi(register port_ptr_t /*port*/) __attribute__ ((always_inline)) { hi(); }
	inline static void lo(register port_ptr_t /*port*/) __attribute__ ((always_inline)) { lo(); }
	inline static void fastset(register port_ptr_t /*port*/, register uint8_t val) __attribute__ ((always_inline)) { set(val); }

	inline static port_t hival() __attribute__ ((always_inline)) { return 0; }
	inline static port_t loval() __attribute__ ((always_inline)) { return 0; }
	inline static port_ptr_t port() __attribute__ ((always_inline)) { return NULL; }
	inline static port_t mask() __attribute__ ((always_inline)) { return _MASK; }
};



/// PARTICLE definitions for pins.  Getting around  the fact that I can't pass GPIO register addresses in as template arguments by instead creating
/// a custom type for each GPIO register with a single, static, aggressively inlined function that returns that specific GPIO register.  A similar
/// trick is used a bit further below for the ARM GPIO registers (of which there are far more than on PARTICLE!)
typedef volatile uint8_t & reg8_t;
#define _R(T) struct __gen_struct_ ## T
#define _RD8(T) struct __gen_struct_ ## T { static inline reg8_t r() { return T; }};

#define _FL_IO(L,C) _RD8(DDR ## L); _RD8(PORT ## L); _RD8(PIN ## L); _FL_DEFINE_PORT3(L, C, _R(PORT ## L));
#define _FL_DEFPIN(_PIN, BIT, L) template<> class FastPin<_PIN> : public _PARTICLEPIN<_PIN, 1<<BIT, _R(PORT ## L), _R(DDR ## L), _R(PIN ## L)> {};
#endif


// Particle Boards (Photon, Argon).
// https://docs.particle.io/datasheets/wi-fi/photon-datasheet/
// https://docs.particle.io/datasheets/wi-fi/argon-datasheet/
#if defined(SPARK)

  #if (defined(PLATFORM_PHOTON) && PLATFORM_ID == PLATFORM_PHOTON) \
   || (defined(PLATFORM_PHOTON_DEV) && PLATFORM_ID == PLATFORM_PHOTON_DEV) \
   || (defined(PLATFORM_PHOTON_PRODUCTION) && PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION)

// _IO32(A); _IO32(B); _IO32(C); _IO32(D); _IO32(E); _IO32(F); _IO32(G);
    #define MAX_PIN 19
    _FL_DEFPIN(0, 7, B);
    _FL_DEFPIN(1, 6, B);
    _FL_DEFPIN(2, 5, B);
    _FL_DEFPIN(3, 4, B);
    _FL_DEFPIN(4, 3, B);
    _FL_DEFPIN(5, 15, A);
    _FL_DEFPIN(6, 14, A);
    _FL_DEFPIN(7, 13, A);
    _FL_DEFPIN(10, 5, C);
    _FL_DEFPIN(11, 3, C);
    _FL_DEFPIN(12, 2, C);
    _FL_DEFPIN(13, 5, A);
    _FL_DEFPIN(14, 6, A);
    _FL_DEFPIN(15, 7, A);
    _FL_DEFPIN(16, 4, A);
    _FL_DEFPIN(17, 0, A);
    _FL_DEFPIN(18, 10, A);
    _FL_DEFPIN(19, 9, A);
    _FL_DEFPIN(20, 7, C);

  #elif PLATFORM_ID == PLATFORM_ARGON
    #if defined(__FASTPIN_ARM_NRF52_VARIANT_FOUND)
        #error "Cannot define more than one board at a time"
    #else
        #define __FASTPIN_ARM_NRF52_VARIANT_FOUND
    #endif
    #define MAX_PIN 19
    _FL_DEFPIN(0, 26, 1);
    _FL_DEFPIN(1, 27, 1);
    _FL_DEFPIN(2, 33, 1);
    _FL_DEFPIN(3, 34, 1);
    _FL_DEFPIN(4, 40, 1);
    _FL_DEFPIN(5, 42, 1);
    _FL_DEFPIN(6, 43, 1);
    _FL_DEFPIN(7, 44, 1);
    _FL_DEFPIN(8, 35, 1);
    _FL_DEFPIN(9, 6, 0);
    _FL_DEFPIN(10, 8, 0);
    _FL_DEFPIN(11, 46, 1);
    _FL_DEFPIN(12, 45, 1);
    _FL_DEFPIN(13, 47, 1);
    _FL_DEFPIN(14, 3, 0);
    _FL_DEFPIN(15, 4, 0);
    _FL_DEFPIN(16, 28, 0);
    _FL_DEFPIN(17, 29, 0);
    _FL_DEFPIN(18, 30, 0);
    _FL_DEFPIN(19, 31, 0);

  #else
    #define MAX_PIN 19
    _DEFPIN_ARM(0, 7, B);
    _DEFPIN_ARM(1, 6, B);
    _DEFPIN_ARM(2, 5, B);
    _DEFPIN_ARM(3, 4, B);
    _DEFPIN_ARM(4, 3, B);
    _DEFPIN_ARM(5, 15, A);
    _DEFPIN_ARM(6, 14, A);
    _DEFPIN_ARM(7, 13, A);
    _DEFPIN_ARM(8, 8, A);
    _DEFPIN_ARM(9, 9, A);
    _DEFPIN_ARM(10, 0, A);
    _DEFPIN_ARM(11, 1, A);
    _DEFPIN_ARM(12, 4, A);
    _DEFPIN_ARM(13, 5, A);
    _DEFPIN_ARM(14, 6, A);
    _DEFPIN_ARM(15, 7, A);
    _DEFPIN_ARM(16, 0, B);
    _DEFPIN_ARM(17, 1, B);
    _DEFPIN_ARM(18, 3, A);
    _DEFPIN_ARM(19, 2, A);

#endif

#endif // FASTLED_FORCE_SOFTWARE_PINS

FASTLED_NAMESPACE_END

#endif // __INC_FASTPIN_PARTICLE_H
