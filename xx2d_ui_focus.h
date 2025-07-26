#pragma once
#include <xx2d_node_derived.h>
#include <xx2d_label.h>
#include <xx2d_scale9sprite.h>
#include <xx2d_scrollview.h>

namespace xx {

	struct FocusBase : MouseEventHandlerNode {
		static constexpr uint64_t cFocusBaseTypeId{ 0xFBFBFBFBFBFBFBFB };	// unique. need store into ud
		bool isFocus{};
		virtual void SetFocus() { assert(!isFocus); isFocus = true; };
		virtual void LostFocus() { assert(isFocus); isFocus = false; };
		// todo: focus navigate? prev next?
	};


	struct FocusButton : FocusBase {
		xx::Ref<Scale9SpriteConfig> cfgNormal, cfgHighlight;
		XY fixedSize{};

		std::function<void()> onClicked = [] { CoutN("FocusButton clicked."); };
		std::function<void()> onFocus = [] {};	// play sound?

		FocusButton& Init(int z_, XY position_, XY anchor_
			, xx::Ref<Scale9SpriteConfig> cfgNormal_, xx::Ref<Scale9SpriteConfig> cfgHighlight_
			, std::u32string_view txtLeft_ = U"", std::u32string_view txtRight_ = U"", XY fixedSize_ = {}) {
			assert(children.Empty());
			ud = cFocusBaseTypeId;
			isFocus = false;
			z = z_;
			position = position_;
			anchor = anchor_;
			cfgNormal = std::move(cfgNormal_);
			cfgHighlight = std::move(cfgHighlight_);
			fixedSize = fixedSize_;

			auto& cfg = isFocus ? *cfgHighlight : *cfgNormal;
			auto lblLeft = MakeChildren<Label>();
			lblLeft->Init(z + 1, { cfg.txtPadding.x, cfg.txtPaddingRightBottom.y }, cfg.txtScale, {}, cfg.txtColor, txtLeft_);
			if (fixedSize.x > 0) {
				size = fixedSize;
			}
			else {
				size = lblLeft->size + cfg.txtPadding + cfg.txtPaddingRightBottom;
			}
			if (txtRight_.size()) {
				MakeChildren<Label>()->Init(z + 1, { size.x - cfg.txtPadding.x, cfg.txtPaddingRightBottom.y }, cfg.txtScale, { 1, 0 }, cfg.txtColor, txtRight_);
			}
			MakeChildren<Scale9Sprite>()->Init(z, {}, cfg.borderScale, {}, size / cfg.borderScale, cfg);
			FillTransRecursive();
			return *this;
		}

		void ApplyCfg() {
			assert(children.len == 2 || children.len == 3);		// lblLeft [, lblRight], bg
			auto& cfg = isFocus ? *cfgHighlight : *cfgNormal;
			auto lblLeft = (Label*)children[0].pointer;
			lblLeft->Init(z + 1, { cfg.txtPadding.x, cfg.txtPaddingRightBottom.y }, cfg.txtScale, {}, cfg.txtColor);
			if (fixedSize.x > 0) {
				size = fixedSize;
			}
			else {
				size = lblLeft->size + cfg.txtPadding + cfg.txtPaddingRightBottom;
			}
			Scale9Sprite* bg;
			if (children.len == 3) {
				((Label*)children[1].pointer)->Init(z + 1, {size.x - cfg.txtPadding.x, cfg.txtPaddingRightBottom.y}, cfg.txtScale, {1, 0}, cfg.txtColor);
				bg = (Scale9Sprite*)children[2].pointer;
			}
			else {
				bg = (Scale9Sprite*)children[1].pointer;
			}
			bg->Init(z, {}, cfg.borderScale, {}, size / cfg.borderScale, cfg);
			FillTransRecursive();
		}

		Label& LabelLeft() {
			assert(children.len == 2 || children.len == 3);
			return *(Label*)children[0].pointer;
		}

		Label& LabelRight() {
			assert(children.len == 3);
			return *(Label*)children[1].pointer;
		}

		void SetFocus() override {
			assert(!isFocus);
			isFocus = true;
			ApplyCfg();
			gEngine->mouseEventHandler = xx::WeakFromThis(this);
			onFocus();
			//xx::CoutN("SetFocus");
		}

		void LostFocus() override {
			assert(isFocus);
			isFocus = false;
			ApplyCfg();
			gEngine->mouseEventHandler.Reset();
			//xx::CoutN("LostFocus");
		}

		// todo: enable disable

		virtual void OnMouseDown() override {
			if (MousePosInArea()) {
				onClicked();
			}
		}

		virtual void OnMouseMove() override {
			if (MousePosInArea()) {
				if (isFocus) return;
				SetFocus();
			}
			else {
				if (!isFocus) return;
				LostFocus();
				gEngine->mouseEventHandler.Reset();
			}
		}
	};


	struct FocusSlider : FocusBase {
		xx::Ref<Scale9SpriteConfig> cfgNormal, cfgHighlight, cfgBar, cfgBlock;
		float height{}, widthTxtLeft{}, widthBar{}, widthTxtRight{};
		double value{}, valueBak{};	// 0 ~ 1
		bool blockMoving{};

		std::function<void(double)> onChanged = [](double v) { CoutN("FocusSlider changed. v = ", v); };
		std::function<std::u32string(double)> valueToString = [](double v)->std::u32string {
			return xx::StringU8ToU32(xx::ToString((int32_t)(v * 100)));
		};
		std::function<void()> onFocus = [] {};	// play sound?

		// InitBegin + set value/ToSting + InitEnd
		FocusSlider& InitBegin(int z_, XY position_, XY anchor_
			, xx::Ref<Scale9SpriteConfig> cfgNormal_, xx::Ref<Scale9SpriteConfig> cfgHighlight_
			, xx::Ref<Scale9SpriteConfig> cfgBar_, xx::Ref<Scale9SpriteConfig> cfgBlock_
			, float height_, float widthTxtLeft_, float widthBar_, float widthTxtRight_
			, std::u32string_view txtLeft_)
		{
			assert(children.Empty());
			ud = cFocusBaseTypeId;
			isFocus = false;
			z = z_;
			position = position_;
			anchor = anchor_;
			cfgNormal = std::move(cfgNormal_);
			cfgHighlight = std::move(cfgHighlight_);
			cfgBar = std::move(cfgBar_);
			cfgBlock = std::move(cfgBlock_);
			height = height_;
			widthTxtLeft = widthTxtLeft_;
			widthBar = widthBar_;
			widthTxtRight = widthTxtRight_;
			blockMoving = false;
			
			auto& cfg = isFocus ? *cfgHighlight : *cfgNormal;
			MakeChildren<Label>()->Init(z + 1, { cfg.txtPadding.x, cfg.txtPaddingRightBottom.y }, cfg.txtScale, {}, cfg.txtColor, txtLeft_);
			return *this;
		}

		FocusSlider& InitEnd() {
			assert(value >= 0 && value <= 1);
			auto& cfg = isFocus ? *cfgHighlight : *cfgNormal;
			size = { widthTxtLeft + widthBar + widthTxtRight, height };
			// todo: bar, block

			auto barMinWidth = cfgBar->center.x * 2. * cfgBar->borderScale;
			XY barSize{ std::max(widthBar * value, barMinWidth), height * 0.25f };
			XY barPos{ widthTxtLeft, (height - barSize.y) * 0.5f };
			MakeChildren<Scale9Sprite>()->Init(z + 1, barPos, cfgBar->borderScale, {}, barSize / cfgBar->borderScale, *cfgBar);

			XY blockSize{ height * 0.6f, height * 0.6f };
			XY blockPos{ widthTxtLeft + widthBar * value - blockSize.x * 0.5f, (height - blockSize.y) * 0.5f };
			MakeChildren<Scale9Sprite>()->Init(z + 2, blockPos, cfgBlock->borderScale, {}, blockSize / cfgBlock->borderScale, *cfgBlock);

			auto txtRight = valueToString(value);
			MakeChildren<Label>()->Init(z + 1, { size.x - cfg.txtPadding.x, cfg.txtPaddingRightBottom.y }, cfg.txtScale, { 1, 0 }, cfg.txtColor, txtRight);
			MakeChildren<Scale9Sprite>()->Init(z, {}, cfg.borderScale, {}, size / cfg.borderScale, cfg);
			FillTransRecursive();
			return *this;
		}

		FocusSlider& Init(int z_, XY position_, XY anchor_
			, xx::Ref<Scale9SpriteConfig> cfgNormal_, xx::Ref<Scale9SpriteConfig> cfgHighlight_
			, xx::Ref<Scale9SpriteConfig> cfgBar_, xx::Ref<Scale9SpriteConfig> cfgBlock_
			, float height_, float widthTxtLeft_, float widthBar_, float widthTxtRight_
			, std::u32string_view txtLeft_, double value_) {
			InitBegin(z_, position_, anchor_
				, std::move(cfgNormal_), std::move(cfgHighlight_), std::move(cfgBar_), std::move(cfgBlock_)
				, height_, widthTxtLeft_, widthBar_, widthTxtRight_, txtLeft_);
			SetValue(value_);
			InitEnd();
			return *this;
		}

		void ApplyCfg() {
			assert(children.len == 5);		// lblLeft, bar, block, lblRight, bg
			auto& cfg = isFocus ? *cfgHighlight : *cfgNormal;
			//auto lblLeft = (Label*)children[0].pointer;
			//lblLeft->Init(z + 1, { cfg.txtPadding.x, cfg.txtPaddingRightBottom.y }, cfg.txtScale, {}, cfg.txtColor);
			((Scale9Sprite*)children[4].pointer)->Init(z, {}, cfg.borderScale, {}, size / cfg.borderScale, cfg);
			//FillTransRecursive();
		}

		FocusSlider& SetValue(double v) {
			value = valueBak = v;
			return *this;
		}

		void ApplyValue() {
			assert(value >= 0 && value <= 1);

			auto barMinWidth = cfgBar->center.x * 2. * cfgBar->borderScale;
			XY barSize{ std::max(widthBar * value, barMinWidth), height * 0.25f };
			XY barPos{ widthTxtLeft, (height - barSize.y) * 0.5f };
			auto bar = ((Scale9Sprite*)children[1].pointer);
			bar->Init(z + 1, barPos, cfgBar->borderScale, {}, barSize / cfgBar->borderScale, *cfgBar);

			XY blockSize{ height * 0.6f, height * 0.6f };
			XY blockPos{ widthTxtLeft + widthBar * value - blockSize.x * 0.5f, (height - blockSize.y) * 0.5f };
			auto block = ((Scale9Sprite*)children[2].pointer);
			block->position = blockPos;
			block->FillTrans();

			LabelRight().SetText(valueToString(value));
		}

		Label& LabelLeft() {
			assert(children.len == 5);
			return *(Label*)children[0].pointer;
		}

		Label& LabelRight() {
			assert(children.len == 5);
			return *(Label*)children[3].pointer;
		}

		void SetFocus() override {
			assert(!isFocus);
			isFocus = true;
			ApplyCfg();
			gEngine->mouseEventHandler = xx::WeakFromThis(this);
			onFocus();
			//xx::CoutN("SetFocus");
		}

		void LostFocus() override {
			assert(isFocus);
			isFocus = false;
			ApplyCfg();
			gEngine->mouseEventHandler.Reset();
			//xx::CoutN("LostFocus");
		}

		// todo: enable disable

		void OnMouseDown() override {
			if (MousePosInArea()) {
				auto mx = gEngine->mouse.pos.x;
				auto x1 = worldMinXY.x + widthTxtLeft * worldScale.x;
				auto x2 = worldMinXY.x + (widthTxtLeft + widthBar) * worldScale.x;
				assert(worldMaxXY.x > x1);
				assert(worldMaxXY.x > x2);
				if (mx <= x1) {
					value = 0;
				}
				else if (mx >= x2) {
					value = 1;
				}
				else {
					value = (mx - x1) / (x2 - x1);
				}
				ApplyValue();
				blockMoving = true;
			}
		}

		void OnMouseMove() override {
			if (MousePosInArea()) {
				if (!isFocus) {
					SetFocus();
				}
				if (blockMoving) {
					OnMouseDown();
				}
			}
			else {
				if (!isFocus) return;
				LostFocus();
				if (blockMoving) {
					blockMoving = false;
					if (valueBak != value) {
						onChanged(value);
						valueBak = value;
					}
				}
				gEngine->mouseEventHandler.Reset();
			}
		}

		void OnMouseUp() override {
			blockMoving = false;
			if (valueBak != value) {
				onChanged(value);
				valueBak = value;
			}
		};

	};
}
