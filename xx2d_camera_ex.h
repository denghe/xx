#pragma once

namespace xx {

	struct CameraEx {
		static constexpr FromTo<float> cSyncSpeedPerFrameRange{ 100.f / 120.f, 10000.f / 120.f };
		static constexpr float cSyncSpeedPerFrameInc{ 1000.f * Cfg::frameDelay };
		float syncSpeedPerFrame{ cSyncSpeedPerFrameRange.from };
		float scale{ 1 };
		XY mapSize{};
		XY original{};
		XY newOriginal{};

		template<bool isNew = true>
		void SetOriginal(XY const& p1, XY const& p2) {
			FromTo<XY> aabb;
			if (p1.x > p2.x) {
				aabb.from.x = p2.x;
				aabb.to.x = p1.x;
			}
			else {
				aabb.from.x = p1.x;
				aabb.to.x = p2.x;
			}
			if (p1.y > p2.y) {
				aabb.from.y = p2.y;
				aabb.to.y = p1.y;
			}
			else {
				aabb.from.y = p1.y;
				aabb.to.y = p2.y;
			}
			auto midPos = aabb.from + (aabb.to - aabb.from) * 0.5f;
			// todo: safe area limit for p1
			
			if constexpr (isNew) {
				newOriginal = midPos;
			}
			else {
				original = midPos;
			}
		}

		void Update() {
			// smooth sync
			if (original != newOriginal) {
				auto d = newOriginal - original;
				auto dd = d.x * d.x + d.y * d.y;
				if (dd < syncSpeedPerFrame * syncSpeedPerFrame) {
					original = newOriginal;
					syncSpeedPerFrame = cSyncSpeedPerFrameRange.from;
				}
				else {
					auto mag = std::sqrtf(dd);
					auto norm = d / mag;
					original += norm * syncSpeedPerFrame;
					if (syncSpeedPerFrame < cSyncSpeedPerFrameRange.to) {
						syncSpeedPerFrame += cSyncSpeedPerFrameInc;
					}
				}
			}
		}

		XX_INLINE CameraEx& IncreaseScale(float v, float maxScale) {
			scale += v;
			if (scale > maxScale) scale = maxScale;
			return *this;
		}

		XX_INLINE CameraEx& DecreaseScale(float v, float minScale) {
			scale -= v;
			if (scale < minScale) scale = minScale;
			return *this;
		}

		// for Draw
		XX_INLINE XY ToGLPos(XY const& logicPos) const {
			return (logicPos - original).FlipY() * scale;
		}
		XX_INLINE XY ToGLPos(float x, float y) const {
			return ToGLPos({ x, y });
		}

		// for mouse pos to logic
		XX_INLINE XY ToLogicPos(XY const& glPos) const {
			return (glPos / scale).FlipY() + original;
		}

	};

}
