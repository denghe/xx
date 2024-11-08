#pragma once

#include <bit>
#include <concepts>
#include <type_traits>
#include <utility>
#include <initializer_list>
#include <chrono>
#include <optional>
#include <variant>
#include <array>
#include <tuple>
#include <vector>
#include <queue>
#include <deque>
#include <string>
#include <sstream>
#include <string_view>
#include <charconv>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <list>
#include <memory>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <filesystem>
#include <coroutine>
#if __has_include(<span>)
#include <span>
#endif
#if __has_include(<format>)
#include <format>
#endif

#define _USE_MATH_DEFINES  // needed for M_PI and M_PI2
#include <math.h>          // M_PI
#undef _USE_MATH_DEFINES
#include <cstddef>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;


#ifdef _WIN32
#ifndef NOMINMAX
#	define NOMINMAX
#endif
//#	define NODRAWTEXT
//#	define NOGDI            // d3d9 need it
#	define NOBITMAP
#	define NOMCX
#	define NOSERVICE
#	define NOHELP
#	define WIN32_LEAN_AND_MEAN
#   include <WinSock2.h>
#   include <process.h>
#	include <Windows.h>
#   include <intrin.h>     // _BitScanReverseXXXX _byteswap_XXXX
#   include <ShlObj.h>
#else
#	include <unistd.h>    // for usleep
#   include <arpa/inet.h>  // __BYTE_ORDER __LITTLE_ENDIAN __BIG_ENDIAN
#endif

#ifndef XX_NOINLINE
#   ifndef NDEBUG
#       define XX_NOINLINE
#       define XX_INLINE
#       define XX_INLINE
#   else
#       ifdef _MSC_VER
#           define XX_NOINLINE __declspec(noinline)
#           define XX_INLINE __forceinline
#       else
#           define XX_NOINLINE __attribute__((noinline))
#           define XX_INLINE __attribute__((always_inline))
#           define XX_INLINE __attribute__((always_inline))
#       endif
#   endif
#endif

#ifndef XX_STRINGIFY
#	define XX_STRINGIFY(x)  XX_STRINGIFY_(x)
#	define XX_STRINGIFY_(x)  #x
#endif

#ifdef _MSC_VER
#    define XX_ALIGN2( x )		    __declspec(align(2)) x
#    define XX_ALIGN4( x )		    __declspec(align(4)) x
#    define XX_ALIGN8( x )		    __declspec(align(8)) x
#    define XX_ALIGN16( x )		    __declspec(align(16)) x
#    define XX_ALIGN32( x )		    __declspec(align(32)) x
#    define XX_ALIGN64( x )		    __declspec(align(64)) x
#else
#    define XX_ALIGN2( x )          x __attribute__ ((aligned (2)))
#    define XX_ALIGN4( x )          x __attribute__ ((aligned (4)))
#    define XX_ALIGN8( x )          x __attribute__ ((aligned (8)))
#    define XX_ALIGN16( x )         x __attribute__ ((aligned (16)))
#    define XX_ALIGN32( x )         x __attribute__ ((aligned (32)))
#    define XX_ALIGN64( x )         x __attribute__ ((aligned (64)))
#endif

#ifdef _MSC_VER
#    define XX_LIKELY(x)                 (x)
#    define XX_UNLIKELY(x)               (x)
#else
#    define XX_UNLIKELY(x)               __builtin_expect((x), 0)
#    define XX_LIKELY(x)                 __builtin_expect((x), 1)
#endif

// __restrict like?
#if defined(__clang__)
#  define XX_ASSUME(e) __builtin_assume(e)
#elif defined(__GNUC__) && !defined(__ICC)
#  define XX_ASSUME(e) if (e) {} else { __builtin_unreachable(); }  // waiting for gcc13 c++23 [[assume]]
#elif defined(_MSC_VER) || defined(__ICC)
#  define XX_ASSUME(e) __assume(e)
#endif

#ifndef _countof
template<typename T, size_t N>
size_t _countof(T const (&arr)[N]) {
    return N;
}
#endif

#ifndef _offsetof
#define _offsetof(s,m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - _offsetof(type, member)))
#endif

#ifndef _WIN32
inline void Sleep(int const& ms) {
    usleep(ms * 1000);
}
#endif

#if defined(XX_DISABLE_ASSERT_IN_RELEASE) or not defined(NDEBUG)
    #define xx_assert assert
#else
    #ifdef _MSC_VER
        extern "C" {
            _ACRTIMP void __cdecl _wassert(
                _In_z_ wchar_t const* _Message,
                _In_z_ wchar_t const* _File,
                _In_   unsigned       _Line
            );
        }
        #define xx_assert(expression) (void)(                                                        \
            (!!(expression)) ||                                                              \
            (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
        )
    #elif defined(__GNUC__) and defined(__MINGW32__)
        #ifdef __cplusplus
        extern "C" {
        #endif
        _CRTIMP void __cdecl __MINGW_ATTRIB_NORETURN _wassert(const wchar_t *_Message,const wchar_t *_File,unsigned _Line);
        _CRTIMP void __cdecl __MINGW_ATTRIB_NORETURN _assert (const char *_Message, const char *_File, unsigned _Line);
        #ifdef __cplusplus
        }
        #endif
        #if defined(_UNICODE) || defined(UNICODE)
            #define xx_assert(_Expression) \
             (void) \
             ((!!(_Expression)) || \
              (_wassert(_CRT_WIDE(#_Expression),_CRT_WIDE(__FILE__),__LINE__),0))
        #else /* not unicode */
            #define xx_assert(_Expression) \
             (void) \
             ((!!(_Expression)) || \
              (_assert(#_Expression,__FILE__,__LINE__),0))
        #endif /* _UNICODE||UNICODE */
    #else
        #ifdef EMSCRIPTEN
            #define xx_assert(x) ((void)((x) || (__assert_fail(#x, __FILE__, __LINE__, __func__),0)))
        #else
            extern "C" {
            /* This prints an "Assertion failed" message and aborts.  */
            extern void __assert_fail(const char *__assertion, const char *__file,
                                      unsigned int __line, const char *__function)
            __THROW __attribute__ ((__noreturn__));
            }
            #define xx_assert(expression) (void)(                                                \
                (!!(expression)) ||                                                              \
                (__assert_fail(#expression, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__), 0)           \
            )
        #endif
    #endif
#endif

#ifdef __GNUC__
//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif


namespace xx {
    /************************************************************************************/
    // util types

    enum class ForeachResult {
        Continue,
        RemoveAndContinue,
        Break,
        RemoveAndBreak
    };

    template<typename T>
    struct FromTo {
        T from, to;
    };

    template<typename T>
    struct CurrentMax {
        T current, max;
    };

    /************************************************************************************/
    // scope guards

    template<class F>   // F == lambda
    [[nodiscard]] auto MakeScopeGuard(F&& f) noexcept {
        struct ScopeGuard {
            F f;
            bool cancel;
            explicit ScopeGuard(F&& f) noexcept : f(std::forward<F>(f)), cancel(false) {}
            ~ScopeGuard() noexcept { if (!cancel) { f(); } }
            inline void Cancel() noexcept { cancel = true; }
            inline void operator()(bool cancel = false) {
                f();
                this->cancel = cancel;
            }
        };
        return ScopeGuard(std::forward<F>(f));
    }

    template<class F>
    [[nodiscard]] auto MakeSimpleScopeGuard(F&& f) noexcept {
        struct SG { F f; SG(F&& f) noexcept : f(std::forward<F>(f)) {} ~SG() { f(); } };
        return SG(std::forward<F>(f));
    }

    /************************************************************************************/
    // aligned alloc free

    template<typename T>
    XX_INLINE T* AlignedAlloc(size_t siz = sizeof(T)) {
        assert(siz >= sizeof(T));
        if constexpr (alignof(T) <= sizeof(void*)) return (T*)malloc(siz);
        else {
#if defined(_MSC_VER) || defined(__MINGW32__)
            return (T*)_aligned_malloc(siz, alignof(T));
#else   // emscripten
            return (T*)aligned_alloc(alignof(T), siz);
#endif
        }
    }

    template<typename T>
    XX_INLINE void AlignedFree(void* p) {
#ifdef _MSC_VER
        if constexpr (alignof(T) <= sizeof(void*)) free(p);
        else {
            _aligned_free(p);
        }
#else
        free(p);
#endif
    }

    /************************************************************************************/
    // mem utils

    template<typename T>
    XX_INLINE T BSwap(T i) {
        T r;
#ifdef _WIN32
        if constexpr (sizeof(T) == 2) *(uint16_t*)&r = _byteswap_ushort(*(uint16_t*)&i);
        if constexpr (sizeof(T) == 4) *(uint32_t*)&r = _byteswap_ulong(*(uint32_t*)&i);
        if constexpr (sizeof(T) == 8) *(uint64_t*)&r = _byteswap_uint64(*(uint64_t*)&i);
#else
        if constexpr (sizeof(T) == 2) *(uint16_t*)&r = __builtin_bswap16(*(uint16_t*)&i);
        if constexpr (sizeof(T) == 4) *(uint32_t*)&r = __builtin_bswap32(*(uint32_t*)&i);
        if constexpr (sizeof(T) == 8) *(uint64_t*)&r = __builtin_bswap64(*(uint64_t*)&i);
#endif
        return r;
    }

    // signed int decode: return (in is singular: negative) ? -(in + 1) / 2 : in / 2
    inline XX_INLINE int16_t ZigZagDecode(uint16_t in) {
        return (int16_t)((int16_t)(in >> 1) ^ (-(int16_t)(in & 1)));
    }
    inline XX_INLINE int32_t ZigZagDecode(uint32_t in) {
        return (int32_t)(in >> 1) ^ (-(int32_t)(in & 1));
    }
    inline XX_INLINE int64_t ZigZagDecode(uint64_t in) {
        return (int64_t)(in >> 1) ^ (-(int64_t)(in & 1));
    }

    // return first bit '1' index
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    XX_INLINE size_t Calc2n(T n) {
        return (sizeof(size_t) * 8 - 1) - std::countl_zero(n);
    }

    // return 2^x ( >= n )
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    XX_INLINE T Round2n(T n) {
        auto shift = Calc2n(n);
        auto rtv = T(1) << shift;
        if (rtv == n) return n;
        else return rtv << 1;
    }

    // signed int encode: return in < 0 ? (-in * 2 - 1) : (in * 2)
    inline XX_INLINE uint16_t ZigZagEncode(int16_t in) {
        return (uint16_t)((in << 1) ^ (in >> 15));
    }
    inline XX_INLINE uint32_t ZigZagEncode(int32_t in) {
        return (in << 1) ^ (in >> 31);
    }
    inline XX_INLINE uint64_t ZigZagEncode(int64_t in) {
        return (in << 1) ^ (in >> 63);
    }

    // flag enum bit check
    template<typename T, class = std::enable_if_t<std::is_enum_v<T>>>
    inline bool FlagContains(T const& a, T const& b) {
        using U = std::underlying_type_t<T>;
        return ((U)a & (U)b) != U{};
    }

}

/************************************************************************************/
// stackless simulate

/* example:

    int lineNumber = 0;
    int Update() {
        COR_BEGIN
            // COR_YIELD
        COR_END
    }
    ... lineNumber = Update();

*/
#define COR_BEGIN	switch (lineNumber) { case 0:
#define COR_YIELD	return __LINE__; case __LINE__:;
#define COR_EXIT	return 0;
#define COR_END		} return 0;
