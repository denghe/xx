#pragma once
#include <xx2d_base0.h>

namespace xx {

	struct MouseEventHandlerNode : Node, SpaceGridAB2Item<MouseEventHandlerNode, XY> {
		virtual void OnMouseDown() {};
		virtual void OnMouseMove() {};
		virtual void OnMouseUp() {};

		XX_FORCE_INLINE bool MousePosInArea() const {		// virtual ?
			return PosInArea(gEngine->mouse.pos);
		}

		virtual void TransUpdate() override {
			auto& hs = gEngine->mouseEventHandlers;
			auto pos = hs.max_2 + worldMinXY + worldSize / 2;
			if (Calc::Intersects::BoxPoint(XY{}, hs.max, pos)) {
				SGABAddOrUpdate(hs, pos, worldSize);
			} else {
				SGABRemove();
			}
		}

		~MouseEventHandlerNode() {
			SGABRemove();
		}
	};

}
