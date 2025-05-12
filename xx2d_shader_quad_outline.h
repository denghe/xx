#pragma once
#include <xx2d_shader.h>

namespace xx {

    struct QuadInstanceOutlineData {
        XY pos{}, anchor{ 0.5, 0.5 };       // float * 4

        XY scale{ 1, 1 };
        float radians{}, colorplus{ 1 };    // float * 4

        RGBA8 color{ 255, 255, 255, 255 };  // u8 * 4

        UVRect texRect{};                   // u16 * 4

        RGBA8 outlineColor{};               // u8 * 4

        XY outlineThickness{};              // float * 2
    };

    struct Shader_QuadInstanceOutline : Shader {
        using Shader::Shader;
        GLint uTex0{ -1 }, aVert{ -1 }, aPosAnchor{ -1 }, aScaleRadiansColorplus{ -1 }, aColor{ -1 }, aTexRect{ -1 }, aOutlineColor{ -1 }, aOutlineThickness{ -1 };
        GLVertexArrays va;
        GLBuffer vb, ib;

        static constexpr int32_t maxQuadNums{ 20000 };
        GLuint lastTextureId{};
        std::unique_ptr<QuadInstanceOutlineData[]> quadInstanceOutlineDatas = std::make_unique_for_overwrite<QuadInstanceOutlineData[]>(maxQuadNums);
        int32_t quadCount{};

        void Init() {

            v = LoadGLVertexShader({ R"(#version 300 es
uniform vec2 uCxy;	// screen center coordinate

in vec2 aVert;	// fans index { 0, 0 }, { 0, 1.f }, { 1.f, 0 }, { 1.f, 1.f }

in vec4 aPosAnchor;
in vec4 aScaleRadiansColorplus;
in vec4 aColor;
in vec4 aTexRect;
in vec4 aOutlineColor;
in vec2 aOutlineThickness;

out vec2 vTexCoord;
out float vColorplus;
out vec4 vColor;
out vec4 vOutlineColor;
out vec2 vOutlineThickness;

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
    vTexCoord = vec2(aTexRect.x + aVert.x * aTexRect.z, aTexRect.y + aTexRect.w - aVert.y * aTexRect.w);
    vOutlineColor = aOutlineColor;
    vOutlineThickness = aOutlineThickness;
})"sv });

            f = LoadGLFragmentShader({ R"(#version 300 es
precision highp float;          // mediump draw border has issue
uniform sampler2D uTex0;

in vec2 vTexCoord;
in float vColorplus;
in vec4 vColor;
in vec4 vOutlineColor;
in vec2 vOutlineThickness;

out vec4 oColor;

void main() {
    vec2 texSize = vec2(textureSize(uTex0, 0));

    vec2 p1 = vec2(vTexCoord.x - vOutlineThickness.x, vTexCoord.y + vOutlineThickness.y);
    vec2 p2 = vec2(vTexCoord.x + vOutlineThickness.x, vTexCoord.y + vOutlineThickness.y);
    vec2 p3 = vec2(vTexCoord.x + vOutlineThickness.x, vTexCoord.y - vOutlineThickness.y);
    vec2 p4 = vec2(vTexCoord.x - vOutlineThickness.x, vTexCoord.y - vOutlineThickness.y);
    float a1 = vColor.w * texture(uTex0, p1 / texSize).w;
    float a2 = vColor.w * texture(uTex0, p2 / texSize).w;
    float a3 = vColor.w * texture(uTex0, p3 / texSize).w;
    float a4 = vColor.w * texture(uTex0, p4 / texSize).w;

    // vec2 p5 = vec2(vTexCoord.x, vTexCoord.y + vOutlineThickness.y);
    // vec2 p6 = vec2(vTexCoord.x, vTexCoord.y - vOutlineThickness.y);
    // vec2 p7 = vec2(vTexCoord.x + vOutlineThickness.x, vTexCoord.y);
    // vec2 p8 = vec2(vTexCoord.x - vOutlineThickness.x, vTexCoord.y);
    // float a5 = vColor.w * texture(uTex0, p5 / texSize).w;
    // float a6 = vColor.w * texture(uTex0, p6 / texSize).w;
    // float a7 = vColor.w * texture(uTex0, p7 / texSize).w;
    // float a8 = vColor.w * texture(uTex0, p8 / texSize).w;

    vec4 c = vColor * texture(uTex0, vTexCoord / texSize);
    c = clamp(vec4( (c.x + 0.00001f) * vColorplus, (c.y + 0.00001f) * vColorplus, (c.z + 0.00001f) * vColorplus, c.w ), 0.f, 1.f);
    float a = clamp(a1 + a2 + a3 + a4
    // + a5 + a6 + a7 + a8
    , 0.f, 1.f) - c.w;
    if (a > 0.5f) oColor = vOutlineColor;
    else oColor = c;
})"sv });

            p = LinkGLProgram(v, f);

            uCxy = glGetUniformLocation(p, "uCxy");
            uTex0 = glGetUniformLocation(p, "uTex0");

            aVert = glGetAttribLocation(p, "aVert");
            aPosAnchor = glGetAttribLocation(p, "aPosAnchor");
            aScaleRadiansColorplus = glGetAttribLocation(p, "aScaleRadiansColorplus");
            aColor = glGetAttribLocation(p, "aColor");
            aTexRect = glGetAttribLocation(p, "aTexRect");
            aOutlineColor = glGetAttribLocation(p, "aOutlineColor");
            aOutlineThickness = glGetAttribLocation(p, "aOutlineThickness");
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

            glVertexAttribPointer(aPosAnchor, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceOutlineData), 0);  // offsetof(QuadInstanceOutlineData, pos
            glVertexAttribDivisor(aPosAnchor, 1);
            glEnableVertexAttribArray(aPosAnchor);

            glVertexAttribPointer(aScaleRadiansColorplus, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceOutlineData), (GLvoid*)offsetof(QuadInstanceOutlineData, scale));
            glVertexAttribDivisor(aScaleRadiansColorplus, 1);
            glEnableVertexAttribArray(aScaleRadiansColorplus);

            glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadInstanceOutlineData), (GLvoid*)offsetof(QuadInstanceOutlineData, color));
            glVertexAttribDivisor(aColor, 1);
            glEnableVertexAttribArray(aColor);

            glVertexAttribPointer(aTexRect, 4, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(QuadInstanceOutlineData), (GLvoid*)offsetof(QuadInstanceOutlineData, texRect));
            glVertexAttribDivisor(aTexRect, 1);
            glEnableVertexAttribArray(aTexRect);

            glVertexAttribPointer(aOutlineColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadInstanceOutlineData), (GLvoid*)offsetof(QuadInstanceOutlineData, outlineColor));
            glVertexAttribDivisor(aOutlineColor, 1);
            glEnableVertexAttribArray(aOutlineColor);

            glVertexAttribPointer(aOutlineThickness, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceOutlineData), (GLvoid*)offsetof(QuadInstanceOutlineData, outlineThickness));
            glVertexAttribDivisor(aOutlineThickness, 1);
            glEnableVertexAttribArray(aOutlineThickness);

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
            glBufferData(GL_ARRAY_BUFFER, sizeof(QuadInstanceOutlineData) * quadCount, quadInstanceOutlineDatas.get(), GL_STREAM_DRAW);

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

        QuadInstanceOutlineData* Draw(GLuint texId, int32_t numQuads) {
            assert(gEngine->shader == this);
            assert(numQuads <= maxQuadNums);
            if (quadCount + numQuads > maxQuadNums || (lastTextureId && lastTextureId != texId)) {
                Commit();
            }
            lastTextureId = texId;
            auto r = &quadInstanceOutlineDatas[quadCount];
            quadCount += numQuads;
            return r;
        }

        QuadInstanceOutlineData* Draw(Ref<Frame> const& f, float outlineThickness, RGBA8 outlineColor) {
            auto q = Draw(f->tex->GetValue(), 1);
            q->pos = {};
            q->anchor = 0.5f;
            q->scale = 1;
            q->radians = 0;
            q->colorplus = 1.f;
            q->color = xx::RGBA8_White;
            q->texRect.x = uint16_t(f->textureRect.x - outlineThickness - 1.f);
            q->texRect.y = uint16_t(f->textureRect.y - outlineThickness - 1.f);
            q->texRect.w = uint16_t(f->textureRect.w + (outlineThickness + 1.f) * 2.f);
            q->texRect.h = uint16_t(f->textureRect.h + (outlineThickness + 1.f) * 2.f);
            q->outlineColor = outlineColor;
            q->outlineThickness = outlineThickness;
            return q;
        }

    };

}
