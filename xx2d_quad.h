#pragma once
#include <xx2d_framebuffer.h>

namespace xx {

    // 2d rect picture draw class. for shaderQuadInstance
    struct Quad : QuadInstanceData {
        Ref<Frame> frame;
        GLuint texId{};   // cache: == *frame->tex

        template<bool forceOverrideTexRectId = false>
        XX_INLINE Quad& SetFrame(Ref<Frame> f) {
            assert(f);
            assert(f->tex);
            if constexpr (!forceOverrideTexRectId) {
                if (frame == f) return *this;
            }
            if (f->textureRotated) {
                texRect.x = f->textureRect.x;
                texRect.y = f->textureRect.y;
                texRect.w = f->textureRect.h;
                texRect.h = f->textureRect.w;
            } else {
                texRect.data = f->textureRect.data;
            }
            texId = f->tex->GetValue();
            frame = std::move(f);
            return *this;
        }

        XX_INLINE Quad& ClearFrame() {
            frame.Reset();
            texId = 0;
            return *this;
        }

        operator bool() const {
            return !!texId;
        }

        XX_INLINE XY Size() const {
            assert(frame);
            return { (float)texRect.w, (float)texRect.h };
        }

        XX_INLINE XY GetSize() const {
            return Size();
        }

        XX_INLINE XY GetSizeScaled() const {
            return Size() * scale;
        }

        // tp support: .SetAnchor( anchor - (f->spriteOffset / f->spriteSize) )     // todo: fix
        XX_INLINE Quad& SetAnchor(XY const& a) {
            anchor = a;
            return *this;
        }
        // tp support: .SetRotate( radians + (f->textureRotated ? gNPI / 2 : 0.f) )
        XX_INLINE Quad& SetRotate(float r) {
            radians = r;
            return *this;
        }
        XX_INLINE Quad& AddRotate(float r) {
            radians += r;
            return *this;
        }
        XX_INLINE Quad& SetScale(XY const& s) {
            scale = s;
            return *this;
        }
        XX_INLINE Quad& SetScale(float s) {
            scale = { s, s };
            return *this;
        }
        XX_INLINE Quad& AddScale(XY const& s) {
            scale += s;
            return *this;
        }
        XX_INLINE Quad& AddScale(float s) {
            scale += XY{ s, s };
            return *this;
        }
        XX_INLINE Quad& SetPosition(XY const& p) {
            pos = p;
            return *this;
        }
        XX_INLINE Quad& SetPositionX(float x) {
            pos.x = x;
            return *this;
        }
        XX_INLINE Quad& SetPositionY(float y) {
            pos.y = y;
            return *this;
        }
        XX_INLINE Quad& AddPosition(XY const& p) {
            pos += p;
            return *this;
        }
        XX_INLINE Quad& AddPositionX(float x) {
            pos.x += x;
            return *this;
        }
        XX_INLINE Quad& AddPositionY(float y) {
            pos.y += y;
            return *this;
        }
        XX_INLINE Quad& SetColor(RGBA8 c) {
            color = c;
            return *this;
        }
        XX_INLINE Quad& SetColorA(uint8_t a) {
            color.a = a;
            return *this;
        }
        XX_INLINE Quad& SetColorAf(float a) {
            color.a = uint8_t(255 * a);
            return *this;
        }
        XX_INLINE Quad& SetColorplus(float v) {
            colorplus = v;
            return *this;
        }
        XX_INLINE Quad& Draw() const {
            assert(texId);
            auto& eg = EngineBase1::Instance();
            eg.ShaderBegin(eg.shaderQuadInstance).Draw(texId, *this);
            return (Quad&)*this;
        }

        /*
        auto& q = DrawOnce( frame );
        auto a = frame->spriteOffset / frame->spriteSize;   // todo: fix
        auto r = frame->textureRotated ? gNPI / 2 : 0.f;
        q.pos = {};
        q.anchor = anchor - a;
        q.scale = {1, 1};
        q.radians = radians + r;
        q.colorplus = 1;
        q.color = {255, 255, 255, 255};
        */
        XX_INLINE static QuadInstanceData& DrawOnce(Ref<Frame> const& f) {
            auto& eg = EngineBase1::Instance();
            auto& r = *eg.ShaderBegin(eg.shaderQuadInstance).Draw(f->tex->GetValue(), 1);
            if (f->textureRotated) {
                r.texRect.x = f->textureRect.x;
                r.texRect.y = f->textureRect.y;
                r.texRect.w = f->textureRect.h;
                r.texRect.h = f->textureRect.w;
            } else {
                r.texRect.data = f->textureRect.data;
            }
            return r;
        }

        XX_INLINE static QuadInstanceData* DrawMore(Ref<Frame> const& f, int32_t count) {
            assert(count > 0);
            auto& eg = EngineBase1::Instance();
            auto r = eg.ShaderBegin(eg.shaderQuadInstance).Draw(f->tex->GetValue(), count);
            if (f->textureRotated) {
                for (int32_t i = 0; i < count; ++i) {
                    r[i].texRect.x = f->textureRect.x;
                    r[i].texRect.y = f->textureRect.y;
                    r[i].texRect.w = f->textureRect.h;
                    r[i].texRect.h = f->textureRect.w;
                }
            } else {
                for (int32_t i = 0; i < count; ++i) {
                    r[i].texRect.data = f->textureRect.data;
                }
            }
            return r;
        }
    };

    // mem moveable tag
    template<>
    struct IsPod<Quad, void> : std::true_type {};

}
