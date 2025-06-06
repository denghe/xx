﻿#pragma once
#include <xx2d_base1.h>

namespace xx {

    struct FrameBuffer {
        GLFrameBuffer fb;
        XY bakWndSiz{};
        std::array<uint32_t, 3> bakBlend{};
        XYu bakTexSiz{};

        // need ogl frame env
        explicit FrameBuffer(bool autoInit = false) {
            if (autoInit) {
                Init();
            }
        }

        // need ogl frame env
        FrameBuffer& Init() {
            xx_assert(!fb);   // excessive initializations ?
            fb = MakeGLFrameBuffer();
            return *this;
        }

        inline static Ref<GLTexture> MakeTexture(XYu const& wh, bool hasAlpha = true) {
            return MakeRef<GLTexture>(GLTexture::Create(wh.x, wh.y, hasAlpha));
        }

        template<typename Func>
        void DrawTo(Ref<GLTexture>& t, std::optional<RGBA8> const& c, Func&& func, Data* store = {}) {
            Begin(t, c);
            func();
            End(store);
        }

        template<typename Func>
        Ref<GLTexture> Draw(XYu const& wh, bool hasAlpha, std::optional<RGBA8> const& c, Func&& func, Data* store = {}) {
            auto t = MakeTexture(wh, hasAlpha);
            DrawTo(t, c, std::forward<Func>(func), store);
            return t;
        }

    protected:
        void Begin(Ref<GLTexture>& t, std::optional<RGBA8> const& c = {}) {
            assert(t);
            auto& eb = EngineBase1::Instance();
            eb.ShaderEnd();
            bakWndSiz = eb.windowSize;
            bakBlend = eb.blend;
            bakTexSiz = { t->Width(), t->Height() };
            eb.SetWindowSize((float)t->Width(), (float)t->Height());
            eb.flipY = -1;
            BindGLFrameBufferTexture(fb, *t);
            eb.GLViewport();
            if (c.has_value()) {
                eb.GLClear(c.value());
            }
            eb.GLBlendFunc(eb.blendDefault);
        }

        void End(Data* store = {}) {
            auto& eb = EngineBase1::Instance();
            eb.ShaderEnd();
            if (store) {
                GLFrameBufferSaveTo(*store, 0, 0, bakTexSiz.x, bakTexSiz.y);
            }
            UnbindGLFrameBuffer();
            eb.SetWindowSize(bakWndSiz.x, bakWndSiz.y);
            eb.flipY = 1;
            eb.GLViewport();
            eb.GLBlendFunc(bakBlend);
        }
    };

}
