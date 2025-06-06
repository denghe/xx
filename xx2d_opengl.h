﻿#pragma once
#include <xx2d_includes.h>

namespace xx {

#ifndef NDEBUG
	inline void CheckGLErrorAt(const char* file, int line, const char* func) {
		if (auto e = glGetError(); e != GL_NO_ERROR) {
			printf("glGetError() == %d file = %s line = %d\n", e, file, line);
			xx_assert(false);
		}
	}
#define CheckGLError() ::xx::CheckGLErrorAt(__FILE__, __LINE__, __FUNCTION__)
#else
#define CheckGLError() ((void)0)
#endif

	enum class GLResTypes {
		Shader, Program, VertexArrays, Buffer, Texture, FrameBuffer
	};

	template<GLResTypes T, typename...VS>
	struct GLRes {

		std::tuple<GLuint, VS...> vs;

		GLRes(GLuint i) : vs(std::make_tuple(i)) {}

		template<typename...Args>
		GLRes(GLuint i, Args&&... args) : vs(std::make_tuple(i, std::forward<Args>(args)...)) {}

		operator GLuint const& () const { return std::get<0>(vs); }
		GLuint const& GetValue() const { return std::get<0>(vs); }
		GLuint* GetValuePointer() { return &std::get<0>(vs); }

		GLRes(GLRes const&) = delete;
		GLRes& operator=(GLRes const&) = delete;
		GLRes() = default;
		GLRes(GLRes&& o) noexcept {
			std::swap(vs, o.vs);
		}
		GLRes& operator=(GLRes&& o) noexcept {
			std::swap(vs, o.vs);
			return *this;
		}

		~GLRes() {
			if (!std::get<0>(vs)) return;
			if constexpr (T == GLResTypes::Shader) {
				glDeleteShader(std::get<0>(vs));
			}
			if constexpr (T == GLResTypes::Program) {
				glDeleteProgram(std::get<0>(vs));
			}
			if constexpr (T == GLResTypes::VertexArrays) {
				glDeleteVertexArrays(1, &std::get<0>(vs));
			}
			if constexpr (T == GLResTypes::Buffer) {
				glDeleteBuffers(1, &std::get<0>(vs));
			}
			if constexpr (T == GLResTypes::Texture) {
				glDeleteTextures(1, &std::get<0>(vs));
			}
			if constexpr (T == GLResTypes::FrameBuffer) {
				glDeleteFramebuffers(1, &std::get<0>(vs));
			}
			std::get<0>(vs) = 0;
		}
	};

	using GLShader = GLRes<GLResTypes::Shader>;

	using GLProgram = GLRes<GLResTypes::Program>;

	using GLVertexArrays = GLRes<GLResTypes::VertexArrays>;

	using GLBuffer = GLRes<GLResTypes::Buffer>;

	using GLFrameBuffer = GLRes<GLResTypes::FrameBuffer>;

	using GLTextureCore = GLRes<GLResTypes::Texture>;


	template<GLuint filter = GL_NEAREST /* GL_LINEAR */, GLuint wraper = GL_CLAMP_TO_EDGE /* GL_REPEAT */>
	void GLTexParameteri() {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wraper);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wraper);
	}

	inline void GLTexParameteri(GLuint filter, GLuint wraper) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wraper);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wraper);
	}

	template<bool exitBind0 = false, GLuint filter = GL_NEAREST /* GL_LINEAR */, GLuint wraper = GL_CLAMP_TO_EDGE /* GL_REPEAT */>
	GLuint GLGenTextures() {
		GLuint t{};
		glGenTextures(1, &t);
		glBindTexture(GL_TEXTURE_2D, t);
		GLTexParameteri<filter, wraper>();
		if constexpr (exitBind0) {
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		//printf("GLGenTextures t = %d\n", t);
		return t;
	}

	struct GLTexture : GLRes<GLResTypes::Texture, GLsizei, GLsizei, std::string> {
		using BT = GLRes<GLResTypes::Texture, GLsizei, GLsizei, std::string>;
		using BT::BT;

		template<GLuint filter = GL_NEAREST /* GL_LINEAR */, GLuint wraper = GL_CLAMP_TO_EDGE /* GL_REPEAT */>
		void SetGLTexParm() {
			glBindTexture(GL_TEXTURE_2D, GetValue());
			GLTexParameteri<filter, wraper>();
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void SetGLTexParm(GLuint filter, GLuint wraper) {
			glBindTexture(GL_TEXTURE_2D, GetValue());
			GLTexParameteri(filter, wraper);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void GenerateMipmap() {
			glBindTexture(GL_TEXTURE_2D, GetValue());
			glGenerateMipmap(GetValue());
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		auto const& Width() const { return std::get<1>(vs); }
		auto const& Height() const { return std::get<2>(vs); }
		auto const& FileName() const { return std::get<3>(vs); }

		template<GLuint filter = GL_NEAREST /* GL_LINEAR */, GLuint wraper = GL_CLAMP_TO_EDGE /* GL_REPEAT */>
		static GLTexture Create(uint32_t w, uint32_t h, bool hasAlpha) {
			auto t = GLGenTextures<false, filter, wraper>();
			auto c = hasAlpha ? GL_RGBA : GL_RGB;
			glTexImage2D(GL_TEXTURE_2D, 0, c, w, h, 0, c, GL_UNSIGNED_BYTE, {});
			glBindTexture(GL_TEXTURE_2D, 0);
			return GLTexture(t, w, h, "");
		}
	};

	struct GLVertTexture : GLRes<GLResTypes::Texture, GLsizei, GLsizei, int32_t, int32_t> {
		using BT = GLRes<GLResTypes::Texture, GLsizei, GLsizei, int32_t, int32_t>;
		using BT::BT;

		auto const& Width() const { return std::get<1>(vs); }
		auto const& Height() const { return std::get<2>(vs); }
		auto const& NumVerts() const { return std::get<3>(vs); }
		auto const& NumFrames() const { return std::get<4>(vs); }
	};

	struct GLTiledTexture : GLRes<GLResTypes::Texture, GLsizei, GLsizei, int32_t, int32_t> {
		using BT = GLRes<GLResTypes::Texture, GLsizei, GLsizei, int32_t, int32_t>;
		using BT::BT;

		auto const& Width() const { return std::get<1>(vs); }
		auto const& Height() const { return std::get<2>(vs); }
		auto const& SizeX() const { return std::get<3>(vs); }
		auto const& SizeY() const { return std::get<4>(vs); }
	};

	/**********************************************************************************************************************************/
	/**********************************************************************************************************************************/

	inline GLShader LoadGLShader(GLenum type, std::initializer_list<std::string_view>&& codes_) {
		assert(codes_.size() && (type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER));
		auto&& shader = glCreateShader(type);
		xx_assert(shader);
		std::vector<GLchar const*> codes;
		codes.resize(codes_.size());
		std::vector<GLint> codeLens;
		codeLens.resize(codes_.size());
		auto ss = codes_.begin();
		for (size_t i = 0; i < codes.size(); ++i) {
			codes[i] = (GLchar const*)ss[i].data();
			codeLens[i] = (GLint)ss[i].size();
		}
		glShaderSource(shader, (GLsizei)codes_.size(), codes.data(), codeLens.data());
		glCompileShader(shader);
		GLint r = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
		if (!r) {
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &r);	// fill txt len into r
			std::string s;
			if (r) {
				s.resize(r);
				glGetShaderInfoLog(shader, r, nullptr, s.data());	// copy txt to s
			}
			CoutN("glCompileShader failed: err msg = ", s);
			xx_assert(false);
		}
		return GLShader(shader);
	}

	inline GLShader LoadGLVertexShader(std::initializer_list<std::string_view>&& codes_) {
		return LoadGLShader(GL_VERTEX_SHADER, std::move(codes_));
	}

	inline GLShader LoadGLFragmentShader(std::initializer_list<std::string_view>&& codes_) {
		return LoadGLShader(GL_FRAGMENT_SHADER, std::move(codes_));
	}

	inline GLProgram LinkGLProgram(GLuint vs, GLuint fs) {
		auto program = glCreateProgram();
		xx_assert(program);
		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		GLint r = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &r);
		if (!r) {
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &r);
			std::string s;
			if (r) {
				s.resize(r);
				glGetProgramInfoLog(program, r, nullptr, s.data());
			}
			CoutN("glLinkProgram failed: err msg = ", s);
			xx_assert(false);
		}
		return GLProgram(program);
	}

	/**********************************************************************************************************************************/
	/**********************************************************************************************************************************/

	inline GLFrameBuffer MakeGLFrameBuffer() {
		GLuint f{};
		glGenFramebuffers(1, &f);
		//glBindFramebuffer(GL_FRAMEBUFFER, f);
		return GLFrameBuffer(f);
	}

	inline void BindGLFrameBufferTexture(GLuint f, GLuint t) {
		glBindFramebuffer(GL_FRAMEBUFFER, f);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t, 0);
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	}

	inline void UnbindGLFrameBuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// only support GL_RGBA, GL_UNSIGNED_BYTE
	inline void GLFrameBufferSaveTo(Data& tar, GLint x, GLint y, GLsizei width, GLsizei height) {
		auto siz = width * height * 4;
		tar.Resize(siz);
		glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tar.buf);
	}


	/**********************************************************************************************************************************/
	/**********************************************************************************************************************************/

	template<GLuint filter = GL_NEAREST /* GL_LINEAR */, GLuint wraper = GL_CLAMP_TO_EDGE /* GL_REPEAT */>
	inline GLuint LoadGLTexture_core(int textureUnit = 0) {
		GLuint t{};
		glGenTextures(1, &t);
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, t);
		GLTexParameteri<filter, wraper>();
		return t;
	}

	inline GLTexture LoadGLTexture(std::string_view const& buf, std::string_view const& fullPath) {
		xx_assert(buf.size() > 12);

		/***********************************************************************************************************************************/
		// etc 2.0 / pkm2

		if (buf.starts_with("PKM 20"sv) && buf.size() >= 16) {
			auto p = (uint8_t*)buf.data();
			uint16_t format = (p[6] << 8) | p[7];				// 1 ETC2_RGB_NO_MIPMAPS, 3 ETC2_RGBA_NO_MIPMAPS
			uint16_t encodedWidth = (p[8] << 8) | p[9];			// 4 align width
			uint16_t encodedHeight = (p[10] << 8) | p[11];		// 4 align height
			uint16_t width = (p[12] << 8) | p[13];				// width
			uint16_t height = (p[14] << 8) | p[15];				// height
			xx_assert(width > 0 && height > 0 && encodedWidth >= width && encodedWidth - width < 4 && encodedHeight >= height && encodedHeight - height < 4);
			if (format == 1) {
				xx_assert(buf.size() == 16 + encodedWidth * encodedHeight / 2);
			} else if (format == 3) {
				xx_assert(buf.size() == 16 + encodedWidth * encodedHeight);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 8 - 4 * (width & 0x1));
			} else {
				CoutN("unsppported PKM 20 format. only support ETC2_RGB_NO_MIPMAPS & ETC2_RGBA_NO_MIPMAPS. fn = ", fullPath);
				xx_assert(false);
			}
			auto t = LoadGLTexture_core();
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, format == 3 ? GL_COMPRESSED_RGBA8_ETC2_EAC : GL_COMPRESSED_RGB8_ETC2, (GLsizei)width, (GLsizei)height, 0, (GLsizei)(buf.size() - 16), p + 16);
			glBindTexture(GL_TEXTURE_2D, 0);
			CheckGLError();
			return { t, width, height, fullPath };
		}

		/***********************************************************************************************************************************/
		// astc

		else if (buf.starts_with("\x13\xab\xa1\x5c"sv) && buf.size() >= 16) {
			struct Header {
				uint8_t magic[4], block_x, block_y, block_z, dim_x[3], dim_y[3], dim_z[3];
			};
			auto p = (uint8_t*)buf.data();
			auto& header = *(Header*)p;

			uint32_t block_x = std::max((uint32_t)header.block_x, 1u);
			uint32_t block_y = std::max((uint32_t)header.block_y, 1u);
			GLsizei w = header.dim_x[0] + (header.dim_x[1] << 8) + (header.dim_x[2] << 16);
			GLsizei h = header.dim_y[0] + (header.dim_y[1] << 8) + (header.dim_y[2] << 16);

			int fmt{};
			GLuint t;
			if (w == 0 || h == 0 || block_x < 4 || block_y < 4) goto LabError;

			switch (block_x) {
			case 4:
				if (block_y == 4) {
					fmt = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
					break;
				} else goto LabError;
			case 5:
				if (block_y == 4) {
					fmt = GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
					break;
				} else if (block_y == 5) {
					fmt = GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
					break;
				} else goto LabError;
			case 6:
				if (block_y == 5) {
					fmt = GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
					break;
				} else if (block_y == 6) {
					fmt = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
					break;
				} else goto LabError;
			case 8:
				if (block_y == 5) {
					fmt = GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
					break;
				} else if (block_y == 6) {
					fmt = GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
					break;
				} else if (block_y == 8) {
					fmt = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
					break;
				} else goto LabError;
			case 10:
				if (block_y == 5) {
					fmt = GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
					break;
				} else if (block_y == 6) {
					fmt = GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
					break;
				} else if (block_y == 8) {
					fmt = GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
					break;
				} else if (block_y == 10) {
					fmt = GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
					break;
				} else goto LabError;
			case 12:
				if (block_y == 10) {
					fmt = GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
					break;
				} else if (block_y == 12) {
					fmt = GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
					break;
				} else goto LabError;
			default:
				goto LabError;
			}

			t = LoadGLTexture_core();
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, (GLsizei)(buf.size() - 16), p + 16);
			glBindTexture(GL_TEXTURE_2D, 0);
			CheckGLError();
			return { t, w, h, fullPath };

		LabError:
			CoutN("bad astc file header. fn = ", fullPath);
			xx_assert(false);
		}

		/***********************************************************************************************************************************/
		// png

		else if (buf.starts_with("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"sv)) {
			int w, h, comp;
			if (auto image = stbi_load_from_memory((stbi_uc*)buf.data(), (int)buf.size(), &w, &h, &comp, 0)) {
				auto c = comp == 3 ? GL_RGB : GL_RGBA;
				if (comp == 4) {
					glPixelStorei(GL_UNPACK_ALIGNMENT, 8 - 4 * (w & 0x1));
				}
				auto t = LoadGLTexture_core();
				glTexImage2D(GL_TEXTURE_2D, 0, c, w, h, 0, c, GL_UNSIGNED_BYTE, image);
				glBindTexture(GL_TEXTURE_2D, 0);
				CheckGLError();
				stbi_image_free(image);
				return { t, w, h, fullPath };
			} else {
				CoutN("failed to load texture. fn = ", fullPath);
				xx_assert(false);
			}
		}

		/***********************************************************************************************************************************/
		// jpg

		else if (buf.starts_with("\xFF\xD8"sv)) {
			int w, h, comp;
			if (auto image = stbi_load_from_memory((stbi_uc*)buf.data(), (int)buf.size(), &w, &h, &comp, 0)) {
				auto t = LoadGLTexture_core();
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
				glBindTexture(GL_TEXTURE_2D, 0);
				CheckGLError();
				stbi_image_free(image);
				return { t, w, h, fullPath };
			} else {
				CoutN("failed to load texture. fn = ", fullPath);
				xx_assert(false);
			}
		}

		/***********************************************************************************************************************************/
		// todo: more format support here

		CoutN("unsupported texture type. fn = ", fullPath);
		xx_assert(false);
		return {};
	}

	// data's bytes len == w * h * sizeof(colorFormat)
	template<GLuint filter = GL_NEAREST /* GL_LINEAR */, GLuint wraper = GL_CLAMP_TO_EDGE /* GL_REPEAT */>
	inline GLTexture LoadGLTexture(void* data, GLsizei w, GLsizei h, GLint colorFormat = GL_RGBA) {
		auto t = LoadGLTexture_core<filter, wraper>();
		glPixelStorei(GL_UNPACK_ALIGNMENT, 8 - 4 * (w & 0x1));
		glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, w, h, 0, colorFormat, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		return { t, w, h, "::memory::" };
	}

	inline GLVertTexture LoadGLVertTexture(void* data, GLsizei w, GLsizei h, int32_t numVerts, int32_t numFrames) {
		auto t = LoadGLTexture_core<GL_NEAREST, GL_REPEAT>();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		return { t, w, h, numVerts, numFrames };
	}

	inline GLTiledTexture LoadGLTiledTexture(void* data, GLsizei w, GLsizei h, XYi size) {
		auto t = LoadGLTexture_core<GL_NEAREST, GL_REPEAT>();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		return { t, w, h, size.x, size.y };
	}

}
