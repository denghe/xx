﻿#pragma once
#include <xx_data.h>
#include <Fixed64.h>

namespace xx {

    // Fixed64 helper class
    // some code is copy from Fixed64.h or FixedUtil.h
    struct FX64 {
        using FP_ULONG = Fixed64::FP_ULONG;
        using FP_LONG = Fixed64::FP_LONG;
        using FP_INT = Fixed64::FP_INT;

        /********************************************************************************************/
        // following code is copy from Fixed64.h && FixedUtil.h

    protected:
        static constexpr auto Shift = Fixed64::Shift;
        static constexpr auto FractionMask = Fixed64::FractionMask;
        static constexpr auto IntegerMask = Fixed64::IntegerMask;
        static constexpr auto Zero = Fixed64::Zero;
        static constexpr auto Neg1 = Fixed64::Neg1;
        static constexpr auto One = Fixed64::One;
        static constexpr auto Two = Fixed64::Two;
        static constexpr auto Three = Fixed64::Three;
        static constexpr auto Four = Fixed64::Four;
        static constexpr auto Half = Fixed64::Half;
        static constexpr auto Pi = Fixed64::Pi;
        static constexpr auto Pi2 = Fixed64::Pi2;
        static constexpr auto PiHalf = Fixed64::PiHalf;
        static constexpr auto E = Fixed64::E;
        static constexpr auto MinValue = Fixed64::MinValue;
        static constexpr auto MaxValue = Fixed64::MaxValue;
        static constexpr auto RCP_LN2 = Fixed64::RCP_LN2;
        static constexpr auto RCP_LOG2_E = Fixed64::RCP_LOG2_E;
        static constexpr auto RCP_HALF_PI = Fixed64::RCP_HALF_PI;

        constexpr static FP_INT CeilToInt(FP_LONG v) {
            return (FP_INT)((v + (One - 1)) >> Shift);
        }
        constexpr static FP_INT FloorToInt(FP_LONG v) {
            return (FP_INT)(v >> Shift);
        }
        constexpr static FP_INT RoundToInt(FP_LONG v) {
            return (FP_INT)((v + Half) >> Shift);
        }
        static constexpr double ToDouble(FP_LONG v) {
            return (double)v * (1.0 / 4294967296.0);
        }
        static constexpr float ToFloat(FP_LONG v) {
            return (float)v * (1.0f / 4294967296.0f);
        }

        static constexpr FP_INT Nlz(FP_ULONG x) {
            FP_INT n = 0;
            if (x <= INT64_C(0x00000000FFFFFFFF)) { n = n + 32; x = x << 32; }
            if (x <= INT64_C(0x0000FFFFFFFFFFFF)) { n = n + 16; x = x << 16; }
            if (x <= INT64_C(0x00FFFFFFFFFFFFFF)) { n = n + 8; x = x << 8; }
            if (x <= INT64_C(0x0FFFFFFFFFFFFFFF)) { n = n + 4; x = x << 4; }
            if (x <= INT64_C(0x3FFFFFFFFFFFFFFF)) { n = n + 2; x = x << 2; }
            if (x <= INT64_C(0x7FFFFFFFFFFFFFFF)) { n = n + 1; }
            if (x == 0) return 64;
            return n;
        }

        static constexpr FP_LONG LogicalShiftRight(FP_LONG v, FP_INT shift) {
            return (FP_LONG)((FP_ULONG)v >> shift);
        }
        static constexpr FP_LONG ShiftRight(FP_LONG v, FP_INT shift) {
            return (shift >= 0) ? (v >> shift) : (v << -shift);
        }
        static constexpr FP_INT Qmul30(FP_INT a, FP_INT b) {
            return (FP_INT)((FP_LONG)a * (FP_LONG)b >> 30);
        }
        static constexpr FP_INT RcpPoly4Lut8Table[] = {
            796773553, -1045765287, 1072588028, -1073726795, 1073741824,
            456453183, -884378041, 1042385791, -1071088216, 1073651788,
            276544830, -708646126, 977216564, -1060211779, 1072962711,
            175386455, -559044324, 893798171, -1039424537, 1071009496,
            115547530, -440524957, 805500803, -1010097984, 1067345574,
            78614874, -348853503, 720007233, -974591889, 1061804940,
            54982413, -278348465, 641021491, -935211003, 1054431901,
            39383664, -223994590, 569927473, -893840914, 1045395281,
        };

        static constexpr FP_INT RcpPoly4Lut8(FP_INT a) {
            FP_INT offset = (a >> 27) * 5;
            FP_INT y = Qmul30(a, RcpPoly4Lut8Table[offset + 0]);
            y = Qmul30(a, y + RcpPoly4Lut8Table[offset + 1]);
            y = Qmul30(a, y + RcpPoly4Lut8Table[offset + 2]);
            y = Qmul30(a, y + RcpPoly4Lut8Table[offset + 3]);
            y = y + RcpPoly4Lut8Table[offset + 4];
            return y;
        }

        // ********* add by me
        static constexpr FP_LONG LogicalShiftRight(FP_ULONG v, FP_INT shift) {
            return (FP_LONG)(v >> shift);
        }

        static constexpr FP_LONG Mul(FP_LONG a, FP_LONG b) {
            FP_LONG ai = a >> Shift;
            FP_ULONG af = (a & FractionMask);   // ********* fix by me
            FP_LONG bi = b >> Shift;
            FP_ULONG bf = (b & FractionMask);   // ********* fix by me
            return LogicalShiftRight(af * bf, Shift) + ai * b + af * bi;
        }

        static constexpr FP_LONG MulIntLongLong(FP_INT a, FP_LONG b) {
            assert(a >= 0);
            FP_LONG bi = b >> Shift;
            FP_LONG bf = b & FractionMask;
            return LogicalShiftRight(a * bf, Shift) + a * bi;
        }

        static constexpr FP_INT ONE = (1 << 30);
        static constexpr FP_LONG Div(FP_LONG a, FP_LONG b) {
            if (b == MinValue || b == 0) {
                assert(false);
                return 0;
            }

            // Handle negative values.
            FP_INT sign = (b < 0) ? -1 : 1;
            b *= sign;

            // Normalize input into [1.0, 2.0( range (convert to s2.30).
            FP_INT offset = 31 - Nlz((FP_ULONG)b);
            FP_INT n = (FP_INT)ShiftRight(b, offset + 2);
            assert(n >= ONE);

            // Polynomial approximation.
            FP_INT res = RcpPoly4Lut8(n - ONE);

            // Apply exponent, convert back to s32.32.
            FP_LONG y = MulIntLongLong(res, a) << 2;
            return ShiftRight(sign * y, offset);
        }

        // pow log ...

        // ********* add by me
        static constexpr FP_LONG Pow2(FP_LONG a) {
            FP_LONG ai = a >> Shift;
            FP_ULONG af = (a & FractionMask);
            return LogicalShiftRight(af * af, Shift) + ai * a + af * ai;
        }

        /********************************************************************************************/
    public:

        XX_INLINE FX64 Sqrt() const { return Fixed64::Sqrt(value); }
        XX_INLINE FX64 SqrtFast() const { return Fixed64::SqrtFast(value); }
        XX_INLINE FX64 SqrtFastest() const { return Fixed64::SqrtFastest(value); }
        XX_INLINE FX64 SqrtPrecise() const { return Fixed64::SqrtPrecise(value); }

        XX_INLINE FX64 RSqrt() const { return Fixed64::RSqrt(value); }
        XX_INLINE FX64 RSqrtFast() const { return Fixed64::RSqrtFast(value); }
        XX_INLINE FX64 RSqrtFastest() const { return Fixed64::RSqrtFastest(value); }

        XX_INLINE FX64 Sin() const { return Fixed64::Sin(value); }
        XX_INLINE FX64 SinFast() const { return Fixed64::SinFast(value); }
        XX_INLINE FX64 SinFastest() const { return Fixed64::SinFastest(value); }

        XX_INLINE FX64 Cos() const { return Fixed64::Cos(value); }
        XX_INLINE FX64 CosFast() const { return Fixed64::CosFast(value); }
        XX_INLINE FX64 CosFastest() const { return Fixed64::CosFastest(value); }

        XX_INLINE static FX64 Atan2(FX64 const& y, FX64 const& x) { return Fixed64::Atan2(y.value, x.value); }
        XX_INLINE static FX64 Atan2Fast(FX64 const& y, FX64 const& x) { return Fixed64::Atan2Fast(y.value, x.value); }
        XX_INLINE static FX64 Atan2Fastest(FX64 const& y, FX64 const& x) { return Fixed64::Atan2Fastest(y.value, x.value); }

        XX_INLINE FX64 Abs() const { return Fixed64::Abs(value); }

        XX_INLINE FX64 Normalize() const {
            auto m = Mul(value, value);
            auto s = Fixed64::Sqrt(m);
            return Div(value, s);
        }

        XX_INLINE FX64 Clamp(FX64 const& min, FX64 const& max) const {
            return { (value > max.value) ? max.value : (value < min.value) ? min.value : value };
        }
        XX_INLINE FX64 Clamp01() const {
            return { (value > One) ? One : (value < Zero) ? Zero : value };
        }


        // ... 

        /********************************************************************************************/

        FP_LONG value;
        //constexpr operator FP_LONG() const { return value; }

        constexpr FX64() : value(0) {};
        constexpr FX64(FX64 const&) = default;
        constexpr FX64& operator=(FX64 const&) = default;

        constexpr FX64(FP_LONG v) : value(v) {}
        constexpr FX64(int32_t v) : value((FP_LONG)v << Fixed64::Shift) {}
        constexpr FX64(double v) : value((FP_LONG)(v * 4294967296.0)) {}
        constexpr FX64(float v) : value((FP_LONG)(v * 4294967296.0f)) {}

        constexpr FX64& operator=(FP_LONG v) { value = (FP_LONG)v << Fixed64::Shift; return *this; }
        constexpr FX64& operator=(int32_t v) { value = (FP_LONG)v << Fixed64::Shift; return *this; }
        constexpr FX64& operator=(double v) { value = (FP_LONG)(v * 4294967296.0); return *this; }
        constexpr FX64& operator=(float v) { value = (FP_LONG)(v * 4294967296.0f); return *this; }

        constexpr FX64 operator-() const { return { -value }; }

        constexpr FX64 operator+(FX64 const& v) const { return { value + v.value }; }
        constexpr FX64 operator-(FX64 const& v) const { return { value - v.value }; }
        constexpr FX64 operator*(FX64 const& v) const { return { Mul(value, v.value) }; }
        constexpr FX64 operator/(FX64 const& v) const { return { Div(value, v.value) }; }

        constexpr FX64& operator+=(FX64 const& v) { value += v.value; return *this; }
        constexpr FX64& operator-=(FX64 const& v) { value -= v.value; return *this; }
        constexpr FX64& operator*=(FX64 const& v) { value = Mul(value, v.value); return *this; }
        constexpr FX64& operator/=(FX64 const& v) { value = Div(value, v.value); return *this; }

        constexpr bool operator==(FX64 const& v) const { return value == v.value; }
        constexpr bool operator!=(FX64 const& v) const { return value != v.value; }
        constexpr bool operator>=(FX64 const& v) const { return value >= v.value; }
        constexpr bool operator<=(FX64 const& v) const { return value <= v.value; }
        constexpr bool operator>(FX64 const& v) const { return value > v.value; }
        constexpr bool operator<(FX64 const& v) const { return value < v.value; }

        constexpr void SetZero() { value = 0; }
        constexpr bool IsZero() const { return value == 0; }
        constexpr int32_t ToInt() const { return FloorToInt(value); }
        constexpr int32_t ToIntFloor() const { return FloorToInt(value); }
        constexpr int32_t ToIntCeil() const { return CeilToInt(value); }
        constexpr int32_t ToIntRound() const { return RoundToInt(value); }
        constexpr double ToDouble() const { return ToDouble(value); }
        constexpr float ToFloat() const { return ToFloat(value); }

        constexpr FX64 Pow2() const { return Pow2(value); }
    };

    static constexpr FX64 FX64_0{ 0 };
    static constexpr FX64 FX64_1{ 1 };
    static constexpr FX64 FX64_10{ 10 };
    static constexpr FX64 FX64_0d1{ 0.1 };
    static constexpr FX64 FX64_PI{ 3.14159 };
    static constexpr FX64 FX64_NPI{ -3.14159 };
    static constexpr FX64 FX64_2PI{ FX64_PI * 2 };
    static constexpr FX64 FX64_N2PI{ -FX64_2PI };
    static constexpr FX64 FX64_PI_NEG{ -FX64_PI };
    static constexpr FX64 FX64_PI_DIV_10{ 0.314159 };
    static constexpr FX64 FX64_PI_DIV_10_NEG{ -FX64_PI_DIV_10 };
    // ...

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<FX64, T>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.ToDouble());
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_base_of_v<FX64, T>>> {

        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixed<needReserve>(in.value);
        }

        static inline int Read(Data_r& dr, T& out) {
            return dr.ReadFixed(out.value);
        }
    };

}
