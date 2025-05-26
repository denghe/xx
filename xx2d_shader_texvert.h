#pragma once
#include <xx2d_shader_spine38.h>

namespace xx {

    // tex data: float * 8
    // pos(2) + uv(2) + color(4)
    struct TexData {
        float x, y, u, v, r, g, b, a;
    };

    struct TexVertData {
        XY pos{}, scale{};
        int32_t frameIndex{};
    };

    struct Shader_TexVert : Shader {
        using Shader::Shader;
        GLint uTex0{ -1 }, uTex1{ -1 }, aVertIndex{ -1 }, aPosScale{ -1 }, aFrameIndex{ -1 };
        GLVertexArrays va;
        GLBuffer vb, ib;

        static constexpr int32_t vertCap{ 8192 };
        static constexpr int32_t dataCap{ 100000 };
        GLuint lastTextureId{}, lastVertTextureId{}, numVerts{};
        std::unique_ptr<TexVertData[]> datas = std::make_unique_for_overwrite<TexVertData[]>(dataCap);
        int32_t dataCount{};

        void Init() {

            v = LoadGLVertexShader({ R"(#version 300 es
precision mediump float;
uniform vec2 uCxy;	                                            // screen center coordinate
uniform sampler2D uTex1;                                        // TexData[][]

in int aVertIndex;                                              // vert index: 0, 1, 2, ...... 8191

in vec4 aPosScale;
in int aFrameIndex;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vec2 pos = aPosScale.xy;
    vec2 scale = aPosScale.zw;
    int offset = aVertIndex * 2;
    vec4 xyuv = texelFetch(uTex1, ivec2(offset + 0, aFrameIndex), 0);
    vColor    = texelFetch(uTex1, ivec2(offset + 1, aFrameIndex), 0);
    vTexCoord = xyuv.zw;
    gl_Position = vec4((pos + xyuv.xy * scale) * uCxy, 0, 1);
})"sv });

            f = LoadGLFragmentShader({ R"(#version 300 es
precision highp float;
uniform sampler2D uTex0;

in vec2 vTexCoord;
in vec4 vColor;

out vec4 oColor;

void main() {
    oColor = vColor * texture(uTex0, vTexCoord / vec2(textureSize(uTex0, 0)));
})"sv });

            p = LinkGLProgram(v, f);

            uCxy = glGetUniformLocation(p, "uCxy");
            uTex0 = glGetUniformLocation(p, "uTex0");
            uTex1 = glGetUniformLocation(p, "uTex1");

            aVertIndex = glGetAttribLocation(p, "aVertIndex");
            aPosScale = glGetAttribLocation(p, "aPosScale");
            aFrameIndex = glGetAttribLocation(p, "aFrameIndex");
            CheckGLError();

            glGenVertexArrays(1, va.GetValuePointer());
            glGenBuffers(1, ib.GetValuePointer());
            glGenBuffers(1, vb.GetValuePointer());

            glBindVertexArray(va);

            auto d = std::make_unique<int[]>(vertCap);
            for (int i = 0; i < vertCap; ++i) d[i] = i;
            glBindBuffer(GL_ARRAY_BUFFER, ib);
            glBufferData(GL_ARRAY_BUFFER, vertCap * 4, d.get(), GL_STATIC_DRAW);
            glVertexAttribIPointer(aVertIndex, 1, GL_INT, sizeof(int), 0);
            glEnableVertexAttribArray(aVertIndex);

            glBindBuffer(GL_ARRAY_BUFFER, vb);

            glVertexAttribPointer(aPosScale, 4, GL_FLOAT, GL_FALSE, sizeof(TexVertData), (GLvoid*)offsetof(TexVertData, pos));
            glVertexAttribDivisor(aPosScale, 1);
            glEnableVertexAttribArray(aPosScale);

            glVertexAttribIPointer(aFrameIndex, 1, GL_INT, sizeof(TexVertData), (GLvoid*)offsetof(TexVertData, frameIndex));
            glVertexAttribDivisor(aFrameIndex, 1);
            glEnableVertexAttribArray(aFrameIndex);

            glBindVertexArray(0);
            CheckGLError();
        }

        virtual void Begin() override {
            assert(!gEngine->shader);
            glUseProgram(p);
            glActiveTexture(GL_TEXTURE0 + 0);
            glActiveTexture(GL_TEXTURE0 + 1);
            glUniform1i(uTex0, 0);
            glUniform1i(uTex1, 1);
            glUniform2f(uCxy, 2 / gEngine->windowSize.x, -2 / gEngine->windowSize.y * gEngine->flipY);
            glBindVertexArray(va);
        }

        virtual void End() override {
            assert(gEngine->shader == this);
            if (dataCount) {
                Commit();
            }
        }

        void Commit() {
            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TexVertData) * dataCount, datas.get(), GL_STREAM_DRAW);

            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, lastTextureId);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, lastVertTextureId);

            glDrawArraysInstanced(GL_TRIANGLES, 0, numVerts, dataCount);
            CheckGLError();

            drawVerts += dataCount * numVerts;
            drawCall += 1;

            lastTextureId = 0;
            lastVertTextureId = 0;
            dataCount = 0;
        }

        TexVertData* Draw(Ref<GLTexture> tex, Ref<GLVertTexture> const& vertTex, int32_t count) {
            assert(gEngine->shader == this);
            assert(count <= dataCap);
            auto texId = tex->GetValue();
            auto vertTexId = vertTex->GetValue();
            if (dataCount + count > dataCap
                || (lastTextureId && lastTextureId != texId) 
                || (lastVertTextureId && lastVertTextureId != vertTexId)) {
                Commit();
            }
            lastTextureId = texId;
            lastVertTextureId = vertTexId;
            numVerts = vertTex->NumVerts();
            assert(vertCap >= numVerts);
            auto r = &datas[dataCount];
            dataCount += count;
            return r;
        }

    };

}
