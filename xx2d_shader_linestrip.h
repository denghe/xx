﻿#pragma once
#include <xx2d_shader.h>

namespace xx {

    // 1 point data ( for draw line strip )
    struct XYRGBA8 : XY, RGBA8 {};

    struct Shader_LineStrip : Shader {
        using Shader::Shader;
        GLint aPos{ -1 }, aColor{ -1 };
        GLVertexArrays va;
        GLBuffer vb, ib;

        static constexpr int32_t maxIndexNums = int32_t(maxVertNums * 1.5);
        int32_t pointsCount{}, indexsCount{};
        std::unique_ptr<XYRGBA8[]> points = std::make_unique_for_overwrite<XYRGBA8[]>(maxVertNums);
        std::unique_ptr<uint16_t[]> indexs = std::make_unique_for_overwrite<uint16_t[]>(maxIndexNums);

        void Init() {
            v = LoadGLVertexShader({ R"(#version 300 es
precision highp float;
uniform vec2 uCxy;	// screen center coordinate

in vec2 aPos;
in vec4 aColor;

out vec4 vColor;

void main() {
	gl_Position = vec4(aPos * uCxy, 0, 1);
	vColor = aColor;
})"sv });

            f = LoadGLFragmentShader({ R"(#version 300 es
precision mediump float;

in vec4 vColor;

out vec4 oColor;

void main() {
	oColor = vColor;
})"sv });

            p = LinkGLProgram(v, f);

            uCxy = glGetUniformLocation(p, "uCxy");

            aPos = glGetAttribLocation(p, "aPos");
            aColor = glGetAttribLocation(p, "aColor");
            CheckGLError();

            glGenVertexArrays(1, va.GetValuePointer());
            glBindVertexArray(va);
            glGenBuffers(1, (GLuint*)&vb);
            glGenBuffers(1, (GLuint*)&ib);

            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, sizeof(XYRGBA8), 0);
            glEnableVertexAttribArray(aPos);
            glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(XYRGBA8), (GLvoid*)offsetof(XYRGBA8, r));
            glEnableVertexAttribArray(aColor);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);

            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            CheckGLError();
        }

        virtual void Begin() override {
            assert(!gEngine->shader);
            glUseProgram(p);
            glUniform2f(uCxy, 2 / gEngine->windowSize.x, 2 / gEngine->windowSize.y * gEngine->flipY);
            glBindVertexArray(va);
        }

        virtual void End() override {
            assert(gEngine->shader == this);
            if (indexsCount) {
                Commit();
            }
        }

        void Commit() {
            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glBufferData(GL_ARRAY_BUFFER, sizeof(XYRGBA8) * pointsCount, points.get(), GL_STREAM_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * indexsCount, indexs.get(), GL_STREAM_DRAW);

            glDrawElements(GL_LINE_STRIP, indexsCount, GL_UNSIGNED_SHORT, 0);

            drawVerts += indexsCount;
            drawCall += 1;

            pointsCount = 0;
            indexsCount = 0;
        }

        XYRGBA8* Draw(int32_t pc) {
            assert(gEngine->shader == this);
            assert(pc <= maxVertNums);
            auto&& c = pointsCount + pc;
            if (c > maxVertNums) {
                Commit();
                c = pc;
            }
            auto rtv = &points[pointsCount];
            for (auto i = pointsCount; i < c; ++i) {
                indexs[indexsCount++] = i;
            }
            indexs[indexsCount++] = 65535;	// primitive restart
            assert(indexsCount <= maxIndexNums);
            pointsCount = c;
            return rtv;
        }

        void Draw(XYRGBA8* pointsBuf, int32_t pc) {
            memcpy(Draw(pc), pointsBuf, sizeof(XYRGBA8) * pc);
        }

    };

}
