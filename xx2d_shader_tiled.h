#pragma once
#include "xx2d_shader.h"

namespace xx {

    struct QuadInstanceTiledData {
        XY pos{}, scale{};                  // float * 4
        XY tileSize{}, mapSize{};           // float * 4
        XY tiling{}, offset{};              // float * 4
        XYi miniMapSize{};                  // i32 * 2
    };

    struct Shader_QuadInstanceTiled : xx::Shader {
        using xx::Shader::Shader;
        GLint uTex0{ -1 }, uTex1{ -1 }, aVert{ -1 }, aPosScale{ -1 }, aTileSizeMapSize{ -1 }, aTilingOffset{ -1 }, aMiniMapSize{ -1 };
        xx::GLVertexArrays va;
        xx::GLBuffer vb, ib;

        static constexpr int32_t maxQuadNums{ 100 };
        GLuint lastTextureId{}, lastMiniTextureId{};
        std::unique_ptr<QuadInstanceTiledData[]> quadInstanceTiledDatas = std::make_unique_for_overwrite<QuadInstanceTiledData[]>(maxQuadNums);
        int32_t quadCount{};

        void Init() {

            v = xx::LoadGLVertexShader({ R"(#version 300 es
uniform vec2 uCxy;	            // screen center coordinate

in vec2 aVert;	// fans index { 0, 0 }, { 0, 1.f }, { 1.f, 0 }, { 1.f, 1.f }

in vec4 aPosScale;
in vec4 aTileSizeMapSize;
in vec4 aTilingOffset;
in vec2 aMiniMapSize;

out vec2 vTexCoord;
flat out vec4 vTileSizeOffset;
flat out ivec2 vMiniMapSize;

void main() {
    gl_Position = vec4((aPosScale.xy + aTileSizeMapSize.zw * aVert * aPosScale.zw) * uCxy, 0.f, 1.f);
    vTexCoord = vec2(aTileSizeMapSize.z * aVert.x, aTileSizeMapSize.w - aTileSizeMapSize.w * aVert.y) * aTilingOffset.xy;
    vTileSizeOffset = vec4( aTileSizeMapSize.xy, aTilingOffset.zw );
    vMiniMapSize = ivec2(aMiniMapSize);
})"sv });

            f = xx::LoadGLFragmentShader({ R"(#version 300 es
precision highp float;          // mediump draw border has issue
uniform sampler2D uTex0;        // tiles
uniform sampler2D uTex1;        // minimap indexs[]     .xy: uv  .z: colorPlus

in vec2 vTexCoord;
flat in vec4 vTileSizeOffset;
flat in ivec2 vMiniMapSize;

out vec4 oColor;

void main() {
    vec2 tileTexSize = vec2(textureSize(uTex0, 0));
    vec2 tileSize = vTileSizeOffset.xy;
    vec2 offset = vTileSizeOffset.zw;
    vec2 p = vTexCoord + offset;
    ivec2 n = ivec2(p / tileSize);
    ivec2 miniIdx = n - (n / vMiniMapSize) * vMiniMapSize;
    vec4 uvc = texelFetch(uTex1, miniIdx, 0);
    vec2 uv = uvc.xy * tileSize;
    vec2 uvOffset = p - tileSize * vec2(miniIdx);
    vec4 c = texture(uTex0, (uv + uvOffset) / tileTexSize);
    oColor = vec4( (c.x + 0.00001f) * uvc.z, (c.y + 0.00001f) * uvc.z, (c.z + 0.00001f) * uvc.z, c.w );
})"sv });

            p = LinkGLProgram(v, f);

            uCxy = glGetUniformLocation(p, "uCxy");
            uTex0 = glGetUniformLocation(p, "uTex0");
            uTex1 = glGetUniformLocation(p, "uTex1");

            aVert = glGetAttribLocation(p, "aVert");
            aPosScale = glGetAttribLocation(p, "aPosScale");
            aTileSizeMapSize = glGetAttribLocation(p, "aTileSizeMapSize");
            aTilingOffset = glGetAttribLocation(p, "aTilingOffset");
            aMiniMapSize = glGetAttribLocation(p, "aMiniMapSize");
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

            glVertexAttribPointer(aPosScale, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceTiledData), 0);
            glVertexAttribDivisor(aPosScale, 1);
            glEnableVertexAttribArray(aPosScale);

            glVertexAttribPointer(aTileSizeMapSize, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceTiledData), (GLvoid*)offsetof(QuadInstanceTiledData, tileSize));
            glVertexAttribDivisor(aTileSizeMapSize, 1);
            glEnableVertexAttribArray(aTileSizeMapSize);

            glVertexAttribPointer(aTilingOffset, 4, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceTiledData), (GLvoid*)offsetof(QuadInstanceTiledData, tiling));
            glVertexAttribDivisor(aTilingOffset, 1);
            glEnableVertexAttribArray(aTilingOffset);

            glVertexAttribPointer(aMiniMapSize, 2, GL_FLOAT, GL_FALSE, sizeof(QuadInstanceTiledData), (GLvoid*)offsetof(QuadInstanceTiledData, miniMapSize));
            glVertexAttribDivisor(aMiniMapSize, 1);
            glEnableVertexAttribArray(aMiniMapSize);

            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            CheckGLError();
        }

        void Begin() {
            assert(!gEngine->shader);
            glUseProgram(p);
            glActiveTexture(GL_TEXTURE0 + 0);
            glActiveTexture(GL_TEXTURE0 + 1);
            glUniform1i(uTex0, 0);
            glUniform1i(uTex1, 1);
            glUniform2f(uCxy, 2 / gEngine->windowSize.x, 2 / gEngine->windowSize.y * gEngine->flipY);
            glBindVertexArray(va);
        }

        void End() {
            assert(gEngine->shader == this);
            if (quadCount) {
                Commit();
            }
        }

        void Commit() {
            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glBufferData(GL_ARRAY_BUFFER, sizeof(QuadInstanceTiledData) * quadCount, quadInstanceTiledDatas.get(), GL_STREAM_DRAW);

            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, lastTextureId);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, lastMiniTextureId);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, quadCount);

            CheckGLError();

            drawVerts += quadCount * 6;
            drawCall += 1;

            quadCount = 0;
        }

        QuadInstanceTiledData* Draw(GLuint texId_Tiled, GLuint texId_Mini) {
            assert(gEngine->shader == this);
            if (quadCount + 1 > maxQuadNums
                || (lastTextureId && lastTextureId != texId_Tiled)
                || (lastMiniTextureId && lastMiniTextureId != texId_Mini)) {
                Commit();
            }
            lastTextureId = texId_Tiled;
            lastMiniTextureId = texId_Mini;
            auto r = &quadInstanceTiledDatas[quadCount];
            quadCount += 1;
            return r;
        }
        QuadInstanceTiledData* Draw(xx::Ref<xx::GLTexture> const& texTiled, xx::Ref<xx::GLTiledTexture> const& texMini) {
            auto r = Draw(texTiled->GetValue(), texMini->GetValue());
            r->miniMapSize = { texMini->SizeX(), texMini->SizeY() };
            return r;
        }
    };

}
