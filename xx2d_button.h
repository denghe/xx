﻿#pragma once
#include <xx2d_node_derived.h>
#include <xx2d_label.h>
#include <xx2d_scale9sprite.h>
#include <xx2d_scrollview.h>

namespace xx {

	struct Button : MouseEventHandlerNode {
		constexpr static float cBgColorplusHighlight{ 1.5 };
		constexpr static float cBgColorplusNormal{ 1 };
		constexpr static float cBgColorplusDark{ 0.75 };
		constexpr static float cBgChangeColorplusSpeed{ cBgColorplusHighlight / 0.2 };
		constexpr static XY cTextPadding{ 20, 5 };

		std::function<void()> onClicked = [] { CoutN("button clicked."); };

		Shared<Label> lbl;
		Shared<Scale9Sprite> bg;

		// todo: scale support
		void Init(int z_, XY const& position_, XY const& anchor_, Scale9SpriteConfig const& cfg_, std::u32string_view const& txt_, float borderScale = 1.f) {
			z = z_;
			position = position_;
			anchor = anchor_;

			auto cornerSize = cfg_.GetCornerSize();
			lbl = MakeChildren<Label>();
			lbl->Init(z + 1, (cornerSize + cTextPadding) / 2, cfg_.texScale, {}, RGBA8_White, txt_);
			size = lbl->size + cornerSize + cTextPadding;

			bg = MakeChildren<Scale9Sprite>();
			bg->Init(z, {}, { borderScale, borderScale }, {}, size / borderScale, cfg_);

			FillTransRecursive();
		}

		template<typename F>
		void Init(int z_, XY const& position_, XY const& anchor_, Scale9SpriteConfig const& cfg_, std::u32string_view const& txt_, F&& callback, float borderScale = 1.f) {
			Init(z_, position_, anchor_, cfg_, txt_, borderScale);
			onClicked = std::forward<F>(callback);
		}

		TaskGuard bgChangeColorplus;
		Task<> BgHighlight() {
			auto step = cBgChangeColorplusSpeed / gEngine->framePerSeconds;
		LabBegin:
			while (true) {
				if (bg->colorplus < cBgColorplusHighlight) {
					bg->colorplus += step;
				}
				co_yield 0;
				if (!MousePosInArea()) break;
			}
			for (; bg->colorplus > cBgColorplusNormal; bg->colorplus -= step) {
				co_yield 0;
				if (MousePosInArea()) goto LabBegin;
			}
			bg->colorplus = cBgColorplusNormal;
			co_return;
		}

		XY mPos{};
		virtual void OnMouseDown() override {
			assert(!gEngine->mouseEventHandler);
			gEngine->mouseEventHandler = WeakFromThis(this);
			bgChangeColorplus.Clear();
			bg->colorplus = cBgColorplusDark;
			mPos = gEngine->mouse.pos;
		}

		virtual void OnMouseMove() override {
			if (gEngine->mouseEventHandler.pointer() != this) {		// mbtn up
				if (!bgChangeColorplus) {
					bgChangeColorplus(gEngine->tasks, BgHighlight());
				}
			} else if (scissor) {
				// check scroll view & detect move distance & set handler to sv
				if (Calc::DistancePow2(mPos, gEngine->mouse.pos) > 3 * 3) {
					gEngine->mouseEventHandler.Reset();
					bg->colorplus = cBgColorplusNormal;
					scissor.Lock().As<MouseEventHandlerNode>()->OnMouseDown();
				}
			}
		}

		virtual void OnMouseUp() override {
			assert(gEngine->mouseEventHandler.pointer() == this);
			gEngine->mouseEventHandler.Reset();
			bg->colorplus = cBgColorplusNormal;
			if (MousePosInArea()) {
				onClicked();
			}
		}
	};

}
