#pragma once
#include "xx_xy.h"

namespace xx {

    inline static constexpr float gPI{ (float)M_PI }, gNPI{ -gPI }, g2PI{ gPI * 2 }, gPI_2{ gPI / 2 };
    inline static constexpr float gSQRT2{ 1.414213562373095f }, gSQRT2_1 { 0.7071067811865475f };

    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/

    namespace FrameControl {

        inline XX_FORCE_INLINE void Forward(float& frameIndex, float inc, float from, float to) {
            frameIndex += inc;
            if (frameIndex >= to) {
                frameIndex = from + (frameIndex - to);
            }
        }

        inline XX_FORCE_INLINE void Forward(float& frameIndex, float inc, float to) {
            Forward(frameIndex, inc, 0, to);
        }

        inline XX_FORCE_INLINE	void Backward(float& frameIndex, float inc, float from, float to) {
            frameIndex -= inc;
            if (frameIndex <= from) {
                frameIndex = to - (from - frameIndex);
            }
        }

        inline XX_FORCE_INLINE	void Backward(float& frameIndex, float inc, float to) {
            Backward(frameIndex, inc, 0, to);
        }

    }

    /*******************************************************************************************************************************************/
    /*******************************************************************************************************************************************/


    namespace RotateControl {

        inline XX_FORCE_INLINE float Gap(float tar, float cur) {
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
        inline XX_FORCE_INLINE float LerpAngleByRate(float tar, float cur, float rate) {
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
        inline XX_FORCE_INLINE float LerpAngleByFixed(float tar, float cur, float a) {
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
        inline XX_FORCE_INLINE bool Step(float& cur, float tar, float a) {
            return (cur = LerpAngleByFixed(tar, cur, a)) == tar;
        }

        // limit a by from ~ to
        // no change: return false
        inline XX_FORCE_INLINE bool Limit(float& a, float from, float to) {
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

        inline XX_FORCE_INLINE float DistancePow2(float x1, float y1, float x2, float y2) {
            return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
        }

        inline XX_FORCE_INLINE float Distance(float x1, float y1, float x2, float y2) {
            return std::sqrt(DistancePow2(x1, y1, x2, y2));
        }

        inline XX_FORCE_INLINE float DistancePow2(XY const& p1, XY const& p2) {
            return DistancePow2(p1.x, p1.y, p2.x, p2.y);
        }

        inline XX_FORCE_INLINE float Distance(XY const& p1, XY const& p2) {
            return Distance(p1.x, p1.y, p2.x, p2.y);
        }

        inline XX_FORCE_INLINE float DistanceLimit(float d, float from, float to) {
            if (d < from) return from;
            else if (d > to) return to;
            else return d;
        }

        /*
            XY p, c;
            ...
            p2 = RotatePoint(c - p, 1.23) + c;
        */
        inline XX_FORCE_INLINE XY RotatePoint(XY const& d, float radians) {
            auto c = std::cos(radians);
            auto s = std::sin(radians);
            return { d.x * c - d.y * s, d.x * s + d.y * c };
        }

        // copy from cocos
        // Bezier cubic formula:
        //    ((1 - t) + t)3 = 1 
        // Expands to ...
        //   (1 - t)3 + 3t(1-t)2 + 3t2(1 - t) + t3 = 1 
        inline XX_FORCE_INLINE float Bezierat(float a, float b, float c, float d, float t) {
            return (std::pow(1.f - t, 3.f) * a +
                3.f * t * (std::pow(1.f - t, 2.f)) * b +
                3.f * std::pow(t, 2.f) * (1.f - t) * c +
                std::pow(t, 3.f) * d);
        }

        namespace Intersects {

            // b: box
            template<typename XY1, typename XY2>
            XX_FORCE_INLINE bool BoxPoint(XY1 const& b1minXY, XY1 const& b1maxXY, XY2 const& p) {
                return !(b1maxXY.x < p.x || p.x < b1minXY.x || b1maxXY.y < p.y || p.y < b1minXY.y);
            }

            // b: box
            template<typename XY1, typename XY2>
            XX_FORCE_INLINE bool BoxBox(XY1 const& b1minXY, XY1 const& b1maxXY, XY2 const& b2minXY, XY2 const& b2maxXY) {
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
            XX_FORCE_INLINE bool CircleCircle(T c1x, T c1y, T c1r, T c2x, T c2y, T c2r) {
                auto dx = c1x - c2x;
                auto dy = c1y - c2y;
                auto rr = c1r + c2r;
                return (dx * dx) + (dy * dy) <= rr * rr;
            }

            template<typename T = float>
            XX_FORCE_INLINE bool PointCircle(T px, T py, T cx, T cy, T r) {
                auto dx = px - cx;
                auto dy = py - cy;
                return (dx * dx) + (dy * dy) <= r * r;
            }

            inline XX_FORCE_INLINE bool LinePoint(float x1, float y1, float x2, float y2, float px, float py) {
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

}
