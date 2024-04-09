#pragma once
#include <xx2d_includes.h>
#include <xx2d_opengl.h>
#include <xx2d_prims.h>

namespace xx {

    // sprite frame
    struct Frame {
        Ref<GLTexture> tex;
        std::string key;
        // std::vector<std::string> aliases;	// unused
        std::optional<XY> anchor;
        XY spriteOffset{};
        XY spriteSize{};		// content size
        XY spriteSourceSize{};	// original pic size
        UVRect textureRect{};
        size_t ud{};   // user data
        bool textureRotated{};
        std::vector<uint16_t> triangles;
        std::vector<float> vertices;
        std::vector<float> verticesUV;

        // single texture -> frame
        inline Ref<Frame> static Create(Ref<GLTexture> t) {
            auto f = MakeRef<Frame>();
            f->key = t->FileName();
            f->anchor = { 0.5, 0.5 };
            f->textureRotated = false;
            f->spriteSize = f->spriteSourceSize = { (float)t->Width(), (float)t->Height() };
            f->spriteOffset = {};
            f->textureRect = { 0, 0, (uint16_t)f->spriteSize.x, (uint16_t)f->spriteSize.y };
            f->tex = std::move(t);
            return f;
        }
    };

    // frame's tiny version
    struct TinyFrame {
        Ref<GLTexture> tex;
        UVRect texRect{};
    };


    struct AnimFrame {
        Ref<Frame> frame;
        float durationSeconds;
    };

    using AnimFrames = std::vector<AnimFrame>;

    struct Anim {
        std::vector<AnimFrame> animFrames;
        size_t cursor{};
        float timePool{};

        void Step() {
            if (++cursor == animFrames.size()) {
                cursor = 0;
            }
        }

        bool Update(float delta) {
            auto bak = cursor;
            timePool += delta;
        LabBegin:
            auto&& af = animFrames[cursor];
            if (timePool >= af.durationSeconds) {
                timePool -= af.durationSeconds;
                Step();
                goto LabBegin;
            }
            return bak != cursor;
        }

        AnimFrame& GetCurrentAnimFrame() const {
            return (AnimFrame&)animFrames[cursor];
        }
    };

}
