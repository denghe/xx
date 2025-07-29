#pragma once
#include <xx2d_node.h>

namespace xx {

	struct Scale9SpriteConfig {
		Ref<Frame> frame;
		XY texScale{ 1, 1 };
		UVRect center{};
		RGBA8 color{ xx::RGBA8{ 0x5f, 0x15, 0xd9, 0xff } };
		float borderScale{ 1.f };
		RGBA8 txtColor{ xx::RGBA8_White };
		XY txtPadding{ 20, 5 };
		XY txtPaddingRightBottom{ 20, 10 };
		float txtScale{ 1 };
		float iconPadding{ 5 };

		XX_INLINE XY GetCornerSize() const {
			return { float(frame->textureRect.w - center.w) * borderScale, float(frame->textureRect.h - center.h) * borderScale };
		}

		XX_INLINE XY GetCornerScaledSize() const {
			return GetCornerSize() * texScale;
		}
	};

	struct Scale9Sprite : Node {
		Ref<Frame> frame;
		UVRect center;
		RGBA8 color;
		XY texScale;
		float colorplus;

		Scale9Sprite& Init(int z_, XY const& position_, XY const& scale_, XY const& anchor_, XY const& size_, Scale9SpriteConfig const& cfg_, float colorplus_ = 1) {
			Node::Init(z_, position_, scale_, anchor_, size_);
			texScale = cfg_.texScale;
			frame = cfg_.frame;
			center = cfg_.center;
			color = cfg_.color;
			colorplus = colorplus_;
			return *this;
		}

		Scale9Sprite& Init(int z_, XY position_, XY anchor_, XY size_, Scale9SpriteConfig const& cfg_) {
			return Init(z_, position_, cfg_.borderScale, anchor_, size_ / cfg_.borderScale, cfg_);
		}

		virtual void Draw() override {
			auto qs = EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderQuadInstance).Draw(frame->tex->GetValue(), 9);

			auto& r = frame->textureRect;

			uint16_t tx1 = r.x + 0;
			uint16_t tx2 = r.x + center.x;
			uint16_t tx3 = r.x + center.x + center.w;

			uint16_t ty1 = r.y + 0;
			uint16_t ty2 = r.y + center.y;
			uint16_t ty3 = r.y + center.y + center.h;

			uint16_t tw1 = center.x;
			uint16_t tw2 = center.w;
			uint16_t tw3 = r.w - (center.x + center.w);

			uint16_t th1 = center.y;
			uint16_t th2 = center.h;
			uint16_t th3 = r.h - (center.y + center.h);

			XY ts{ texScale * worldScale };

			float sx = float(worldSize.x - tw1 * ts.x - tw3 * ts.x) / tw2;
			float sy = float(worldSize.y - th1 * ts.y - th3 * ts.y) / th2;
#ifndef NDEBUG
			if (sx < 0 || sy < 0) {
				CoutN(" sx = ", sx, " sy = ", sy, " ts = ", ts);
				xx_assert(false);
			}
#endif

			float px1 = 0;
			float px2 = tw1 * ts.x;
			float px3 = worldSize.x - tw3 * ts.x;

			float py1 = worldSize.y;
			float py2 = worldSize.y - (th1 * ts.y);
			float py3 = worldSize.y - (worldSize.y - th3 * ts.y);

			auto& basePos = worldMinXY;

			RGBA8 c = { color.r, color.g, color.b, (uint8_t)(color.a * alpha) };

			QuadInstanceData* q;
			q = &qs[0];
			q->pos = basePos + XY{ px1, py1 };
			q->anchor = { 0, 1 };
			q->scale = ts;
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx1, ty1, tw1, th1 };

			q = &qs[1];
			q->pos = basePos + XY{ px2, py1 };
			q->anchor = { 0, 1 };
			q->scale = { sx, ts.y };
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx2, ty1, tw2, th1 };

			q = &qs[2];
			q->pos = basePos + XY{ px3, py1 };
			q->anchor = { 0, 1 };
			q->scale = ts;
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx3, ty1, tw3, th1 };

			q = &qs[3];
			q->pos = basePos + XY{ px1, py2 };
			q->anchor = { 0, 1 };
			q->scale = { ts.x, sy };
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx1, ty2, tw1, th2 };

			q = &qs[4];
			q->pos = basePos + XY{ px2, py2 };
			q->anchor = { 0, 1 };
			q->scale = { sx, sy };
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx2, ty2, tw2, th2 };

			q = &qs[5];
			q->pos = basePos + XY{ px3, py2 };
			q->anchor = { 0, 1 };
			q->scale = { ts.x, sy };
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx3, ty2, tw3, th2 };

			q = &qs[6];
			q->pos = basePos + XY{ px1, py3 };
			q->anchor = { 0, 1 };
			q->scale = ts;
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx1, ty3, tw1, th3 };

			q = &qs[7];
			q->pos = basePos + XY{ px2, py3 };
			q->anchor = { 0, 1 };
			q->scale = { sx, ts.y };
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx2, ty3, tw2, th3 };

			q = &qs[8];
			q->pos = basePos + XY{ px3, py3 };
			q->anchor = { 0, 1 };
			q->scale = ts;
			q->radians = {};
			q->colorplus = colorplus;
			q->color = c;
			q->texRect = { tx3, ty3, tw3, th3 };
		}
	};

}
