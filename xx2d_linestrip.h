﻿#pragma once
#include <xx2d_base1.h>

namespace xx {

	struct LineStrip {
		std::vector<XYRGBA8> pointsBuf;
		bool dirty = true;

		std::vector<XY> points;
		XY size{};
		XY pos{};
		XY anchor{ 0.5, 0.5 };
		XY scale{ 1, 1 };
		float radians{};
		RGBA8 color{ 255, 255, 255, 255 };

		std::vector<XY>& SetPoints() {
			dirty = true;
			return points;
		}
		LineStrip& SetPoints(std::initializer_list<XY> ps) {
			dirty = true;
			points = ps;
			return *this;
		}
		template<bool loop = false, typename A>
		LineStrip& SetPointsArray(A const& a) {
			dirty = true;
			points.clear();
			points.insert(points.begin(), a.begin(), a.end());
			if constexpr (loop) {
				points.push_back(*a.begin());
			}
			return *this;
		}

		LineStrip& FillSectorPoints(XY const& center, float radius, float radians, float theta, int segments = 20, XY const& scale = { 1,1 }) {
			dirty = true;
			points.resize(segments + 3);
			points[0] = center;
			points[segments + 2] = center;
			auto coef = theta * 2 / segments;
			for (int i = 0; i <= segments; ++i) {
				auto rads = i * coef - theta;
				points[i + 1] = XY{ cosf(rads), sinf(rads) } * radius * scale + center;
			}
			return *this;
		}

		LineStrip& FillCirclePoints(XY const& center, float radius, std::optional<float> const& radians = {}, int segments = 20, XY const& scale = { 1,1 }) {
			dirty = true;
			points.reserve(segments + 2);
			points.resize(segments + 1);
			auto coef = 2.0f * (float)M_PI / segments;
			auto a = radians.has_value() ? *radians : 0;
			for (int i = 0; i <= segments; ++i) {
				auto rads = i * coef + a;
				points[i].x = radius * cosf(rads) * scale.x + center.x;
				points[i].y = radius * sinf(rads) * scale.y + center.y;
			}
			if (radians.has_value()) {
				points.push_back(center);
			}
			return *this;
		}

		LineStrip& FillBoxPoints(XY const& center, XY const& wh) {
			auto hwh = wh / 2;
			points.resize(5);
			points[0] = { center.x - hwh.x, center.y - hwh.y };
			points[1] = { center.x - hwh.x, center.y + hwh.y };
			points[2] = { center.x + hwh.x, center.y + hwh.y };
			points[3] = { center.x + hwh.x, center.y - hwh.y };
			points[4] = { center.x - hwh.x, center.y - hwh.y };
			return *this;
		}

		LineStrip& FillRectPoints(XY const& pos, XY const& wh) {
			points.resize(5);
			points[0] = { pos.x, pos.y };
			points[1] = { pos.x + wh.x, pos.y };
			points[2] = { pos.x + wh.x, pos.y + wh.y };
			points[3] = { pos.x, pos.y + wh.y };
			points[4] = { pos.x, pos.y };
			return *this;
		}

		// ... more?

		LineStrip& Clear() {
			dirty = true;
			points.clear();
			return *this;
		}

		LineStrip& SetSize(XY const& s) {
			dirty = true;
			size = s;
			return *this;
		}

		LineStrip& SetAnchor(XY const& a) {
			dirty = true;
			anchor = a;
			return *this;
		}

		LineStrip& SetRotate(float r) {
			dirty = true;
			radians = r;
			return *this;
		}

		LineStrip& SetScale(XY const& s) {
			dirty = true;
			scale = s;
			return *this;
		}
		LineStrip& SetScale(float s) {
			dirty = true;
			scale = { s, s };
			return *this;
		}

		LineStrip& SetPosition(XY const& p) {
			dirty = true;
			pos = p;
			return *this;
		}

		LineStrip& SetColor(RGBA8 c) {
			dirty = true;
			color = c;
			return *this;
		}
		LineStrip& SetColorAf(float a) {
			dirty = true;
			color.a = uint8_t(255 * a);
			return *this;
		}
		LineStrip& SetColorA(uint8_t a) {
			dirty = true;
			color.a = a;
			return *this;
		}

		void Update() {
			if (dirty) {
				auto&& ps = points.size();
				pointsBuf.resize(ps);
				auto trans = AffineTransform::MakePosScaleRadiansAnchorSize(pos, scale, radians, size * anchor);
				for (size_t i = 0; i < ps; ++i) {
					(XY&)pointsBuf[i].x = trans(points[i]);
					memcpy(&pointsBuf[i].r, &color, sizeof(color));
				}
				dirty = false;
			}
		}

		LineStrip& Draw() {
			Update();
			if (int ps = (int)pointsBuf.size()) {
				EngineBase1::Instance().ShaderBegin(EngineBase1::Instance().shaderLineStrip).Draw(pointsBuf.data(), ps);
			}
			return *this;
		}
	};

}
