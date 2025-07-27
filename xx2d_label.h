#pragma once
#include <xx2d_node.h>

namespace xx {

	struct Label : Node {
		Listi32<TinyFrame const*> fs;
		RGBA8 color;

		template<typename S>
		Label& Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, RGBA8 color_, S const& txt_) {
			Init(z_, position_, scale_, anchor_, color_);
			SetText(txt_);
			return *this;
		}

		Label& Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, RGBA8 color_) {
			z = z_;
			position = position_;
			anchor = anchor_;
			scale = scale_;
			color = color_;
			return *this;
		}

		// S : literal string u8string [view]
		template<typename S>
		void SetText(S const& txt_) {
			auto len = (int32_t)StrLen(txt_);
			if (!len) {
				fs.Clear();
				return;
			}
			fs.Resize(len);
			auto& ctc = EngineBase2::Instance().ctcDefault;
			size = { 0, (float)ctc.canvasHeight * scale.y };
			for (int i = 0; i < len; ++i) {
				fs[i] = &ctc.Find(txt_[i]);
				size.x += fs[i]->texRect.w * scale.x;
			}
			FillTrans();
		}

		void SetText() {
			fs.Clear();
		}

		virtual void Draw() override {
			if (fs.Empty()) return;
			auto& shader = EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderQuadInstance);
			auto basePos = worldMinXY;
			for (auto& f : fs) {
				auto& q = *shader.Draw(f->tex->GetValue(), 1);
				q.pos = basePos;
				q.anchor = { 0.f, 0.f };
				q.scale = worldScale;
				q.radians = {};
				q.colorplus = 1;
				q.color = color;
				q.texRect.data = f->texRect.data;
				basePos.x += f->texRect.w * worldScale.x;
			}
		}
	};

}
