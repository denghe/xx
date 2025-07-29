#pragma once
#include <xx2d_base0.h>

namespace xx {

	struct alignas(8) Node {
		Listi32<Shared<Node>> children;
		Weak<Node> parent;											// fill by MakeChildren
		Weak<Node> scissor;											// fill by scroll view MakeContent

        XX_INLINE SimpleAffineTransform& trans(){ return (SimpleAffineTransform&)worldScale; }
        XY worldScale, worldMinXY;
        XX_INLINE SimpleAffineTransform const& trans() const { return (SimpleAffineTransform&)worldScale; }

		XY position{}, scale{ 1, 1 }, anchor{ 0.5, 0.5 }, size{};
		XY worldMaxXY{}, worldSize{};								// boundingBox. world coordinate. fill by FillTrans()
		int z{};													// global z for event priority or batch combine
		bool inParentArea{ true };									// panel true ? combo box pop false ?
		float alpha{ 1 };
		int32_t indexAtParentChildren{-1};
		uint64_t ud{};

		XX_INLINE XY GetScaledSize() const {
			return scale * size;
		}

		// for init
		XX_INLINE void FillTrans() {
			if (parent) {
				trans() = SimpleAffineTransform::MakePosScaleAnchorSize(position, scale, anchor * size).MakeConcat(parent->trans());
			} else {
				trans().PosScaleAnchorSize(position, scale, anchor * size);
			}

			worldMaxXY = trans()(size);
			worldSize = worldMaxXY - trans().Offset();

			TransUpdate();
		}

		// for draw FillZNodes
		XX_INLINE bool IsVisible() const {
			if (scissor && !Calc::Intersects::BoxBox(worldMinXY, worldMaxXY, scissor->worldMinXY, scissor->worldMaxXY)) return false;
			if (inParentArea && parent) return Calc::Intersects::BoxBox(worldMinXY, worldMaxXY, parent->worldMinXY, parent->worldMaxXY);
			return Calc::Intersects::BoxBox(worldMinXY, worldMaxXY, gEngine->worldMinXY, gEngine->worldMaxXY);
		}

		XX_INLINE bool PosInArea(XY const& pos) const {
			if (scissor && !Calc::Intersects::BoxPoint(scissor->worldMinXY, scissor->worldMaxXY, pos)) return false;
			return Calc::Intersects::BoxPoint(worldMinXY, worldMaxXY, pos);
		}

		XX_INLINE bool PosInScissor(XY const& pos) const {
			if (!scissor) return true;
			return Calc::Intersects::BoxPoint(scissor->worldMinXY, scissor->worldMaxXY, pos);
		}

		// for draw bg
		XX_INLINE XY CalcBorderSize(XY const& padding = {}) const {
			return size * scale + padding;
		}

		// for update
		void FillTransRecursive() {
			FillTrans();
			for (auto& c : children) {
				c->FillTransRecursive();
			}
		};

		void SetAlphaRecursive(float alpha_) {
			alpha = alpha_;
			for (auto& c : children) {
				c->SetAlphaRecursive(alpha_);
			}
		}

		void SetScissorRecursive(Weak<Node> scissor_) {
			scissor = scissor_;
			for (auto& c : children) {
				c->SetScissorRecursive(scissor_);
			}
		}

		// for copy: Node::Init(z_, position_, scale_, anchor_, size_);
		XX_INLINE Node& Init(int z_ = 0, XY const& position_ = {}, XY const& scale_ = { 1,1 }, XY const& anchor_ = {}, XY const& size_ = {}) {
			z = z_;
			position = position_;
			scale = scale_;
			anchor = anchor_;
			size = size_;
			FillTrans();
			return *this;
		}

		XX_INLINE void Init_Root() {
			Init(0, -gEngine->windowSize_2, { 1,1 }, {}, gEngine->windowSize);
		}


		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Node, T>>>
		XX_INLINE Shared<T>& AddChildren(Shared<T> c) {
			assert(c);
			assert(!c->parent);
			c->parent = WeakFromThis(this);
			c->indexAtParentChildren = children.len;
			c->scissor = scissor;
			c->inParentArea = !size.IsZeroSimple();
			return (Shared<T>&)children.Emplace(std::move(c));
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<Node, T>>>
		XX_INLINE Shared<T>& MakeChildren() {
			return AddChildren(MakeShared<T>());
		}

		void SwapRemoveFromParent() {
			assert(parent);
			assert(parent->children[indexAtParentChildren].pointer == this);
			auto i = parent->children.Back()->indexAtParentChildren = indexAtParentChildren;
			indexAtParentChildren = -1;
			auto p = parent.GetPointer();
			parent.Reset();
			p->children.SwapRemoveAt(i);
		}

		XX_INLINE void Clear() {
			for (auto i = children.len - 1; i >= 0; --i) {
				children[i]->SwapRemoveFromParent();
			}
		}

		virtual void TransUpdate() {};
		virtual void Draw() {};									// draw current node only ( do not contain children )
		virtual ~Node() {};
	};

	/**********************************************************************************************/
	/**********************************************************************************************/

	struct ZNode {
		decltype(Node::z) z;
		Node* n;
		XX_INLINE Node* operator->() { return n; }
		XX_INLINE static bool LessThanComparer(ZNode const& a, ZNode const& b) {
			return a.z < b.z;
		}
		XX_INLINE static bool GreaterThanComparer(ZNode const& a, ZNode const& b) {
			return a.z > b.z;
		}
	};

	template<bool skipScissorContent = true>
	inline void FillZNodes(Listi32<ZNode>& zns, Node* n) {
		assert(n);
		if constexpr (skipScissorContent) {
			if (n->scissor && n->scissor == n->parent) return;
		}
		if ((n->size.x > 0.f || n->size.y > 0.f) && n->IsVisible()) {
			zns.Emplace(n->z, n);
		}
		for (auto& c : n->children) {
			FillZNodes(zns, c);
		}
	}

	inline void OrderByZDrawAndClear(Listi32<ZNode>& zns) {
		std::sort(zns.buf, zns.buf + zns.len, ZNode::LessThanComparer);	// draw small z first
		for (auto& zn : zns) {
			zn->Draw();
		}
		zns.Clear();
	}

	template<typename T>
	struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<Node, T>>> {
		static inline void Append(std::string& s, Node const& in) {
			::xx::Append(s, "{ trans = ", in.trans()
				, ", position = ", in.position
				, ", scale = ", in.scale
				, ", anchor = ", in.anchor
				, ", size = ", in.size
				, " }"
			);
		}
	};

}
