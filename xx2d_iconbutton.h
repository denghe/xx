#pragma once
#include <xx2d_button.h>

namespace xx {

	struct Image : Node {
		TinyFrame frame;
		RGBA8 color;
		float colorplus;

		void Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, TinyFrame frame_, RGBA8 color_ = RGBA8_White, float colorplus_ = 1) {
			Node::Init(z_, position_, scale_, anchor_, { frame_.texRect.w,frame_.texRect.h });
			frame = frame_;
			color = color_;
			colorplus = colorplus_;
		}

		void Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, Ref<Frame> const& frame_, RGBA8 color_ = RGBA8_White, float colorplus_ = 1) {
			Init(z_, position_, scale_, anchor_, { frame_->tex, frame_->textureRect }, color_, colorplus_);
		}

		void Draw() override {
			auto q = EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderQuadInstance).Draw(frame.tex->GetValue(), 1);
			q->pos = worldMinXY;
			q->anchor = { 0, 0.5f };
			q->scale = worldScale;
			q->radians = {};
			q->colorplus = colorplus;
			q->color = { color.r, color.g, color.b, (uint8_t)(color.a * alpha) };
			q->texRect.data = frame.texRect.data;
		}
	};

	struct IconButton : Button {
		Shared<Image> icon;

		void Init(int z_, XY const& position_, XY const& anchor_, Scale9SpriteConfig const& cfg_, TinyFrame const& icon_, std::u32string_view const& txt_) {
			z = z_;
			position = position_;
			anchor = anchor_;
			MakeContent(cfg_, txt_);

			if (icon_.texRect.h) {
				auto s = (size.y - cfg_.iconPadding * 2) /  icon_.texRect.h;
				icon = MakeChildren<Image>();
				icon->Init(z + 1, { cfg_.iconPadding, size.y * 0.5f }, s, {}, icon_);
			}

			FillTransRecursive();
		}

		template<typename F>
		void Init(int z_, XY const& position_, XY const& anchor_, Scale9SpriteConfig const& cfg_, TinyFrame const& icon_, std::u32string_view const& txt_, F&& callback) {
			Init(z_, position_, anchor_, cfg_, icon_, txt_);
			onClicked = std::forward<F>(callback);
		}

		template<typename F>
		void Init(int z_, XY const& position_, XY const& anchor_, Scale9SpriteConfig const& cfg_, Ref<Frame> const& icon_, std::u32string_view const& txt_, F&& callback) {
			Init(z_, position_, anchor_, cfg_, TinyFrame{ icon_->tex, icon_->textureRect }, txt_, std::forward<F>(callback));
		}
	};

}
