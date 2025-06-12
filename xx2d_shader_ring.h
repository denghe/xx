#pragma once
#include "xx2d_shader.h"

namespace xx {

    // for light layer

    struct RingInstanceData {
        XY pos{};
        float radius{}, colorPlus{};
        RGBA8 color{ 255, 255, 255, 255 };
    };

    struct Shader_RingInstance : Shader {
        using Shader::Shader;
        GLint uSwh2FlipY{ -1 }, aVert{ -1 }, aPosRadiusColorPlus{ -1 }, aColor{ -1 };
        GLVertexArrays va;
        GLBuffer vb, ib;

        static constexpr int32_t maxRingNums{ 20000 };
        std::unique_ptr<RingInstanceData[]> ringInstanceDatas = std::make_unique<RingInstanceData[]>(maxRingNums);
        int32_t ringCount{};

        void Init() {

            v = LoadGLVertexShader({ R"(#version 300 es
uniform vec4 uSwh2FlipY;	// screen width & height / 2, flipY

in vec2 aVert;	// fans index { -1, -1 }, { -1, 1.f }, { 1.f, -1 }, { 1.f, 1.f }

in vec4 aPosRadiusColorPlus;
in vec4 aColor;

flat out vec4 vPosRadiusColorPlus;
flat out vec4 vColor;

void main() {
    vec2 p = aPosRadiusColorPlus.xy;
    float r = aPosRadiusColorPlus.z;
    vec2 v = p + aVert * r;

    gl_Position = vec4(v / (uSwh2FlipY.xy * uSwh2FlipY.zw), 0, 1);  // -1 ~ 1

    vPosRadiusColorPlus.xy = p * uSwh2FlipY.zw + uSwh2FlipY.xy;              // frag coord
    vPosRadiusColorPlus.z = r;
    vPosRadiusColorPlus.w = aPosRadiusColorPlus.w;
    vColor = aColor;
})"sv });

            f = LoadGLFragmentShader({ R"(#version 300 es
precision mediump float;

flat in vec4 vPosRadiusColorPlus;
flat in vec4 vColor;

out vec4 oColor;

void main() {
    vec2 p = vPosRadiusColorPlus.xy;
    float r = vPosRadiusColorPlus.z;
    float d = distance(p, gl_FragCoord.xy);
    if (d > r) discard;

    //float a = smoothstep(r - 100.f, r, d) * smoothstep(r, r - 1.f, d) * (step(r - 7.f, d) * 0.2f + 0.3f);
    //oColor.xyz = vColor.xyz;
    //oColor.a = a;

    float a = 1.f - d / r;
    oColor.xyz = vColor.xyz * a * vPosRadiusColorPlus.w;
    oColor.a = 1.f;
})"sv });

            p = LinkGLProgram(v, f);

            uSwh2FlipY = glGetUniformLocation(p, "uSwh2FlipY");

            aVert = glGetAttribLocation(p, "aVert");
            aPosRadiusColorPlus = glGetAttribLocation(p, "aPosRadiusColorPlus");
            aColor = glGetAttribLocation(p, "aColor");
            CheckGLError();

            glGenVertexArrays(1, va.GetValuePointer());
            glGenBuffers(1, (GLuint*)&ib);
            glGenBuffers(1, (GLuint*)&vb);

            glBindVertexArray(va);

            static const XY verts[4] = { { -1, -1 }, { -1, 1.f }, { 1.f, -1 }, { 1.f, 1.f } };  // maybe octagon better performance ?
            glBindBuffer(GL_ARRAY_BUFFER, ib);
            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
            glVertexAttribPointer(aVert, 2, GL_FLOAT, GL_FALSE, sizeof(XY), 0);
            glEnableVertexAttribArray(aVert);

            glBindBuffer(GL_ARRAY_BUFFER, vb);

            glVertexAttribPointer(aPosRadiusColorPlus, 4, GL_FLOAT, GL_FALSE, sizeof(RingInstanceData), 0);
            glVertexAttribDivisor(aPosRadiusColorPlus, 1);
            glEnableVertexAttribArray(aPosRadiusColorPlus);

            glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RingInstanceData), (GLvoid*)offsetof(RingInstanceData, color));
            glVertexAttribDivisor(aColor, 1);
            glEnableVertexAttribArray(aColor);

            glBindVertexArray(0);
            CheckGLError();
        }

        virtual void Begin() override {
            assert(!gEngine->shader);
            glUseProgram(p);
            glUniform4f(uSwh2FlipY, gEngine->windowSize.x * 0.5f, gEngine->windowSize.y * 0.5f, 1.f, gEngine->flipY);
            glBindVertexArray(va);
        }

        virtual void End() override {
            assert(gEngine->shader == this);
            if (ringCount) {
                Commit();
            }
        }

        void Commit() {
            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glBufferData(GL_ARRAY_BUFFER, sizeof(RingInstanceData) * ringCount, ringInstanceDatas.get(), GL_STREAM_DRAW);

            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ringCount);
            CheckGLError();

            drawVerts += ringCount * 6;
            drawCall += 1;

            ringCount = 0;
        }

        RingInstanceData* Draw(int32_t numRings) {
            assert(gEngine->shader == this);
            assert(numRings <= maxRingNums);
            if (ringCount + numRings > maxRingNums) {
                Commit();
            }
            auto r = &ringInstanceDatas[ringCount];
            ringCount += numRings;
            return r;
        }

    };


}
