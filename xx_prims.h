﻿#pragma once
#include "xx_xy.h"

namespace xx {

    // texture uv mapping pos
    struct UV {
        uint16_t u, v;
    };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<UV, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.u, ", ", in.v);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<UV, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.Write<needReserve>(in.u, in.v);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.Read(out.u, out.v);
        }
    };


    // 3 bytes color
    struct RGB8 {
        uint8_t r, g, b;
        bool operator==(RGB8 const&) const = default;
        bool operator!=(RGB8 const&) const = default;
    };
    constexpr static RGB8 RGB8_Zero{ 0,0,0 };
    constexpr static RGB8 RGB8_Red{ 255,0,0 };
    constexpr static RGB8 RGB8_Green{ 0,255,0 };
    constexpr static RGB8 RGB8_Blue{ 0,0,255 };
    constexpr static RGB8 RGB8_White{ 255,255,255 };
    constexpr static RGB8 RGB8_Black{ 0,0,0 };
    constexpr static RGB8 RGB8_Yellow{ 255,255,0 };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<RGB8, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.r, ", ", in.g, ", ", in.b);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<RGB8, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteBuf<needReserve>(&in, 3);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadBuf(&out, 3);
        }
    };


    // 4 bytes color
    struct RGBA8 {
        uint8_t r, g, b, a;
        bool operator==(RGBA8 const&) const = default;
        bool operator!=(RGBA8 const&) const = default;
    };
    constexpr static RGBA8 RGBA8_Zero{ 0,0,0,0 };
    constexpr static RGBA8 RGBA8_Red{ 255,0,0,255 };
    constexpr static RGBA8 RGBA8_Green{ 0,255,0,255 };
    constexpr static RGBA8 RGBA8_Blue{ 0,0,255,255 };
    constexpr static RGBA8 RGBA8_White{ 255,255,255,255 };
    constexpr static RGBA8 RGBA8_Gray{ 127,127,127,255 };
    constexpr static RGBA8 RGBA8_Black{ 0,0,0,255 };
    constexpr static RGBA8 RGBA8_Yellow{ 255,255,0,255 };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<RGBA8, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.r, ", ", in.g, ", ", in.b, ", ", in.a);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<RGBA8, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteBuf<needReserve>(&in, 4);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadBuf(&out, 4);
        }
    };


    // 4 floats color
    struct RGBA {
        float r, g, b, a;

        operator RGBA8() const {
            return { uint8_t(r * 255), uint8_t(g * 255), uint8_t(b * 255), uint8_t(a * 255) };
        }

        RGBA operator+(RGBA v) const {
            return { r + v.r, g + v.g, b + v.b, a + v.a };
        }
        RGBA operator-(RGBA v) const {
            return { r - v.r, g - v.g, b - v.b, a - v.a };
        }

        RGBA operator*(IsArithmetic auto v) const {
            return { r * v, g * v, b * v, a * v };
        }
        RGBA operator/(IsArithmetic auto v) const {
            return { r / v, g / v, b / v, a / v };
        }

        RGBA& operator+=(RGBA v) {
            r += v.r;
            g += v.g;
            b += v.b;
            a += v.a;
            return *this;
        }
    };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<RGBA, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.r, ", ", in.g, ", ", in.b, ", ", in.a);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<RGBA, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixedArray<needReserve>((float*)&in, 4);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixedArray((float*)&out, 4);
        }
    };


    // pos + size
    struct Rect : XY {
        XY wh;
    };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<Rect, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.x, ", ", in.y, ", ", in.w, ", ", in.h);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<Rect, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixedArray<needReserve>((float*)&in, 4);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixedArray((float*)&out, 4);
        }
    };

    struct PosRadius {
        XY pos;
        float radius;
    };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<PosRadius, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.pos.x, ", ", in.pos.y, ", ", in.radius);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<PosRadius, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixedArray<needReserve>((float*)&in, 3);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixedArray((float*)&out, 3);
        }
    };

    union UVRect {
        struct {
            uint16_t x, y, w, h;
        };
        uint64_t data;
    };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<UVRect, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.x, ", ", in.y, ", ", in.w, ", ", in.h);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<UVRect, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.Write<needReserve>(in.x, in.y, in.w, in.h);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.Read(out.x, out.y, out.w, out.h);
        }
    };


    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/


    struct AffineTransform {
        float a, b, c, d;
        float tx, ty;

        // anchorSize = anchor * size
        void PosScaleRadiansAnchorSize(XY const& pos, XY const& scale, float radians, XY const& anchorSize) {
            float c_ = 1, s_ = 0;
            if (radians) {
                c_ = std::cos(-radians);
                s_ = std::sin(-radians);
            }
            a = c_ * scale.x;
            b = s_ * scale.x;
            c = -s_ * scale.y;
            d = c_ * scale.y;
            tx = pos.x + c_ * scale.x * -anchorSize.x - s_ * scale.y * -anchorSize.y;
            ty = pos.y + s_ * scale.x * -anchorSize.x + c_ * scale.y * -anchorSize.y;
        }

        void PosScaleRadians(XY const& pos, XY const& scale, float radians) {
            float c_ = 1, s_ = 0;
            if (radians) {
                c_ = std::cos(-radians);
                s_ = std::sin(-radians);
            }
            a = c_ * scale.x;
            b = s_ * scale.x;
            c = -s_ * scale.y;
            d = c_ * scale.y;
            tx = pos.x;
            ty = pos.y;
        }

        // anchorSize = anchor * size
        void PosScaleAnchorSize(XY const& pos, XY const& scale, XY const& anchorSize) {
            a = scale.x;
            b = 0;
            c = 0;
            d = scale.y;
            tx = pos.x + scale.x * -anchorSize.x;
            ty = pos.y + scale.y * -anchorSize.y;
        }

        void PosScale(XY const& pos, XY const& scale) {
            a = scale.x;
            b = 0;
            c = 0;
            d = scale.y;
            tx = pos.x;
            ty = pos.y;
        }

        void Pos(XY const& pos) {
            a = 1;
            b = 0;
            c = 0;
            d = 1;
            tx = pos.x;
            ty = pos.y;
        }

        void Identity() {
            a = 1;
            b = 0;
            c = 0;
            d = 1;
            tx = 0;
            ty = 0;
        }

        // default apply
        XY operator()(XY const& point) const {
            return { (float)((double)a * point.x + (double)c * point.y + tx), (float)((double)b * point.x + (double)d * point.y + ty) };
        }

        XY NoRadiansApply(XY const& point) const {
            return { (float)((double)a * point.x + tx), (float)((double)d * point.y + ty) };
        }

        // child concat parent
        AffineTransform MakeConcat(AffineTransform const& t2) {
            auto& t1 = *this;
            return { t1.a * t2.a + t1.b * t2.c, t1.a * t2.b + t1.b * t2.d, t1.c * t2.a + t1.d * t2.c, t1.c * t2.b + t1.d * t2.d,
                t1.tx * t2.a + t1.ty * t2.c + t2.tx, t1.tx * t2.b + t1.ty * t2.d + t2.ty };
        }

        AffineTransform MakeInvert() {
            auto& t = *this;
            auto determinant = 1 / (t.a * t.d - t.b * t.c);
            return { determinant * t.d, -determinant * t.b, -determinant * t.c, determinant * t.a,
                determinant * (t.c * t.ty - t.d * t.tx), determinant * (t.b * t.tx - t.a * t.ty) };
        }

        inline static AffineTransform MakePosScaleRadiansAnchorSize(XY const& pos, XY const& scale, float radians, XY const& anchorSize) {
            AffineTransform at;
            at.PosScaleRadiansAnchorSize(pos, scale, radians, anchorSize);
            return at;
        }

        inline static AffineTransform MakePosScaleAnchorSize(XY const& pos, XY const& scale, XY const& anchorSize) {
            AffineTransform at;
            at.PosScaleAnchorSize(pos, scale, anchorSize);
            return at;
        }

        inline static AffineTransform MakePosScaleRadians(XY const& pos, XY const& scale, float radians) {
            AffineTransform at;
            at.PosScaleRadians(pos, scale, radians);
            return at;
        }

        inline static AffineTransform MakePosScale(XY const& pos, XY const& scale) {
            return { scale.x, 0, 0, scale.y, pos.x, pos.y };
        }

        inline static AffineTransform MakePos(XY const& pos) {
            return { 1.0, 0.0, 0.0, 1.0, pos.x, pos.y };
        }

        inline static AffineTransform MakeIdentity() {
            return { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
        }

    };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<AffineTransform, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.a, ", ", in.b, ", ", in.c, ", ", in.d, ", ", in.tx, ", ", in.ty);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<AffineTransform, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixedArray<needReserve>((float*)&in, 6);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixedArray((float*)&out, 6);
        }
    };



    // without rotation support
    struct SimpleAffineTransform {
        float a{ 1 }, d{ 1 };
        float tx{}, ty{};

        // anchorSize = anchor * size
        XX_INLINE void PosScaleAnchorSize(XY const& pos, XY const& scale, XY const& anchorSize) {
            a = scale.x;
            d = scale.y;
            tx = pos.x - scale.x * anchorSize.x;
            ty = pos.y - scale.y * anchorSize.y;
        }

        XX_INLINE void Identity() {
            a = 1;
            d = 1;
            tx = 0;
            ty = 0;
        }

        XX_INLINE XY const& Offset() const {
            return (XY&)tx;
        }

        XX_INLINE XY const& Scale() const {
            return (XY&)a;
        }

        // apply
        XX_INLINE XY operator()(XY const& point) const {
            return { (float)((double)a * point.x + tx), (float)((double)d * point.y + ty) };
        }

        // child concat parent
        XX_INLINE SimpleAffineTransform MakeConcat(SimpleAffineTransform const& t2) const {
            auto& t1 = *this;
            return { t1.a * t2.a, t1.d * t2.d, t1.tx * t2.a + t2.tx, t1.ty * t2.d + t2.ty };
        }

        XX_INLINE SimpleAffineTransform MakeInvert() const {
            auto& t = *this;
            auto determinant = 1 / (t.a * t.d);
            return { determinant * t.d, determinant * t.a, determinant * (-t.d * t.tx), determinant * (-t.a * t.ty) };
        }

        XX_INLINE static SimpleAffineTransform MakeIdentity() {
            return { 1.0, 1.0, 0.0, 0.0 };
        }

        XX_INLINE static SimpleAffineTransform MakePosScaleAnchorSize(XY const& pos, XY const& scale, XY const& anchorSize) {
            SimpleAffineTransform t;
            t.PosScaleAnchorSize(pos, scale, anchorSize);
            return t;
        }
    };

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_same_v<SimpleAffineTransform, std::remove_cvref_t<T>>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, in.a, ", ", in.d, ", ", in.tx, ", ", in.ty);
        }
    };

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<SimpleAffineTransform, std::remove_cvref_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixedArray<needReserve>((float*)&in, 4);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixedArray((float*)&out, 4);
        }
    };

}
