#pragma once
#include <xx2d_button.h>

namespace xx {

	enum class ImageRadians : int32_t {
		Zero = 0,		// 0'
		PiDiv2 = 1,		// 90'
		Pi = 2,			// 180'
		NegPiDiv2 = -1,	// -90'
		NegPi = -2		// -180'
	};

	struct Image : Node {
		TinyFrame frame{};
		RGBA8 color{};
		float colorplus{};
		float radians{};
		XY anchorSize{};

		void Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, TinyFrame frame_, ImageRadians radians_ = ImageRadians::Zero, RGBA8 color_ = RGBA8_White, float colorplus_ = 1) {
			auto r = (int32_t)radians_;
			XY siz;
			if (r == 1 || r == -1) {
				siz = { frame_.texRect.h * scale_.y, frame_.texRect.w * scale_.x };
			}
			else {
				siz = { frame_.texRect.w * scale_.x, frame_.texRect.h * scale_.y };
			}
			anchorSize = siz * anchor;
			radians = float(M_PI) * 0.5f * r;
			Node::Init(z_, position_, scale_, anchor_, siz);
			frame = frame_;
			color = color_;
			colorplus = colorplus_;
		}

		void Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, Ref<Frame> const& frame_, ImageRadians radians_ = ImageRadians::Zero, RGBA8 color_ = RGBA8_White, float colorplus_ = 1) {
			Init(z_, position_, scale_, anchor_, { frame_->tex, frame_->textureRect }, radians_, color_, colorplus_);
		}

		void Draw() override {
			auto q = EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderQuadInstance).Draw(frame.tex->GetValue(), 1);
			q->pos = worldMinXY + anchorSize;
			q->anchor = 0.5f;
			q->scale = worldScale;
			q->radians = radians;
			q->colorplus = colorplus;
			q->color = { color.r, color.g, color.b, (uint8_t)(color.a * alpha) };
			q->texRect.data = frame.texRect.data;
		}
	};

	struct IconButton : Button {
		Shared<Image> icon;

		IconButton& Init(int z_, XY const& pos_, XY const& anchor_, Scale9SpriteConfig const& cfg_, TinyFrame const& icon_, std::u32string_view const& txt_, ImageRadians iconRadians_ = ImageRadians::Zero, RGBA8 iconColor_ = RGBA8_White, float iconColorplus_ = 1) {
			z = z_;
			position = pos_;
			anchor = anchor_;
			MakeContent(cfg_, txt_);
			if (icon_.texRect.h) {
				auto s = (size.y - cfg_.iconPadding * 2) /  icon_.texRect.h;
				icon = MakeChildren<Image>();
				icon->Init(z + 1, { cfg_.iconPadding, size.y * 0.5f }, s, { 0, 0.5f }, icon_, iconRadians_, iconColor_, iconColorplus_);
			}
			FillTransRecursive();
			return *this;
		}
		IconButton& Init(int z_, XY const& pos_, XY const& anchor_, Scale9SpriteConfig const& cfg_, Ref<Frame> const& icon_, std::u32string_view const& txt_, ImageRadians iconRadians_ = ImageRadians::Zero, RGBA8 iconColor_ = RGBA8_White, float iconColorplus_ = 1) {
			return Init(z_, pos_, anchor_, cfg_, TinyFrame{ icon_->tex, icon_->textureRect }, txt_, iconRadians_, iconColor_, iconColorplus_);
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

	// usually for full screen click avoid
	struct SwallowButton : MouseEventHandlerNode {

		std::function<void()> onClicked = [] { CoutN("SwallowButton clicked."); };

		// example: sb.Init(.....).onClicked = [](){ ... };
		SwallowButton& Init(int z_, XY const& pos_ = {}, XY const& anchor_ = 0.5f, XY const& size_ = gEngine->windowSize) {
			Node::Init(z_, pos_, XY{ 1,1 }, anchor_, size_);
			return *this;
		}

		void Draw() override {}

		XY mPos{};
		virtual void OnMouseDown() override {
			assert(!gEngine->mouseEventHandler);
			gEngine->mouseEventHandler = WeakFromThis(this);
			mPos = gEngine->mouse.pos;
		}

		virtual void OnMouseMove() override {
			if (gEngine->mouseEventHandler.pointer() != this) {		// mbtn up
			}
			else if (scissor) {
				// check scroll view & detect move distance & set handler to sv
				if (Calc::DistancePow2(mPos, gEngine->mouse.pos) > 3 * 3) {
					gEngine->mouseEventHandler.Reset();
					scissor.Lock().As<MouseEventHandlerNode>()->OnMouseDown();
				}
			}
		}

		virtual void OnMouseUp() override {
			assert(gEngine->mouseEventHandler.pointer() == this);
			gEngine->mouseEventHandler.Reset();
			if (MousePosInArea()) {
				onClicked();
			}
		}
	};


	struct EmptyButton : Button {

		EmptyButton& Init(int z_, XY const& pos_, XY const& anchor_, Scale9SpriteConfig const& cfg_, XY const& size_) {
			z = z_;
			position = pos_;
			anchor = anchor_;
			Node::Init(z_, pos_, 1, anchor_, size_);
		
			bg = MakeChildren<Scale9Sprite>();
			bg->Init(z, {}, cfg_.borderScale, {}, size / cfg_.borderScale, cfg_);

			return *this;
		}

	};

}
