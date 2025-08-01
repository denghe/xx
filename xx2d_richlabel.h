﻿#pragma once
#include <xx2d_base2.h>

namespace xx {

	struct RichLabel : Node {

		enum class ItemTypes : uint8_t {
			Unknown, Space, Text, Picture	// ... more
		};

		struct ItemBase {
			ItemTypes type;
			VAligns align;
			float x, y, h;	// changed by  FillXH , FillY
		};

		struct Space : ItemBase {
			XY size;
			Space(VAligns align, XY const& size) {
				this->type = ItemTypes::Space;
				this->align = align;
				this->size = size;
			}
		};

		struct Text : ItemBase {
			TinyList<TinyFrame const*> chars;
			RGBA8 color;
			XY scale;
			Text(VAligns align, RGBA8 color, XY const& scale) {
				this->type = ItemTypes::Text;
				this->align = align;
				this->color = color;
				this->scale = scale;
			}
		};

		struct Picture : ItemBase {
			Ref<Quad> quad;
			Picture(VAligns align, Ref<Quad> quad) {
				this->type = ItemTypes::Picture;
				this->align = align;
				this->quad = std::move(quad);
			}
		};

		// ... more

		struct Item {
			alignas(MaxAlignof_v<Space, Text, Picture>) char data[MaxSizeof_v<Space, Text, Picture>];

			Item(Item const&) = delete;
			Item& operator=(Item const&) = delete;
			Item& operator=(Item&& o) noexcept = delete;
			Item(Item&& o) noexcept {
				memcpy(&data, &o.data, sizeof(data));
				o.Type() = ItemTypes::Unknown;
			}
			Item() {
				((ItemBase&)data).type = ItemTypes::Unknown;
			}

			template<std::derived_from<ItemBase> T, typename ... Args>
			T& Emplace(Args&& ...args) {
				assert(((ItemBase&)data).type == ItemTypes::Unknown);
				return *new (&data) T(std::forward<Args>(args)...);
			}

			template<std::derived_from<ItemBase> T>
			T& As() {
				auto& ib = (ItemBase&)data;
				if constexpr (std::same_as<T, Space>) assert(ib.type == ItemTypes::Space);
				if constexpr (std::same_as<T, Text>) assert(ib.type == ItemTypes::Text);
				if constexpr (std::same_as<T, Picture>) assert(ib.type == ItemTypes::Picture);
				return (T&)ib;
			}

			ItemTypes const& Type() const {
				return ((ItemBase&)data).type;
			}
			ItemTypes& Type() {
				return ((ItemBase&)data).type;
			}

			~Item() {
				auto& ib = (ItemBase&)data;
				switch (ib.type) {
				case ItemTypes::Unknown:
					break;
				case ItemTypes::Space:
					((Space&)ib).~Space();
					break;
				case ItemTypes::Text:
					((Text&)ib).~Text();
					break;
				case ItemTypes::Picture:
					((Picture&)ib).~Picture();
					break;
				default: xx_assert(false);
				}
			}
		};

		Listi32<Item> items;
		Listi32<Picture*> pics;				// for batch draw
		float width{}, y{}, lineX{}, lineHeight{};
		int32_t lineItemsCount{};
		HAligns halign{};	// when line end, horizontal align current line

		RichLabel& Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, float width_, int capacity_ = 128) {
			assert(width_ > 0);
			Node::Init(z_, position_, scale_, anchor_, { width_, 0 });
			width = width_;
			y = {};

			LineInit();
			items.Clear();
			items.Reserve(capacity_);
			return *this;
		}

	protected:

		template<std::derived_from<ItemBase> T, typename ... Args>
		T& AddItem(Args&& ...args) {
			++lineItemsCount;
			return items.Emplace().Emplace<T>(std::forward<Args>(args)...);
		}

		void FillXH(ItemBase& o, float h) {
			o.x = lineX;
			o.h = h;
			if (lineHeight < h) {
				lineHeight = h;
			}
		}

		Text& MakeText(VAligns align, RGBA8 color, XY const& scale) {
			auto& o = AddItem<Text>(align, color, scale);
			FillXH(o, (float)EngineBase2::Instance().ctcDefault.canvasHeight * scale.y);
			return o;
		}

		Text& GetLastText(VAligns align, RGBA8 color, XY const& scale) {
			if (items.Empty() || items.Back().Type() != ItemTypes::Text) return MakeText(align, color, scale);
			if (auto& t = items.Back().As<Text>(); t.align == align && t.color == color && t.scale == scale) return t;
			return MakeText(align, color, scale);
		}

		void FillY() {
			y -= lineHeight;
			for (int e = items.len, i = e - lineItemsCount; i < e; ++i) {
				auto& o = items[i].As<ItemBase>();
				float leftHeight = lineHeight - o.h;
				assert(leftHeight >= 0);
				// todo: float valign support ?
				if (o.align == VAligns::Bottom || std::abs(leftHeight) < std::numeric_limits<float>::epsilon()) {
					o.y = y;
				} else if (o.align == VAligns::Top) {
					o.y = y + leftHeight;
				} else {	// o.align == VAligns::Center
					o.y = y + leftHeight / 2;
				}
			}
			if (halign != HAligns::Left) {
				float left = width - lineX;
				assert(left >= 0);
				float x = halign == HAligns::Right ? left : left / 2;
				for (int e = items.len, i = e - lineItemsCount; i < e; ++i) {
					auto& o = items[i].As<ItemBase>();
					o.x += x;
				}
			}
		}

		XX_INLINE void LineInit() {
			lineX = {};
			lineHeight = {};
			lineItemsCount = {};
		}

		XX_INLINE void NewLine() {
			FillY();
			LineInit();
		}

	public:

		template<typename S>
		RichLabel& AddText(S const& text, XY const& scale_ = { 1, 1 }, RGBA8 color = RGBA8_White, VAligns align = VAligns::Center) {
			auto& ctc = EngineBase2::Instance().ctcDefault;
			Text* t{};
			TinyFrame* charFrame{};
			float charWidth, leftWidth{};
			for (auto& c : text) {
				if (c == U'\r') continue;
				else if (c == U'\n') {
					NewLine();
				} else {
					charFrame = &ctc.Find(c);
					charWidth = charFrame->texRect.w * scale_.x;
					leftWidth = width - lineX;
					if (leftWidth >= charWidth) {
						t = &GetLastText(align, color, scale_);
					} else {
						NewLine();
						t = &MakeText(align, color, scale_);
					}
					t->chars.Add(charFrame);
					lineX += charWidth;
				}
			}
			return *this;
		}

		// SetOffset( width - txtWidth ) + AddText( txt )
		// txt can't include /r/n
		template<typename S>
		RichLabel& AddRightText(S const& text, XY const& scale_ = { 1, 1 }, RGBA8 color = RGBA8_White, VAligns align = VAligns::Center) {
			auto& ctc = EngineBase2::Instance().ctcDefault;
			float txtWidth{};
			for (auto& c : text) {
				txtWidth += ctc.Find(c).texRect.w * scale_.x;
			}
			assert(width >= txtWidth);
			SetOffset(width - txtWidth);
			AddText(text, scale_, color, align);
			lineX = width;
			return *this;
		}

		// AddText( txt, auto scale x )
		// txt can't include /r/n
		template<typename S>
		RichLabel& AddLimitedWidthText(S const& text, float txtWidth, XY const& scale_ = { 1, 1 }, RGBA8 color = RGBA8_White, VAligns align = VAligns::Center) {
			auto& ctc = EngineBase2::Instance().ctcDefault;
			float tw{};
			for (auto& c : text) {
				tw += ctc.Find(c).texRect.w * scale_.x;
			}
			if (tw <= txtWidth) {
				AddText(text, scale_, color, align);
			}
			else {
				AddText(text, { scale_.x / tw * txtWidth, scale_.y }, color, align);
			}
			return *this;
		}



		RichLabel& AddPicture(Ref<Quad> quad, VAligns align = VAligns::Center) {
			assert(quad);
			quad->anchor = {};
			auto size = quad->Size() * quad->scale;
			auto leftWidth = width - lineX;
			if (leftWidth < size.x) {
				NewLine();
			}
			auto& o = AddItem<Picture>(align, std::move(quad));
			FillXH(o, size.y);
			lineX += size.x;
			return *this;
		}

		RichLabel& AddPicture(Ref<Frame> frame, XY const& scale = { 1,1 }, VAligns align = VAligns::Center) {
			auto quad = MakeRef<Quad>();
			quad->SetFrame(std::move(frame)).SetScale(scale);
			return AddPicture(std::move(quad), align);
		}

		RichLabel& AddSpace(XY const& size, VAligns align = VAligns::Center) {
			auto leftWidth = width - lineX;
			if (leftWidth < size.x) {
				NewLine();
			}
			auto& o = AddItem<Space>(align, size);
			FillXH(o, size.y);
			lineX += size.x;
			return *this;
		}

		RichLabel& AddSpace(float width) {
			return AddSpace({ width,0 });
		}

		RichLabel& SetOffset(float x) {
			//assert(x > lineX);
			return AddSpace({ x - lineX, 0 });
		}

		RichLabel& SetHAlign(HAligns a = HAligns::Left) {
			halign = a;
			return *this;
		}



		// ... more

		RichLabel& Commit() {
			FillY();
			size = { width, -y };
			FillTrans();
			return *this;
		}

		virtual void Draw() override {
			auto& shader = EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderQuadInstance);
			XY basePos{ worldMinXY.x, worldMinXY.y + worldSize.y };
			pics.Clear();
			for (auto& o : items) {
				switch (o.Type()) {
				case ItemTypes::Space:
				{
					auto& t = o.As<Space>();
					// todo
					break;
				}
				case ItemTypes::Text:
				{
					auto& t = o.As<Text>();
					auto pos = basePos + XY{ t.x * worldScale.x, t.y * worldScale.y };
					for(auto e = t.chars.Len(), i = 0; i < e; ++i) {
						auto f = t.chars[i];
						auto& q = *shader.Draw(f->tex->GetValue(), 1);
						q.anchor = { 0.f, 0.f };
						q.color = t.color;
						q.colorplus = 1;
						q.pos = pos;
						q.radians = {};
						q.scale = t.scale * worldScale;
						q.texRect.data = f->texRect.data;
						pos.x += f->texRect.w * t.scale.x * worldScale.x;
					}
					break;
				}
				case ItemTypes::Picture:
				{
					auto& t = o.As<Picture>();
					pics.Add(&t);
					break;
				}
				default: xx_assert(false);
				}
			}
			for (auto& p : pics) {
				auto& t = *p;
				auto pos = basePos + XY{ t.x * worldScale.x, t.y * worldScale.y };
				t.quad->SetPosition(pos).SetScale(p->quad->scale * worldScale).Draw();
			}
		}
	};

}
