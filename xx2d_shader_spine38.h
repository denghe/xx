#pragma once
#include <xx2d_shader_vertexs.h>

namespace xx {

    struct Shader_Spine38 : Shader {
        using Shader::Shader;
        GLint uTex0{ -1 }, aPos{ -1 }, aColor{ -1 }, aTexCoord{ -1 };
        GLVertexArrays va;
        GLBuffer vb;

        static constexpr int32_t maxIndexNums{ maxVertNums * 4 };
        GLuint lastTextureId{};
        int32_t vertsCount{};
        std::unique_ptr<VertexData[]> verts = std::make_unique<VertexData[]>(maxVertNums);

        void Init() {
            v = LoadGLVertexShader({ R"(#version 300 es
precision highp float;
uniform vec2 uCxy;	// screen center coordinate

in vec2 aPos;
in vec2 aTexCoord;
in vec4 aColor;

out vec4 vColor;
out vec2 vTexCoord;

void main() {
	gl_Position = vec4(aPos * uCxy, 0, 1);
	vTexCoord = aTexCoord;
	vColor = aColor;
})"sv });

            f = LoadGLFragmentShader({ R"(#version 300 es
precision highp float;
uniform sampler2D uTex0;

in vec4 vColor;
in vec2 vTexCoord;

out vec4 oColor;

void main() {
	oColor = vColor * texture(uTex0, vTexCoord / vec2(textureSize(uTex0, 0)));
})"sv });

            p = LinkGLProgram(v, f);

            uCxy = glGetUniformLocation(p, "uCxy");
            uTex0 = glGetUniformLocation(p, "uTex0");

            aPos = glGetAttribLocation(p, "aPos");
            aTexCoord = glGetAttribLocation(p, "aTexCoord");
            aColor = glGetAttribLocation(p, "aColor");
            CheckGLError();

            glGenVertexArrays(1, va.GetValuePointer());
            glBindVertexArray(va);
            glGenBuffers(1, (GLuint*)&vb);

            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), 0);
            glEnableVertexAttribArray(aPos);
            glVertexAttribPointer(aTexCoord, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, uv));
            glEnableVertexAttribArray(aTexCoord);
            glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, color));
            glEnableVertexAttribArray(aColor);

            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
            if (vertsCount) {
                Commit();
            }
        }

        void Commit() {
            glBindBuffer(GL_ARRAY_BUFFER, vb);
            glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * vertsCount, verts.get(), GL_STREAM_DRAW);

            glBindTexture(GL_TEXTURE_2D, lastTextureId);
            glDrawArrays(GL_TRIANGLES, 0, vertsCount);
            CheckGLError();

            drawVerts += vertsCount;
            drawCall += 1;

            lastTextureId = 0;
            vertsCount = 0;
        }

        VertexData* Draw(GLuint texId, int32_t numVerts) {
            assert(numVerts <= maxVertNums);
            if (vertsCount + numVerts > maxVertNums || (lastTextureId && lastTextureId != texId)) {
                Commit();
            }
            lastTextureId = texId;
            auto vc = vertsCount;
            vertsCount += numVerts;
            return &verts[vc];
        }

        VertexData* Draw(Ref<GLTexture> const& tex, int32_t numVerts) {
            return Draw(tex->GetValue(), numVerts);
        }
    };

}
