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
			FillTrans();
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


	struct ImageButton : MouseEventHandlerNode {
		constexpr static float cBgColorplusHighlight{ 1.5 };
		constexpr static float cBgColorplusNormal{ 1 };
		constexpr static float cBgColorplusDark{ 0.75 };
		constexpr static float cBgChangeColorplusSpeed{ cBgColorplusHighlight / 0.2 };

		std::function<void()> onClicked = [] { CoutN("ImageButton clicked."); };
		TinyFrame frame;
		RGBA8 color;
		float colorplus;

		// example: ib.Init(.....).onClicked = [](){ ... };
		ImageButton& Init(int z_, XY const& pos_, XY const& anchor_, XY const& size_, TinyFrame frame_, RGBA8 color_ = RGBA8_White, float colorplus_ = 1) {
			Node::Init(z_, pos_, XY{ 1,1 }, anchor_, size_);
			frame = frame_;
			color = color_;
			colorplus = colorplus_;
			FillTrans();
			return *this;
		}
		ImageButton& Init(int z_, XY const& pos_, XY const& anchor_, XY const& size_, Ref<Frame> frame_, RGBA8 color_ = RGBA8_White, float colorplus_ = 1) {
			return Init(z_, pos_, anchor_, size_, TinyFrame{ frame_->tex, frame_->textureRect }, color_, colorplus_);
		}

		void Draw() override {
			auto q = EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderQuadInstance).Draw(frame.tex->GetValue(), 1);
			q->pos = position;
			q->anchor = anchor;
			q->scale = worldScale * (size / XY{ frame.texRect.w, frame.texRect.h });
			q->radians = {};
			q->colorplus = colorplus;
			q->color = { color.r, color.g, color.b, (uint8_t)(color.a * alpha) };
			q->texRect.data = frame.texRect.data;
		}

		TaskGuard bgChangeColorplus;
		Task<> BgHighlight() {
			auto step = cBgChangeColorplusSpeed / gEngine->framePerSeconds;
		LabBegin:
			while (true) {
				if (colorplus < cBgColorplusHighlight) {
					colorplus += step;
				}
				co_yield 0;
				if (!MousePosInArea()) break;
			}
			for (; colorplus > cBgColorplusNormal; colorplus -= step) {
				co_yield 0;
				if (MousePosInArea()) goto LabBegin;
			}
			colorplus = cBgColorplusNormal;
			co_return;
		}

		XY mPos{};
		virtual void OnMouseDown() override {
			assert(!gEngine->mouseEventHandler);
			gEngine->mouseEventHandler = WeakFromThis(this);
			bgChangeColorplus.Clear();
			colorplus = cBgColorplusDark;
			mPos = gEngine->mouse.pos;
		}

		virtual void OnMouseMove() override {
			if (gEngine->mouseEventHandler.pointer() != this) {		// mbtn up
				if (!bgChangeColorplus) {
					bgChangeColorplus(gEngine->tasks, BgHighlight());
				}
			}
			else if (scissor) {
				// check scroll view & detect move distance & set handler to sv
				if (Calc::DistancePow2(mPos, gEngine->mouse.pos) > 3 * 3) {
					gEngine->mouseEventHandler.Reset();
					colorplus = cBgColorplusNormal;
					scissor.Lock().As<MouseEventHandlerNode>()->OnMouseDown();
				}
			}
		}

		virtual void OnMouseUp() override {
			assert(gEngine->mouseEventHandler.pointer() == this);
			gEngine->mouseEventHandler.Reset();
			colorplus = cBgColorplusNormal;
			if (MousePosInArea()) {
				onClicked();
			}
		}
	};

}
