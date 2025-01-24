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
#       define XX_INLINE inline
#   else
#       ifdef _MSC_VER
#           define XX_NOINLINE __declspec(noinline)
#           define XX_INLINE __forceinline
#       else
#           define XX_NOINLINE __attribute__((noinline))
#           define XX_INLINE __attribute__((always_inline)) inline
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
    // container helpers

    // element is Shared<T>
    // T has member: indexAtContainer

    template<typename T, typename SizeType>
    XX_INLINE static void RemoveFrom(T& container, SizeType& indexAtContainer) {
        auto bak = indexAtContainer;
        container.Top()->indexAtContainer = bak;
#ifndef NDEBUG
        indexAtContainer = -1;
#endif
        container.SwapRemoveAt(bak);
    }

    template<typename T, typename SizeType>
    XX_INLINE static auto MoveOutFrom(T& container, SizeType indexAtContainer) {
        auto& p = container[indexAtContainer];					// current address
        auto tmp = std::move(p);								// removed out
        if (auto& top = container.Top(); &top != &p) {          // not top?
            top->indexAtContainer = indexAtContainer;			// sync index
            p = std::move(top);						            // move
        }
        container.Pop();										// sync container size
        return tmp;
    }

    template<typename T, typename U>
    XX_INLINE static void AddTo(T& container, U&& item) {
        assert(container.Empty() || container.Top() != item);	// avoid duplicate add
        item->indexAtContainer = container.Len();
        container.Add(std::forward<U>(item));
    }

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
    XX_INLINE int16_t ZigZagDecode(uint16_t in) {
        return (int16_t)((int16_t)(in >> 1) ^ (-(int16_t)(in & 1)));
    }
    XX_INLINE int32_t ZigZagDecode(uint32_t in) {
        return (int32_t)(in >> 1) ^ (-(int32_t)(in & 1));
    }
    XX_INLINE int64_t ZigZagDecode(uint64_t in) {
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
    XX_INLINE uint16_t ZigZagEncode(int16_t in) {
        return (uint16_t)((in << 1) ^ (in >> 15));
    }
    XX_INLINE uint32_t ZigZagEncode(int32_t in) {
        return (in << 1) ^ (in >> 31);
    }
    XX_INLINE uint64_t ZigZagEncode(int64_t in) {
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
            COR_YIELD
        COR_END
    }
    ... lineNumber = Update();

*/
#define COR_BEGIN	switch (lineNumber) { case 0:
#define COR_YIELD	return __LINE__; case __LINE__:;
#define COR_EXIT	return 0;
#define COR_END		} return 0;

/* example:

    int lineNumber{};   // struct member
    void Update() {
        XXSC_BEGIN
            XXSC_YIELD
        XXSC_END
    }

*/
#define XX_BEGIN             switch (lineNumber) { case 0:
#define XX_YIELD             lineNumber = __LINE__; return; case __LINE__:;
#define XX_YIELD_F           lineNumber = __LINE__; return false; case __LINE__:;
#define XX_YIELD_TO_BEGIN    lineNumber = 0; return;
#define XX_YIELD_F_TO_BEGIN  lineNumber = 0; return false;
#define XX_END               }
