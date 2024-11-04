#pragma once
#include "xx_xy.h"
#include "xx_fx64.h"

namespace xx {

    inline static constexpr float gPI{ (float)M_PI }, gNPI{ -gPI }, g2PI{ gPI * 2 }, gPI_2{ gPI / 2 };
    inline static constexpr float gSQRT2{ 1.414213562373095f }, gSQRT2_1 { 0.7071067811865475f };

    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/

    namespace FrameControl {

        inline XX_INLINE void Forward(float& frameIndex, float inc, float from, float to) {
            frameIndex += inc;
            if (frameIndex >= to) {
                frameIndex = from + (frameIndex - to);
            }
        }

        inline XX_INLINE void Forward(float& frameIndex, float inc, float to) {
            Forward(frameIndex, inc, 0, to);
        }

        inline XX_INLINE	void Backward(float& frameIndex, float inc, float from, float to) {
            frameIndex -= inc;
            if (frameIndex <= from) {
                frameIndex = to - (from - frameIndex);
            }
        }

        inline XX_INLINE	void Backward(float& frameIndex, float inc, float to) {
            Backward(frameIndex, inc, 0, to);
        }

    }

    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/


    namespace RotateControl {

        inline XX_INLINE float Gap(float tar, float cur) {
            auto gap = cur - tar;
            if (gap > gPI) {
                gap -= g2PI;
            }
            if (gap < gNPI) {
                gap += g2PI;
            }
            return gap;
        }

        // calc cur to tar by rate
        // return cur's new value
        inline XX_INLINE float LerpAngleByRate(float tar, float cur, float rate) {
            auto gap = Gap(tar, cur);
            return cur - gap * rate;        // todo: verify
        }

        /*
         * linear example:

            auto bak = radians;
            radians = xx::RotateControl::LerpAngleByFixed(targetRadians, radians, frameMaxChangeRadian);
            auto rd = bak - radians;
            if (rd >= xx::gPI)
            {
                rd -= xx::g2PI;
            }
            else if (rd < xx::gNPI)
            {
                rd += xx::g2PI;
            }
            auto radiansStep = rd / n;
        */
        // calc cur to tar by fixed raidians
        // return cur's new value
        inline XX_INLINE float LerpAngleByFixed(float tar, float cur, float a) {
            auto gap = Gap(tar, cur);
            if (gap < 0) {
                if (gap >= -a) return tar;
                else if (auto r = cur + a; r < gPI) return r;
                else return r - g2PI;
            } else {
                if (gap <= a) return tar;
                else if (auto r = cur - a; r > gPI) return r;
                else return r + g2PI;
            }
        }

        // change cur to tar by a( max value )
        // return true: cur == tar
        inline XX_INLINE bool Step(float& cur, float tar, float a) {
            return (cur = LerpAngleByFixed(tar, cur, a)) == tar;
        }

        // limit a by from ~ to
        // no change: return false
        inline XX_INLINE bool Limit(float& a, float from, float to) {
            // -PI ~~~~~~~~~~~~~~~~~~~~~~~~~ a ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ PI
            assert(a >= gNPI && a <= gPI);
            // from ~~~~~~~~~~~~~~ a ~~~~~~~~~~~~~~~~ to
            if (a >= from && a <= to) return false;
            // from ~~~~~~~~~~~~~~~ -PI ~~~~~~~~~~~~~~~~~~ to ~~~~~~~~~~~~~~~ PI
            if (from < gNPI) {
                // ~~~~~~ from ~~~~~~~ -PI ~~~~~~ to ~~~~ a ~~~~ 0 ~~~~~~~~~~~~~ PI
                if (a < 0) {
                    if (a - to < from + g2PI - a) {
                        a = to;
                    } else {
                        a = from + g2PI;
                    }
                    // ~~~~~ d ~~~~~~ from ~~~~~~~ -PI ~~~~~~~~ to ~~~~~~~~ 0 ~~~~~~~ a ~~~~ PI
                } else {
                    auto d = a - g2PI;
                    if (d >= from && d <= to) return false;
                    else {
                        if (from - d < a - to) {
                            a = from + g2PI;
                        } else {
                            a = to;
                        }
                    }
                }
                // -PI ~~~~~~~~~~~~~~~ from ~~~~~~~~~~~~~~~~~~ PI ~~~~~~~~~~~~~~~ to
            } else if (to > gPI) {
                // -PI ~~~~~~~~~~~~~~~ 0 ~~~~~ a ~~~~~ from ~~~~~~ PI ~~~~~~~ to
                if (a > 0) {
                    if (from - a < a - (to - g2PI)) {
                        a = from;
                    } else {
                        a = to - g2PI;
                    }
                    // -PI ~~~~~~~ a ~~~~~~~~ 0 ~~~~~~~ from ~~~~~~ PI ~~~~~~~ to ~~~~~ d ~~~~~
                } else {
                    auto d = a + g2PI;
                    if (d >= from && d <= to) return false;
                    else {
                        if (from - a < d - to) {
                            a = from;
                        } else {
                            a = to - g2PI;
                        }
                    }
                }
            } else {
                // -PI ~~~~~ a ~~~~ from ~~~~~~~~~~~~~~~~~~ to ~~~~~~~~~~~ PI
                if (a < from) {
                    if (to <= 0 || from - a < a - (to - g2PI)) {
                        a = from;
                    } else {
                        a = to;
                    }
                    // -PI ~~~~~~~~~ from ~~~~~~~~~~~~~~~~~~ to ~~~~~ a ~~~~~~ PI
                } else {
                    if (from > 0 || a - to < from + g2PI - a) {
                        a = to;
                    } else {
                        a = from;
                    }
                }
            }
            return true;
        }

    }

    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/

    namespace TranslateControl {

        // b: box    c: circle    w: width    h: height    r: radius
        // if intersect, cx & cy will be changed & return true
        template<typename T = int32_t>
        bool MoveCircleIfIntersectsBox(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr) {
            auto dx = std::abs(cx - bx);
            if (dx > bHalfWidth + cr) return false;

            auto dy = std::abs(cy - by);
            if (dy > bHalfHeight + cr) return false;

            if (dx <= bHalfWidth || dy <= bHalfHeight) {
                if (bHalfWidth - dx > bHalfHeight - dy) {
                    if (by > cy) {
                        cy = by - bHalfHeight - cr - 1;	// top
                    } else {
                        cy = by + bHalfHeight + cr + 1;	// bottom
                    }
                } else {
                    if (bx > cx) {
                        cx = bx - bHalfWidth - cr - 1;	// left
                    } else {
                        cx = bx + bHalfWidth + cr + 1;	// right
                    }
                }
                return true;
            }

            auto dx2 = dx - bHalfWidth;
            auto dy2 = dy - bHalfHeight;
            if (dx2 * dx2 + dy2 * dy2 <= cr * cr) {
                // change cx & cy
                auto incX = dx2, incY = dy2;
                auto dSeq = dx2 * dx2 + dy2 * dy2;
                if (dSeq == T{}) {
                    incX = bHalfWidth + T(cr * 0.7071067811865475 + 1);
                    incY = bHalfHeight + T(cr * 0.7071067811865475 + 1);
                } else {
                    auto d = std::sqrt(dSeq);
                    incX = bHalfWidth + T(cr * dx2 / d) + 1;
                    incY = bHalfHeight + T(cr * dy2 / d) + 1;
                }

                if (cx < bx) {
                    incX = -incX;
                }
                if (cy < by) {
                    incY = -incY;
                }
                cx = bx + incX;
                cy = by + incY;

                return true;
            }
            return false;
        }

        inline bool MoveCircleIfIntersectsBox2(FromTo<XY> aabb, XY& cPos, float cr) {
            // find rect nearest point
            XY np;
            np.x = std::max(aabb.from.x, std::min(cPos.x, aabb.to.x));
            np.y = std::max(aabb.from.y, std::min(cPos.y, aabb.to.y));

            // calc
            auto d = np - cPos;
            auto mag = std::sqrt(d.x * d.x + d.y * d.y);
            auto overlap = cr - mag;

            if (overlap > 0 && mag != 0.f) {
                auto mag_1 = 1 / mag;
                auto p = d * mag_1 * overlap;
                cPos -= p;

                return true;
            }
            return false;
        }

        inline bool MoveCircleIfIntersectsBox2(XY bPos, float bHalfWidth, float bHalfHeight, XY& cPos, float cr) {
            XY siz{ bHalfWidth, bHalfHeight };
            return MoveCircleIfIntersectsBox2({ {bPos - siz}, {bPos + siz} }, cPos, cr);
        }

        // xy.from: min xy . to: max xy.
        inline bool BounceCircleIfIntersectsBox(FromTo<XY> const& xy, float radius, float speed, XY& inc, XY& newPos) {
            // find rect nearest point
            XY np{ std::max(xy.from.x, std::min(newPos.x, xy.to.x)),
            std::max(xy.from.y, std::min(newPos.y, xy.to.y)) };

            // calc
            auto d = np - newPos;
            auto mag = std::sqrt(d.x * d.x + d.y * d.y);
            auto overlap = radius - mag;

            // intersect
            if (overlap > 0 && mag != 0.f) {
                auto mag_1 = 1 / mag;
                auto p = d * mag_1 * overlap;

                // bounce
                if (np.x == newPos.x) {
                    inc.y *= -1;
                } else if (np.y == newPos.y) {
                    inc.x *= -1;
                } else {
                    auto a1 = std::atan2(-inc.y, -inc.x);
                    auto a2 = std::atan2(-p.y, -p.x);
                    auto gap = RotateControl::Gap(a1, a2);
                    a2 += gap;	// todo?
                    inc = XY{ std::cos(a2) * speed, std::sin(a2) * speed };
                }

                newPos -= p;
                return true;
            }
            return false;
        }


    }

    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/

    namespace Calc {

        inline XX_INLINE float DistancePow2(float x1, float y1, float x2, float y2) {
            return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
        }

        inline XX_INLINE float Distance(float x1, float y1, float x2, float y2) {
            return std::sqrt(DistancePow2(x1, y1, x2, y2));
        }

        inline XX_INLINE float DistancePow2(XY const& p1, XY const& p2) {
            return DistancePow2(p1.x, p1.y, p2.x, p2.y);
        }

        inline XX_INLINE float Distance(XY const& p1, XY const& p2) {
            return Distance(p1.x, p1.y, p2.x, p2.y);
        }

        inline XX_INLINE float DistanceLimit(float d, float from, float to) {
            if (d < from) return from;
            else if (d > to) return to;
            else return d;
        }

        /*
            XY p, c;
            ...
            p2 = RotatePoint(c - p, 1.23) + c;
        */
        inline XX_INLINE XY RotatePoint(XY const& d, float radians) {
            auto c = std::cos(radians);
            auto s = std::sin(radians);
            return { d.x * c - d.y * s, d.x * s + d.y * c };
        }

        // copy from cocos
        // Bezier cubic formula:
        //    ((1 - t) + t)3 = 1 
        // Expands to ...
        //   (1 - t)3 + 3t(1-t)2 + 3t2(1 - t) + t3 = 1 
        inline XX_INLINE float Bezierat(float a, float b, float c, float d, float t) {
            return (std::pow(1.f - t, 3.f) * a +
                3.f * t * (std::pow(1.f - t, 2.f)) * b +
                3.f * std::pow(t, 2.f) * (1.f - t) * c +
                std::pow(t, 3.f) * d);
        }

        namespace Intersects {

            // b: box
            template<typename XY1, typename XY2>
            XX_INLINE bool BoxPoint(XY1 const& b1minXY, XY1 const& b1maxXY, XY2 const& p) {
                return !(b1maxXY.x < p.x || p.x < b1minXY.x || b1maxXY.y < p.y || p.y < b1minXY.y);
            }

            // b: box
            template<typename XY1, typename XY2>
            XX_INLINE bool BoxBox(XY1 const& b1minXY, XY1 const& b1maxXY, XY2 const& b2minXY, XY2 const& b2maxXY) {
                return !(b1maxXY.x < b2minXY.x || b2maxXY.x < b1minXY.x || b1maxXY.y < b2minXY.y || b2maxXY.y < b1minXY.y);
            }

            // b: box    c: circle    r: radius
            // if intersect return true
            template<typename T = int32_t>
            bool BoxCircle(T bx, T by, T bHalfWidth, T bHalfHeight, T cx, T cy, T cr) {
                auto dx = std::abs(cx - bx);
                if (dx > bHalfWidth + cr) return false;

                auto dy = std::abs(cy - by);
                if (dy > bHalfHeight + cr) return false;

                if (dx <= bHalfWidth || dy <= bHalfHeight) return true;

                auto dx2 = dx - bHalfWidth;
                auto dy2 = dy - bHalfHeight;
                return dx2 * dx2 + dy2 * dy2 <= cr * cr;
            }

            // reference here:
            // http://www.jeffreythompson.org/collision-detection/poly-circle.php

            template<typename T = float>
            XX_INLINE bool CircleCircle(T c1x, T c1y, T c1r, T c2x, T c2y, T c2r) {
                auto dx = c1x - c2x;
                auto dy = c1y - c2y;
                auto rr = c1r + c2r;
                return (dx * dx) + (dy * dy) <= rr * rr;
            }

            template<typename T = float>
            XX_INLINE bool PointCircle(T px, T py, T cx, T cy, T r) {
                auto dx = px - cx;
                auto dy = py - cy;
                return (dx * dx) + (dy * dy) <= r * r;
            }

            inline XX_INLINE bool LinePoint(float x1, float y1, float x2, float y2, float px, float py) {
                float d1 = Calc::Distance(px, py, x1, y1);
                float d2 = Calc::Distance(px, py, x2, y2);
                float lineLen = Calc::Distance(x1, y1, x2, y2);
                return d1 + d2 >= lineLen - 0.1 && d1 + d2 <= lineLen + 0.1;
            }

            inline bool LineCircle(float x1, float y1, float x2, float y2, float cx, float cy, float r) {
                if (PointCircle(x1, y1, cx, cy, r) || PointCircle(x2, y2, cx, cy, r)) return true;
                float dx = x1 - x2;
                float dy = y1 - y2;
                float dot = (((cx - x1) * (x2 - x1)) + ((cy - y1) * (y2 - y1))) / ((dx * dx) + (dy * dy));
                float closestX = x1 + (dot * (x2 - x1));
                float closestY = y1 + (dot * (y2 - y1));
                if (!LinePoint(x1, y1, x2, y2, closestX, closestY)) return false;
                dx = closestX - cx;
                dy = closestY - cy;
                return (dx * dx) + (dy * dy) <= r * r;
            }

            template<bool vsEndIsFirst = false, typename Vecs>
            bool PolygonPoint(Vecs const& vs, float px, float py) {
                bool collision{};
                for (int curr = 0, next = 1, e = vsEndIsFirst ? (int)std::size(vs) - 1 : (int)std::size(vs); curr < e; ++curr, ++next) {
                    if constexpr (!vsEndIsFirst) {
                        if (next == e) next = 0;
                    }
                    auto& vc = vs[curr];
                    auto& vn = vs[next];
                    if (((vc.y > py && vn.y < py) || (vc.y < py && vn.y > py)) &&
                        (px < (vn.x - vc.x) * (py - vc.y) / (vn.y - vc.y) + vc.x)) {
                        collision = !collision;
                    }
                }
                return collision;
            }

            template<bool checkInside = true, bool vsEndIsFirst = true, typename Vecs>
            bool PolyCircle(Vecs const& vs, float cx, float cy, float r) {
                for (int curr = 0, next = 1, e = vsEndIsFirst ? (int)std::size(vs) - 1 : (int)std::size(vs); curr < e; ++curr, ++next) {
                    if constexpr (!vsEndIsFirst) {
                        if (next == e) next = 0;
                    }
                    if (LineCircle(vs[curr].x, vs[curr].y, vs[next].x, vs[next].y, cx, cy, r)) return true;
                }
                if constexpr (checkInside) {
                    if (PolygonPoint<vsEndIsFirst>(vs, cx, cy)) return true;
                }
                return false;
            }

        }
    }




    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/

    namespace Math {

        template<typename T = XYp, class = std::enable_if_t<xx::IsX_Y_v<T>>>
        XX_INLINE auto SqrMagnitude(T const& p) -> decltype(T::x) {
            return p.x * p.x + p.y * p.y;
        }

        template<typename T = XYp, class = std::enable_if_t<xx::IsX_Y_v<T>>>
        XX_INLINE  auto Dot(T const& a, T const& b) -> decltype(T::x) {
            return a.x * b.x + a.y * b.y;
        }

        template<typename T = FX64>
        XX_INLINE T Clamp01(T const& v) {
            if constexpr (std::is_base_of_v<FX64, T>) {
                return v.Clamp01();
            } else {
                return { (v > (T)1) ? (T)1 : (v < (T)0) ? (T)0 : v };
            }
        }

        template<typename T = XYp, class = std::enable_if_t<xx::IsX_Y_v<T>>>
        XX_INLINE auto SegmentPointSqrDistance(T const& u, T const& x) -> decltype(T::x) {
            auto t = Dot(x, u) / SqrMagnitude(u);
            return SqrMagnitude(x - (Clamp01(t) * u));
        }

        template<typename T = FX64>
        XX_INLINE T Abs(T const& v) {
            if constexpr (std::is_base_of_v<FX64, T>) {
                return v.Abs();
            } else {
                return std::abs(v);
            }
        }

        template<typename T = FX64>
        XX_INLINE T Sqrt(T const& v) {
            if constexpr (std::is_base_of_v<FX64, T>) {
                return v.Sqrt();
            } else {
                return std::sqrt(v);
            }
        }

        template<typename T = FX64>
        XX_INLINE T Sin(T const& v) {
            if constexpr (std::is_base_of_v<FX64, T>) {
                return v.Sin();
            } else {
                return std::sin(v);
            }
        }

        template<typename T = FX64>
        XX_INLINE T Cos(T const& v) {
            if constexpr (std::is_base_of_v<FX64, T>) {
                return v.Cos();
            } else {
                return std::cos(v);
            }
        }

        template<typename T = FX64>
        XX_INLINE T Atan2(T const& v) {
            if constexpr (std::is_base_of_v<FX64, T>) {
                return v.Atan2();
            } else {
                return std::atan2(v);
            }
        }

        // ...

        // https://github.com/CharlesFeng207/Unity-Math-Utils/blob/main/Unity-Math-Utils/Assets/MathUtils.cs

        // sRadiansCosSin == T{ Cos(sRadians), Sin(sRadians) }

        template<typename T = XYp, typename U = decltype(T::x), class = std::enable_if_t<xx::IsX_Y_v<T>>>
        bool IsSectorCircleIntersect(T const& sPos, U sRadius, T const& sRadiansCosSin, U sTheta, T const& cPos, U cRadius) {
            // 1. circle circle check
            auto d = cPos - sPos;
            auto rr = sRadius + cRadius;
            auto d_sm = SqrMagnitude(d);
            if (d_sm > rr * rr) return false;

            // 2. calc local space p
            auto px = Dot(d, sRadiansCosSin);
            auto py = Abs(Dot(d, T{ -sRadiansCosSin.y, sRadiansCosSin.x }));

            // 3. if p_x > ||p|| cos sTheta: cross
            auto theta_cos = Cos(sTheta);
            if (px > Sqrt(d_sm) * theta_cos) return true;

            // 4. check segment circle cross
            auto q = T{ theta_cos, Sin(sTheta) } * sRadius;
            return SegmentPointSqrDistance(q, T{ px, py }) <= cRadius * cRadius;
        }










        // b: box    c: circle    w: width    h: height    r: radius
        // if intersect, cx & cy will be changed & return true
        template<typename T, bool canUp, bool canRight, bool canDown, bool canLeft>
        XX_INLINE bool MoveCircleIfIntersectsBox_core(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner = false) {
            static_assert(canUp || canRight || canDown || canLeft);

            auto dx = std::abs(cx - bx);
            if (dx > bHalfWidth + cr) return false;

            auto dy = std::abs(cy - by);
            if (dy > bHalfHeight + cr) return false;

            if (dx <= bHalfWidth || dy <= bHalfHeight) {
                if constexpr (canUp && canRight && canDown && canLeft) {
                    if (bHalfWidth - dx > bHalfHeight - dy) {
                        if (by > cy) {
                            cy = by - bHalfHeight - cr - 2;	// top
                        } else {
                            cy = by + bHalfHeight + cr + 2;	// bottom
                        }
                    } else {
                        if (bx > cx) {
                            cx = bx - bHalfWidth - cr - 2;	// left
                        } else {
                            cx = bx + bHalfWidth + cr + 2;	// right
                        }
                    }
                } else if constexpr (canUp && canRight && canDown) {
                    if (cx > bx) {
                        if (cy > by) {
                            if ((bx + bHalfWidth) - cx > (by + bHalfHeight) - cy) {
                                cy = by + bHalfHeight + cr + 2;	// bottom
                            } else {
                                cx = bx + bHalfWidth + cr + 2;	// right
                            }
                        } else {
                            if ((bx + bHalfWidth) - cx > cy - (by - bHalfHeight)) {
                                cy = by - bHalfHeight - cr - 2;	// top
                            } else {
                                cx = bx + bHalfWidth + cr + 2;	// right
                            }
                        }
                    } else if (by > cy) {
                        cy = by - bHalfHeight - cr - 2;	// top
                    } else {
                        cy = by + bHalfHeight + cr + 2;	// bottom
                    }
                } else if constexpr (canRight && canDown && canLeft) {
                    if (cy > by) {
                        if (cx > bx) {
                            if ((bx + bHalfWidth) - cx > (by + bHalfHeight) - cy) {
                                cy = by + bHalfHeight + cr + 2;	// bottom
                            } else {
                                cx = bx + bHalfWidth + cr + 2;	// right
                            }
                        } else {
                            if (cx - (bx - bHalfWidth) > (by + bHalfHeight) - cy) {
                                cy = by + bHalfHeight + cr + 2;	// bottom
                            } else {
                                cx = bx - bHalfWidth - cr - 2;	// left
                            }
                        }
                    } else if (cx < bx) {
                        cx = bx - bHalfWidth - cr - 2;	// left
                    } else {
                        cx = bx + bHalfWidth + cr + 2;	// right
                    }
                } else if constexpr (canDown && canLeft && canUp) {
                    if (cx < bx) {
                        if (cy > by) {
                            if (cx - (bx - bHalfWidth) > (by + bHalfHeight) - cy) {
                                cy = by + bHalfHeight + cr + 2;	// bottom
                            } else {
                                cx = bx - bHalfWidth - cr - 2;	// left
                            }
                        } else {
                            if (cx - (bx - bHalfWidth) > cy - (by - bHalfHeight)) {
                                cy = by - bHalfHeight - cr - 2;	// top
                            } else {
                                cx = bx - bHalfWidth - cr - 2;	// left
                            }
                        }
                    } else if (by > cy) {
                        cy = by - bHalfHeight - cr - 2;	// top
                    } else {
                        cy = by + bHalfHeight + cr + 2;	// bottom
                    }
                } else if constexpr (canLeft && canUp && canRight) {
                    if (cy < by) {
                        if (cx > bx) {
                            if ((bx + bHalfWidth) - cx > cy - (by - bHalfHeight)) {
                                cy = by - bHalfHeight - cr - 2;	// top
                            } else {
                                cx = bx + bHalfWidth + cr + 2;	// right
                            }
                        } else {
                            if (cx - (bx - bHalfWidth) > cy - (by - bHalfHeight)) {
                                cy = by - bHalfHeight - cr - 2;	// top
                            } else {
                                cx = bx - bHalfWidth - cr - 2;	// left
                            }
                        }
                    } else if (cx < bx) {
                        cx = bx - bHalfWidth - cr - 2;	// left
                    } else {
                        cx = bx + bHalfWidth + cr + 2;	// right
                    }
                } else if constexpr (canLeft && canUp) {
                    if (isMapCorner) {
                        if (cx - (bx - bHalfWidth) > cy - (by - bHalfHeight)) {
                            cy = by - bHalfHeight - cr - 2;	// top
                        } else {
                            cx = bx - bHalfWidth - cr - 2;	// left
                        }
                    } else {
                        if (cx <= bx && cy <= by) {
                            if (cx - (bx - bHalfWidth) > cy - (by - bHalfHeight)) {
                                cy = by - bHalfHeight - cr - 2;	// top
                            } else {
                                cx = bx - bHalfWidth - cr - 2;	// left
                            }
                        } else if (cx <= bx) {
                            cx = bx - bHalfWidth - cr - 2;	// left
                        } else if (cy <= by) {
                            cy = by - bHalfHeight - cr - 2;	// top
                        }
                    }
                } else if constexpr (canUp && canRight) {
                    if (isMapCorner) {
                        if ((bx + bHalfWidth) - cx > cy - (by - bHalfHeight)) {
                            cy = by - bHalfHeight - cr - 2;	// top
                        } else {
                            cx = bx + bHalfWidth + cr + 2;	// right
                        }
                    } else {
                        if (cx >= bx && cy <= by) {
                            if ((bx + bHalfWidth) - cx > cy - (by - bHalfHeight)) {
                                cy = by - bHalfHeight - cr - 2;	// top
                            } else {
                                cx = bx + bHalfWidth + cr + 2;	// right
                            }
                        } else if (cx >= bx) {
                            cx = bx + bHalfWidth + cr + 2;	// right
                        } else if (cy <= by) {
                            cy = by - bHalfHeight - cr - 2;	// top
                        }
                    }
                } else if constexpr (canRight && canDown) {
                    if (isMapCorner) {
                        if ((bx + bHalfWidth) - cx > (by + bHalfHeight) - cy) {
                            cy = by + bHalfHeight + cr + 2;	// bottom
                        } else {
                            cx = bx + bHalfWidth + cr + 2;	// right
                        }
                    } else {
                        if (cx >= bx && cy >= by) {
                            if ((bx + bHalfWidth) - cx > (by + bHalfHeight) - cy) {
                                cy = by + bHalfHeight + cr + 2;	// bottom
                            } else {
                                cx = bx + bHalfWidth + cr + 2;	// right
                            }
                        } else if (cx >= bx) {
                            cx = bx + bHalfWidth + cr + 2;	// right
                        } else if (cy >= by) {
                            cy = by + bHalfHeight + cr + 2;	// bottom
                        }
                    }
                } else if constexpr (canDown && canLeft) {
                    if (isMapCorner) {
                        if (cx - (bx - bHalfWidth) > (by + bHalfHeight) - cy) {
                            cy = by + bHalfHeight + cr + 2;	// bottom
                        } else {
                            cx = bx - bHalfWidth - cr - 2;	// left
                        }
                    } else {
                        if (cx <= bx && cy >= by) {
                            if (cx - (bx - bHalfWidth) > (by + bHalfHeight) - cy) {
                                cy = by + bHalfHeight + cr + 2;	// bottom
                            } else {
                                cx = bx - bHalfWidth - cr - 2;	// left
                            }
                        } else if (cx <= bx) {
                            cx = bx - bHalfWidth - cr - 2;	// left
                        } else if (cy >= by) {
                            cy = by + bHalfHeight + cr + 2;	// bottom
                        }
                    }
                } else if constexpr (canLeft && canRight) {
                    if (bx > cx) {
                        cx = bx - bHalfWidth - cr - 2;	// left
                    } else {
                        cx = bx + bHalfWidth + cr + 2;	// right
                    }
                } else if constexpr (canUp && canDown) {
                    if (by > cy) {
                        cy = by - bHalfHeight - cr - 2;	// top
                    } else {
                        cy = by + bHalfHeight + cr + 2;	// bottom
                    }
                } else if constexpr (canUp) {
                    cy = by - bHalfHeight - cr - 2;	// top
                } else if constexpr (canRight) {
                    cx = bx + bHalfWidth + cr + 2;	// right
                } else if constexpr (canDown) {
                    cy = by + bHalfHeight + cr + 2;	// bottom
                } else if constexpr (canLeft) {
                    cx = bx - bHalfWidth - cr - 2;	// left
                }
                return true;
            }

            auto dx2 = dx - bHalfWidth;
            auto dy2 = dy - bHalfHeight;
            if (dx2 * dx2 + dy2 * dy2 <= cr * cr) {
                // change cx & cy
                auto incX = dx2, incY = dy2;
                auto dSeq = dx2 * dx2 + dy2 * dy2;
                if (dSeq == T{}) {
                    if constexpr (std::is_integral_v<T>) {
                        incX = bHalfWidth + cr * 7071 / 10000 + 2;
                        incY = bHalfHeight + cr * 7071 / 10000 + 2;
                    } else {
                        incX = bHalfWidth + cr * T{ 0.7071 } + 2;
                        incY = bHalfHeight + cr * T{ 0.7071 } + 2;
                    }
                } else {
                    if constexpr (std::is_integral_v<T>) {
                        auto d = FX64{ dSeq }.RSqrtFastest();
                        incX = bHalfWidth + (FX64{ cr * dx2 } *d).ToInt() + 2;
                        incY = bHalfHeight + (FX64{ cr * dy2 } *d).ToInt() + 2;
                    } else {
                        auto d = std::sqrt(dSeq);
                        incX = bHalfWidth + cr * dx2 / d + 2;
                        incY = bHalfHeight + cr * dy2 / d + 2;
                    }
                }

                if constexpr (canUp && canRight && canDown && canLeft) {
                    if (cx < bx) {
                        incX = -incX;
                    }
                    if (cy < by) {
                        incY = -incY;
                    }
                    cx = bx + incX;
                    cy = by + incY;
                } else if constexpr (canUp && canRight && canDown) {
                    if (cx < bx) {
                        incX = 0;
                    }
                    if (cy < by) {
                        incY = -incY;
                    }
                    cx = bx + incX;
                    cy = by + incY;
                } else if constexpr (canRight && canDown && canLeft) {
                    if (cx < bx) {
                        incX = -incX;
                    }
                    if (cy < by) {
                        incY = 0;
                    }
                    cx = bx + incX;
                    cy = by + incY;
                } else if constexpr (canDown && canLeft && canUp) {
                    if (cx < bx) {
                        incX = -incX;
                    } else {
                        incX = 0;
                    }
                    if (cy < by) {
                        incY = -incY;
                    }
                    cx = bx + incX;
                    cy = by + incY;
                } else if constexpr (canLeft && canUp && canRight) {
                    if (cx < bx) {
                        incX = -incX;
                    }
                    if (cy < by) {
                        incY = -incY;
                    } else {
                        incY = 0;
                    }
                    cx = bx + incX;
                    cy = by + incY;
                } else if constexpr (canLeft && canUp) {
                    cx = bx - incX;
                    cy = by - incY;
                } else if constexpr (canUp && canRight) {
                    cx = bx + incX;
                    cy = by - incY;
                } else if constexpr (canRight && canDown) {
                    cx = bx + incX;
                    cy = by + incY;
                } else if constexpr (canDown && canLeft) {
                    cx = bx - incX;
                    cy = by + incY;
                } else if constexpr (canLeft && canRight) {
                    if (bx > cx) {
                        cx = bx - bHalfWidth - cr - 2;	// left
                    } else {
                        cx = bx + bHalfWidth + cr + 2;	// right
                    }
                } else if constexpr (canUp && canDown) {
                    if (by > cy) {
                        cy = by - bHalfHeight - cr - 2;	// top
                    } else {
                        cy = by + bHalfHeight + cr + 2;	// bottom
                    }
                } else if constexpr (canUp) {
                    cy = by - bHalfHeight - cr - 2;	// top
                } else if constexpr (canRight) {
                    cx = bx + bHalfWidth + cr + 2;	// right
                } else if constexpr (canDown) {
                    cy = by + bHalfHeight + cr + 2;	// bottom
                } else if constexpr (canLeft) {
                    cx = bx - bHalfWidth - cr - 2;	// left
                }

                return true;
            }
            return false;
        }

        // up
        template<typename T> bool MoveCircleIfIntersectsBox1(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, false, false, false>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // right
        template<typename T> bool MoveCircleIfIntersectsBox2(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, false, true, false, false>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // up + right
        template<typename T> bool MoveCircleIfIntersectsBox3(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, true, false, false>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // down
        template<typename T> bool MoveCircleIfIntersectsBox4(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, false, false, true, false>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // up + down
        template<typename T> bool MoveCircleIfIntersectsBox5(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, false, true, false>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // right + down
        template<typename T> bool MoveCircleIfIntersectsBox6(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, false, true, true, false>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // up + right + down
        template<typename T> bool MoveCircleIfIntersectsBox7(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, true, true, false>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // left
        template<typename T> bool MoveCircleIfIntersectsBox8(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, false, false, false, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // up + left
        template<typename T> bool MoveCircleIfIntersectsBox9(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, false, false, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // right + left
        template<typename T> bool MoveCircleIfIntersectsBox10(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, false, true, false, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // up + right + left
        template<typename T> bool MoveCircleIfIntersectsBox11(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, true, false, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // down + left
        template<typename T> bool MoveCircleIfIntersectsBox12(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, false, false, true, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // up + down + left
        template<typename T> bool MoveCircleIfIntersectsBox13(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, false, true, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // right + down + left
        template<typename T> bool MoveCircleIfIntersectsBox14(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, false, true, true, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }
        // up + right + down + left
        template<typename T> bool MoveCircleIfIntersectsBox15(T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            return MoveCircleIfIntersectsBox_core<T, true, true, true, true>(bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
        }

        typedef bool(*MoveCircleIfIntersectsBoxFuncInt32)(int32_t bx, int32_t by, int32_t bHalfWidth, int32_t bHalfHeight, int32_t& cx, int32_t& cy, int32_t cr, bool isMapCorner);
        inline static MoveCircleIfIntersectsBoxFuncInt32 moveCircleIfIntersectsBoxFuncs_int32[] = {
            MoveCircleIfIntersectsBox15<int32_t>,
            MoveCircleIfIntersectsBox1<int32_t>,
            MoveCircleIfIntersectsBox2<int32_t>,
            MoveCircleIfIntersectsBox3<int32_t>,
            MoveCircleIfIntersectsBox4<int32_t>,
            MoveCircleIfIntersectsBox5<int32_t>,
            MoveCircleIfIntersectsBox6<int32_t>,
            MoveCircleIfIntersectsBox7<int32_t>,
            MoveCircleIfIntersectsBox8<int32_t>,
            MoveCircleIfIntersectsBox9<int32_t>,
            MoveCircleIfIntersectsBox10<int32_t>,
            MoveCircleIfIntersectsBox11<int32_t>,
            MoveCircleIfIntersectsBox12<int32_t>,
            MoveCircleIfIntersectsBox13<int32_t>,
            MoveCircleIfIntersectsBox14<int32_t>,
            MoveCircleIfIntersectsBox15<int32_t>,
        };

        typedef bool(*MoveCircleIfIntersectsBoxFuncFloat)(float bx, float by, float bHalfWidth, float bHalfHeight, float& cx, float& cy, float cr, bool isMapCorner);
        inline static MoveCircleIfIntersectsBoxFuncFloat moveCircleIfIntersectsBoxFuncs_float[] = {
            MoveCircleIfIntersectsBox15<float>,
            MoveCircleIfIntersectsBox1<float>,
            MoveCircleIfIntersectsBox2<float>,
            MoveCircleIfIntersectsBox3<float>,
            MoveCircleIfIntersectsBox4<float>,
            MoveCircleIfIntersectsBox5<float>,
            MoveCircleIfIntersectsBox6<float>,
            MoveCircleIfIntersectsBox7<float>,
            MoveCircleIfIntersectsBox8<float>,
            MoveCircleIfIntersectsBox9<float>,
            MoveCircleIfIntersectsBox10<float>,
            MoveCircleIfIntersectsBox11<float>,
            MoveCircleIfIntersectsBox12<float>,
            MoveCircleIfIntersectsBox13<float>,
            MoveCircleIfIntersectsBox14<float>,
            MoveCircleIfIntersectsBox15<float>,
        };

        struct BlockWayout {
            uint8_t up : 1;			// 1
            uint8_t right : 1;		// 2
            uint8_t down : 1;		// 4
            uint8_t left : 1;		// 8
        };

        template<typename T>
        bool MoveCircleIfIntersectsBox(BlockWayout bw, T bx, T by, T bHalfWidth, T bHalfHeight, T& cx, T& cy, T cr, bool isMapCorner) {
            static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, float>);
            auto idx = (uint8_t&)bw;
            if constexpr (std::is_same_v<T, int32_t>) {
                assert(idx < _countof(moveCircleIfIntersectsBoxFuncs_int32));
                return moveCircleIfIntersectsBoxFuncs_int32[idx](bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
            } else {
                assert(idx < _countof(moveCircleIfIntersectsBoxFuncs_float));
                return moveCircleIfIntersectsBoxFuncs_float[idx](bx, by, bHalfWidth, bHalfHeight, cx, cy, cr, isMapCorner);
            }
        }


    }

}
