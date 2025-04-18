﻿#pragma once
#include <xx2d_base1.h>

namespace xx {
	
	struct Scale9 {
		Ref<Frame> frame;
		UVRect center{};
		XY centerAnchor = { 0, 1 };
		XY centerScale{ 1, 1 };
		XY centerOffset{};
		XY texScale{ 1, 1 };
		XY size{};
		XY pos{};
		XY anchor{};
		XY scale{ 1, 1 };
		float radians{};		// clockwise
		float colorplus{ 1 };
		RGBA8 color{ 255, 255, 255, 255 };

		// return center pos
		template<bool drawCenter = true, bool enableRotation = true>
		void Draw() {
			auto qs = EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderQuadInstance).Draw(frame->tex->GetValue(), 9);

			auto r = frame->textureRect;

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

			auto ss = size * scale;
			auto ts = texScale * scale;
			auto co = centerOffset * scale;

			float sx = float(ss.x - tw1 * ts.x - tw3 * ts.x) / tw2;
			float sy = float(ss.y - th1 * ts.y - th3 * ts.y) / th2;
#ifndef NDEBUG
			if (sx < 0 || sy < 0) {
				CoutN(" sx = ", sx, " sy = ", sy, " ts = ", ts);
				xx_assert(false);
			}
#endif
			float px1 = 0;
			float px2 = tw1 * ts.x;
			float px3 = ss.x - tw3 * ts.x;

			float py1 = ss.y;
			float py2 = ss.y - (th1 * ts.y);
			float py3 = ss.y - (ss.y - th3 * ts.y);

			auto offset = pos;

			AffineTransform at;
			if constexpr (enableRotation) {
				at = AffineTransform::MakePosScaleRadiansAnchorSize({}, 1, radians, anchor * ss);
			}
			else {
				radians = 0;
				offset -= anchor * ss;
			}

			{
				auto& q = qs[0];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px1, py1 });
				}
				else {
					q.pos = offset + XY{ px1, py1 };
				}
				q.anchor = { 0, 1 };
				q.scale = ts;
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx1, ty1, tw1, th1 };
			}
			{
				auto& q = qs[1];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px2, py1 });
				}
				else {
					q.pos = offset + XY{ px2, py1 };
				}
				q.anchor = { 0, 1 };
				q.scale = { sx, ts.y };
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx2, ty1, tw2, th1 };
			}
			{
				auto& q = qs[2];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px3, py1 });
				}
				else {
					q.pos = offset + XY{ px3, py1 };
				}
				q.anchor = { 0, 1 };
				q.scale = ts;
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx3, ty1, tw3, th1 };
			}
			{
				auto& q = qs[3];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px1, py2 });
				}
				else {
					q.pos = offset + XY{ px1, py2 };
				}
				q.anchor = { 0, 1 };
				q.scale = { ts.x, sy };
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx1, ty2, tw1, th2 };
			}
			{
				auto& q = qs[4];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px2 + co.x + tw2 * centerScale.x * centerAnchor.x, py2 + co.y + th2 * centerScale.y * (1 - centerAnchor.y) });
				}
				else {
					q.pos = offset + XY{ px2 + co.x + tw2 * centerScale.x * centerAnchor.x, py2 + co.y + th2 * centerScale.y * (1 - centerAnchor.y) };
				}
				q.anchor = centerAnchor;
				if (drawCenter) {
					q.scale = { sx * centerScale.x, sy * centerScale.y };
				}
				else {
					q.scale = {};
				}
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx2, ty2, tw2, th2 };
			}
			{
				auto& q = qs[5];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px3, py2 });
				}
				else {
					q.pos = offset + XY{ px3, py2 };
				}
				q.anchor = { 0, 1 };
				q.scale = { ts.x, sy };
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx3, ty2, tw3, th2 };
			}
			{
				auto& q = qs[6];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px1, py3 });
				}
				else {
					q.pos = offset + XY{ px1, py3 };
				}
				q.anchor = { 0, 1 };
				q.scale = ts;
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx1, ty3, tw1, th3 };
			}
			{
				auto& q = qs[7];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px2, py3 });
				}
				else {
					q.pos = offset + XY{ px2, py3 };
				}
				q.anchor = { 0, 1 };
				q.scale = { sx, ts.y };
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx2, ty3, tw2, th3 };
			}
			{
				auto& q = qs[8];
				if constexpr (enableRotation) {
					q.pos = offset + at({ px3, py3 });
				}
				else {
					q.pos = offset + XY{ px3, py3 };
				}
				q.anchor = { 0, 1 };
				q.scale = ts;
				q.radians = radians;
				q.colorplus = colorplus;
				q.color = color;
				q.texRect = { tx3, ty3, tw3, th3 };
			}
		}
	};

}
