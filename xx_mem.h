﻿#pragma once
#include "xx_includes.h"

namespace xx {
    /************************************************************************************/
    // Scope Guard( F == lambda )

    template<class F>
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
    // helpers

    template<typename T>
    struct FromTo {
        T from{}, to{};
    };

    template<typename T>
    struct CurrentMax {
        T current{}, max{};
    };

    template<typename T, class = std::enable_if_t<std::is_enum_v<T>>>
    inline bool FlagContains(T const& a, T const& b) {
        using U = std::underlying_type_t<T>;
        return ((U)a & (U)b) != U{};
    }


    /************************************************************************************/
    // mem utils

    // 数字字节序交换
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

    // 带符号整数 解码 return (in 为单数) ? -(in + 1) / 2 : in / 2
    inline XX_INLINE int16_t ZigZagDecode(uint16_t in) {
        return (int16_t)((int16_t)(in >> 1) ^ (-(int16_t)(in & 1)));
    }
    inline XX_INLINE int32_t ZigZagDecode(uint32_t in) {
        return (int32_t)(in >> 1) ^ (-(int32_t)(in & 1));
    }
    inline XX_INLINE int64_t ZigZagDecode(uint64_t in) {
        return (int64_t)(in >> 1) ^ (-(int64_t)(in & 1));
    }

    // 返回首个出现 1 的 bit 的下标
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    XX_INLINE size_t Calc2n(T n) {
//#ifdef _MSC_VER
        return (sizeof(size_t) * 8 - 1) - std::countl_zero(n);
//#else
//        if constexpr (sizeof(T) < 8) {
//            return 31 - __builtin_clz(n);
//        } else {
//            return 63 - __builtin_clzll(n);
//        }
//#endif
    }

    // 返回一个刚好大于 n 的 2^x 对齐数
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    XX_INLINE T Round2n(T n) {
        auto shift = Calc2n(n);
        auto rtv = T(1) << shift;
        if (rtv == n) return n;
        else return rtv << 1;
    }

    // 带符号整数 编码  return in < 0 ? (-in * 2 - 1) : (in * 2)
    inline XX_INLINE uint16_t ZigZagEncode(int16_t in) {
        return (uint16_t)((in << 1) ^ (in >> 15));
    }
    inline XX_INLINE uint32_t ZigZagEncode(int32_t in) {
        return (in << 1) ^ (in >> 31);
    }
    inline XX_INLINE uint64_t ZigZagEncode(int64_t in) {
        return (in << 1) ^ (in >> 63);
    }
}
