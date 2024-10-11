#pragma once
#include <xx2d_base1.h>

namespace xx {

	struct Scissor {
		XY bakWndSiz{};
		std::array<uint32_t, 3> bakBlend;

		template<typename Func>
		void DirectDrawTo(XY const& pos, XY const& wh, Func&& func) {
			DirectBegin(pos.x, pos.y, wh.x, wh.y);
			func();
			DirectEnd();
		}

		template<typename Func>
		void OffsetDrawTo(XY const& pos, XY const& wh, Func&& func) {
			OffsetBegin(pos.x, pos.y, wh.x, wh.y);
			func();
			OffsetEnd();
		}

	protected:
		XX_INLINE void DirectBegin(float x, float y, float w, float h) {
			auto& eb = EngineBase1::Instance();
			eb.ShaderEnd();
			bakBlend = eb.blend;
			eb.GLBlendFunc(eb.blendDefault);
			X_Y<GLsizei> wp{ GLsizei(eb.windowSize_2.x + x), GLsizei(eb.windowSize_2.y + y) };
			glScissor(wp.x, wp.y, (GLsizei)w, (GLsizei)h);
			glEnable(GL_SCISSOR_TEST);
		}

		XX_INLINE void DirectEnd() {
			auto& eb = EngineBase1::Instance();
			eb.ShaderEnd();
			glDisable(GL_SCISSOR_TEST);
			eb.GLBlendFunc(bakBlend);
		}


		XX_INLINE void OffsetBegin(float x, float y, float w, float h) {
			auto& eb = EngineBase1::Instance();
			eb.ShaderEnd();
			bakWndSiz = eb.windowSize;
			bakBlend = eb.blend;
			X_Y<GLsizei> wp{ GLsizei(eb.windowSize_2.x + x), GLsizei(eb.windowSize_2.y + y) };
			eb.SetWindowSize(w, h);
			eb.GLBlendFunc(eb.blendDefault);
			glViewport(wp.x, wp.y, (GLsizei)w, (GLsizei)h);
			glScissor(wp.x, wp.y, (GLsizei)w, (GLsizei)h);
			glEnable(GL_SCISSOR_TEST);
		}

		XX_INLINE void OffsetEnd() {
			auto& eb = EngineBase1::Instance();
			eb.ShaderEnd();
			glDisable(GL_SCISSOR_TEST);
			eb.SetWindowSize(bakWndSiz.x, bakWndSiz.y);
			eb.GLViewport();
			eb.GLBlendFunc(bakBlend);
		}
	};

}
