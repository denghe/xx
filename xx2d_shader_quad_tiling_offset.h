#pragma once
#include <xx2d_shader.h>

namespace xx {

    struct QuadInstanceTilingOffsetData {
        XY pos{}, anchor{ 0.5, 0.5 };       // float * 4

        XY scale{ 1, 1 };
        float radians{}, colorplus{ 1 };    // float * 4

        RGBA8 color{ 255, 255, 255, 255 };  // u8n * 4

        UVRect texRect{};                   // u16 * 4

        XY tiling{}, offset{};              // float * 4
    };

    struct Shader_QuadInstanceTilingOffset : Shader {
        using Shader::Shader;
        GLint uTex0{ -1 }, aVert{ -1 }, aPosAnchor{ -1 }, aScaleRadiansColorplus{ -1 }, aColor{ -1 }, aTexRect{ -1 }, aTilingOffset{ -1 };
        GLVertexArrays va;
        GLBuffer vb, ib;

        static constexpr int32_t maxQuadNums{ 1000 };
        GLuint lastTextureId{};
        std::unique_ptr<QuadInstanceTilingOffsetData[]> quadInstanceTilingOffsetDatas = std::make_unique_for_overwrite<QuadInstanceTilingOffsetData[]>(maxQuadNums);
        int32_t quadCount{};

        void Init() {

            v = LoadGLVertexShader({ R"(#version 300 es
uniform vec2 uCxy;	// screen center coordinate

in vec2 aVert;	// fans index { 0, 0 }, { 0, 1.f }, { 1.f, 0 }, { 1.f, 1.f }

in vec4 aPosAnchor;
in vec4 aScaleRadiansColorplus;
in vec4 aColor;
in vec4 aTexRect;
in vec4 aTilingOffset;

out vec2 vTexCoord;
out float vColorplus;
out vec4 vColor;
out vec4 vTexRect;

void main() {
    vec2 pos = aPosAnchor.xy;
    vec2 anchor = aPosAnchor.zw;
    vec2 scale = vec2(aScaleRadiansColorplus.x * aTexRect.z, aScaleRadiansColorplus.y * aTexRect.w);
    float radians = aScaleRadiansColorplus.z;
    vec2 offset = vec2((aVert.x - anchor.x) * scale.x, (aVert.y - anchor.y) * scale.y);

    float c = cos(radians);
    float s = sin(radians);
    vec2 v = pos + vec2(
       dot(offset, vec2(c, s)),
       dot(offset, vec2(-s, c))
    );

    gl_Position = vec4(v * uCxy, 0, 1);
    vColor = aColor;
    vColorplus = aScaleRadiansColorplus.w;
    vTexCoord = (vec2(aVert.x * aTexRect.z, aTexRect.w - aVert.y * aTexRect.w) + aTilingOffset.zw) * aTilingOffset.xy;
    vTexRect = aTexRect;
})"sv });

            f = LoadGLFragmentShader({ R"(#version 300 es
precision highp float;          // mediump draw border has issue
uniform sampler2D uTex0;

in vec2 vTexCoord;
in float vColorplus;
in vec4 vColor;
in vec4 vTexRect;

out vec4 oColor;

void main() {
    vec2 uv = vTexRect.xy + mod(vTexCoord, vTexRect.zw);
    vec4 c = vColor * texture(uTex0, uv / vec2(textureSize(uTex0, 0)));
    oColor = vec4( (c.x + 0.00001f) * vColorplus, (c.y + 0.00001f) * vColorplus, (c.z + 0.00001f) * vColorplus, c.w );
})"sv });

            p = LinkGLProgram(v, f);

            uCxy = glGetUniformLocation(p, "uCxy");
            uTex0 = glGetUniformLocation(p, "uTex0");

            aVert = glGetAttribLocation(p, "aVert");
            aPosAnchor = glGetAttribLocation(p, "aPosAnchor");
            aScaleRadiansColorplus = glGetAttribLocation(p, "aScaleRadiansColorplus");
            aColor = glGetAttribLocation(p, "aColor");
            aTexRect = glGetAttribLocation(p, "aTexRect");
            aTilingOffset = glGetAttribLocation(p, "aTilingOffset");
            CheckGLError();

            glGenVertexArrays(1, va.GetValuePointer());
            glBindVertexArray(va);

            glGenBuffers(1, (GLuint*)&ib);
            static const XY verts[4] = { { 0, 0 }, { 0, 1.f }, { 1.f, 0 }, { 1.f, 1.f } };
            glBindBuffer(GL_ARRAY_BUFFER, ib);
            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
            glVertexAttribPointer(aVert, 2, GL_FLOAT, GL_FALSE, sizeof(XY), 0);
            glEnableVertexAttribArray(aVert);

            glGenBuffers(1, (GLuint*)&vb);
            glBindBuffer(GL_ARRAY_BUFFER, vb);

            glVertexAttribPointer(aPosAnchor, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceTilingOffsetData), 0);  // offsetof(QuadInstanceTilingOffsetData, pos
            glVertexAttribDivisor(aPosAnchor, 1);
            glEnableVertexAttribArray(aPosAnchor);

            glVertexAttribPointer(aScaleRadiansColorplus, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceTilingOffsetData), (GLvoid*)offsetof(QuadInstanceTilingOffsetData, scale));
            glVertexAttribDivisor(aScaleRadiansColorplus, 1);
            glEnableVertexAttribArray(aScaleRadiansColorplus);

            glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadInstanceTilingOffsetData), (GLvoid*)offsetof(QuadInstanceTilingOffsetData, color));
            glVertexAttribDivisor(aColor, 1);
            glEnableVertexAttribArray(aColor);

            glVertexAttribPointer(aTexRect, 4, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(QuadInstanceTilingOffsetData), (GLvoid*)offsetof(QuadInstanceTilingOffsetData, texRect));
            glVertexAttribDivisor(aTexRect, 1);
            glEnableVertexAttribArray(aTexRect);

            glVertexAttribPointer(aTilingOffset, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceTilingOffsetData), (GLvoid*)offsetof(QuadInstanceTilingOffsetData, tiling));
            glVertexAttribDivisor(aTilingOffset, 1);
            glEnableVertexAttribArray(aTilingOffset);

            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            CheckGLError();
        }

        virtual void Begin() override {
            assert(!gEngine->shader);
            glUseProgram(p);
            glActiveTexture(GL_TEXTURE0/* + textureUnit*/);
            glUniform1i(uTex0, 0);
            glUniform2f(uCxy, 2 / gEngine->windowSize.x, 2 / gEngine->windowSize.y * gEngine->flipY);
            glBindVertexArray(va);
        }

        virtual void End() override {
            assert(gEngine->shader == this);
            if (quadCount) {
                Commit();
            }
        }

        void Commit() {
            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glBufferData(GL_ARRAY_BUFFER, sizeof(QuadInstanceTilingOffsetData) * quadCount, quadInstanceTilingOffsetDatas.get(), GL_STREAM_DRAW);

            glBindTexture(GL_TEXTURE_2D, lastTextureId);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, quadCount);

            //auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            //printf("fboStatus = %d lastTextureId = %d\n", fboStatus, lastTextureId);
            CheckGLError();

            drawVerts += quadCount * 6;
            drawCall += 1;

            lastTextureId = 0;
            quadCount = 0;
        }

        QuadInstanceTilingOffsetData* Draw(GLuint texId, int32_t numQuads) {
            assert(gEngine->shader == this);
            assert(numQuads <= maxQuadNums);
            if (quadCount + numQuads > maxQuadNums || (lastTextureId && lastTextureId != texId)) {
                Commit();
            }
            lastTextureId = texId;
            auto r = &quadInstanceTilingOffsetDatas[quadCount];
            quadCount += numQuads;
            return r;
        }
        QuadInstanceTilingOffsetData* Draw(Ref<GLTexture> const& tex, int32_t numQuads) {
            return Draw(tex->GetValue(), numQuads);
        }

        void Draw(GLuint texId, QuadInstanceTilingOffsetData const& qv) {
            memcpy(Draw(texId, 1), &qv, sizeof(QuadInstanceTilingOffsetData));
        }
        void Draw(Ref<GLTexture> const& tex, QuadInstanceTilingOffsetData const& qv) {
			Draw(tex->GetValue(), qv);
        }

    };

}
