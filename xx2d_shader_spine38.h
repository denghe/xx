#pragma once
#include <xx2d_shader_vertexs.h>

namespace xx {

    struct SpineVertexData {
        XY pos, uv;
        RGBA8 color;
    };

    struct Shader_Spine38 : Shader {
        using Shader::Shader;
        GLint uTex0{ -1 }, aPosUv{ -1 }, aColor{ -1 };
        GLVertexArrays va;
        GLBuffer vb;

        static constexpr int32_t vertsCap{ maxVertNums * 4 };
        GLuint lastTextureId{};
        int32_t vertsCount{};
        std::unique_ptr<SpineVertexData[]> verts = std::make_unique<SpineVertexData[]>(vertsCap);

        void Init() {
            v = LoadGLVertexShader({ R"(#version 300 es
precision highp float;
uniform vec2 uCxy;	// screen center coordinate

in vec4 aPosUv;
in vec4 aColor;

out vec4 vColor;
out vec2 vTexCoord;

void main() {
	gl_Position = vec4(aPosUv.xy * uCxy, 0, 1);
	vTexCoord = aPosUv.zw;
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

            aPosUv = glGetAttribLocation(p, "aPosUv");
            aColor = glGetAttribLocation(p, "aColor");
            CheckGLError();

            glGenVertexArrays(1, va.GetValuePointer());
            glGenBuffers(1, vb.GetValuePointer());

            glBindVertexArray(va);

            glBindBuffer(GL_ARRAY_BUFFER, vb);

            glVertexAttribPointer(aPosUv, 4, GL_FLOAT, GL_FALSE, sizeof(SpineVertexData), (GLvoid*)offsetof(SpineVertexData, pos));
            glEnableVertexAttribArray(aPosUv);

            glVertexAttribPointer(aColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SpineVertexData), (GLvoid*)offsetof(SpineVertexData, color));
            glEnableVertexAttribArray(aColor);

            glBindVertexArray(0);

            CheckGLError();
        }

        virtual void Begin() override {
            assert(!gEngine->shader);
            glUseProgram(p);
            glActiveTexture(GL_TEXTURE0/* + textureUnit*/);
            glUniform1i(uTex0, 0);
            glUniform2f(uCxy, 2 / gEngine->windowSize.x, -2 / gEngine->windowSize.y * gEngine->flipY);  // flip y for spine
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
            glBufferData(GL_ARRAY_BUFFER, sizeof(SpineVertexData) * vertsCount, verts.get(), GL_STREAM_DRAW);

            glBindTexture(GL_TEXTURE_2D, lastTextureId);
            glDrawArrays(GL_TRIANGLES, 0, vertsCount);
            CheckGLError();

            drawVerts += vertsCount;
            drawCall += 1;

            lastTextureId = 0;
            vertsCount = 0;
        }

        SpineVertexData* Draw(GLuint texId, int32_t numVerts) {
            assert(numVerts <= vertsCap);
            if (vertsCount + numVerts > vertsCap || (lastTextureId && lastTextureId != texId)) {
                Commit();
            }
            lastTextureId = texId;
            auto vc = vertsCount;
            vertsCount += numVerts;
            return &verts[vc];
        }

        SpineVertexData* Draw(Ref<GLTexture> const& tex, int32_t numVerts) {
            return Draw(tex->GetValue(), numVerts);
        }
    };

}
