﻿#pragma once
#include "xx_space_.h"

namespace xx {

	// todo: refine

	// space grid index system for AABB bounding box. coordinate (0, 0) at left-top, +x = right, +y = buttom
	template<typename Item, typename XY_t = XYi>
	struct SpaceGridAB2;

	template<typename T>
	struct SpaceGridAB2ItemCellInfo {
		T* self{};
		size_t idx{};
		SpaceGridAB2ItemCellInfo* prev{}, * next{};
	};

	// for inherit
	template<typename Derived, typename XY_t = XYi>
	struct SpaceGridAB2Item {
		using SGABCoveredCellInfo = SpaceGridAB2ItemCellInfo<Derived>;
		SpaceGridAB2<Derived, XY_t>* _sgab{};
		XY_t _sgabPos, _sgabRadius, _sgabMin, _sgabMax;	// for Add & Update calc covered cells
		XYi _sgabCRIdxFrom, _sgabCRIdxTo;	// backup for Update speed up
		std::vector<SGABCoveredCellInfo> _sgabCoveredCellInfos;	// todo: change to custom single buf container ?
		size_t _sgabFlag{};	// avoid duplication when Foreach

		XX_INLINE void SGABSetPosSiz(XY_t const& pos, XY_t const& siz) {
			_sgabPos = pos;
			_sgabRadius = siz / 2;
			_sgabMin = pos - _sgabRadius;
			_sgabMax = pos + _sgabRadius;
		}

		XX_INLINE void SGABAdd(SpaceGridAB2<Derived, XY_t>& sgab, XY_t const& pos, XY_t const& siz) {
			assert(!_sgab);
			assert(!_sgabFlag);
			assert(_sgabCoveredCellInfos.empty());
			_sgab = &sgab;
			SGABSetPosSiz(pos, siz);
			_sgab->Add(((Derived*)(this)));
		}

		XX_INLINE void SGABUpdate(XY_t const& pos, XY_t const& siz) {
			assert(_sgab);
			SGABSetPosSiz(pos, siz);
			_sgab->Update(((Derived*)(this)));
		}

		XX_INLINE void SGABAddOrUpdate(SpaceGridAB2<Derived, XY_t>& sgab, XY_t const& pos, XY_t const& siz) {
			if (_sgab) {
				assert(_sgab == &sgab);
				SGABUpdate(pos, siz);
			} else {
				SGABAdd(sgab, pos, siz);
			}
		}

		XX_INLINE void SGABRemove() {
			if (_sgab) {
				_sgab->Remove(((Derived*)(this)));
				_sgab = {};
			}
		}
	};

	template<typename Item, typename XY_t>
	struct SpaceGridAB2 {
		using ItemCellInfo = SpaceGridAB2ItemCellInfo<Item>;
		using VT = XY_t::ElementType;
		XYi cellSize;
		XY_t max, max_2;
		int32_t numRows{}, numCols{};
		int32_t numItems{}, numActives{};	// for easy check & stat
		std::vector<ItemCellInfo*> cells;
		std::vector<Item*> results;	// tmp store Foreach items

		void Init(int32_t numRows_, int32_t numCols_, int32_t cellWidth_, int32_t cellHeight_) {
			assert(cells.empty());
			assert(!numItems);
			assert(!numActives);

			numRows = numRows_;
			numCols = numCols_;
			cellSize.x = cellWidth_;
			cellSize.y = cellHeight_;

			max = { VT(cellWidth_ * numCols), VT(cellHeight_ * numRows) };
			max_2 = max / 2;

			cells.resize(numRows * numCols);
		}

		void Add(Item* c) {
			assert(c);
			assert(c->_sgab == this);
			assert(c->_sgabCoveredCellInfos.empty());
			assert(c->_sgabMin.x < c->_sgabMax.x);
			assert(c->_sgabMin.y < c->_sgabMax.y);
			assert(c->_sgabMin.x >= 0 && c->_sgabMin.y >= 0);
			assert(c->_sgabMax.x < c->_sgab->max.x && c->_sgabMax.y < c->_sgab->max.y);

			// calc covered cells
			auto crIdxFrom = c->_sgabMin.template As<int32_t>() / cellSize;
			auto crIdxTo = c->_sgabMax.template As<int32_t>() / cellSize;
			auto numMaxCoveredCells = (crIdxTo.x - crIdxFrom.x + 2) * (crIdxTo.y - crIdxFrom.y + 2);

			// link
			auto& ccis = c->_sgabCoveredCellInfos;
			ccis.reserve(numMaxCoveredCells);
			for (auto rIdx = crIdxFrom.y; rIdx <= crIdxTo.y; rIdx++) {
				for (auto cIdx = crIdxFrom.x; cIdx <= crIdxTo.x; cIdx++) {
					size_t idx = rIdx * numCols + cIdx;
					assert(idx <= cells.size());
					auto ci = &ccis.emplace_back(ItemCellInfo{ c, idx, nullptr, cells[idx] });
					if (cells[idx]) {
						cells[idx]->prev = ci;
					}
					cells[idx] = ci;
				}
			}

			// store idxs
			c->_sgabCRIdxFrom = crIdxFrom;
			c->_sgabCRIdxTo = crIdxTo;

			// stat
			++numItems;
		}

		void Remove(Item* c) {
			assert(c);
			assert(c->_sgab == this);
			assert(c->_sgabCoveredCellInfos.size());

			// unlink
			auto& ccis = c->_sgabCoveredCellInfos;
			for (auto& ci : ccis) {

				if (ci.prev) {	// isn't header
					ci.prev->next = ci.next;
					if (ci.next) {
						ci.next->prev = ci.prev;
					}
				} else {
					cells[ci.idx] = ci.next;
					if (ci.next) {
						ci.next->prev = {};
					}
				}
			}
			ccis.clear();

			// stat
			--numItems;
		}

		void Update(Item* c) {
			assert(c);
			assert(c->_sgab == this);
			assert(c->_sgabCoveredCellInfos.size());
			assert(c->_sgabMin.x < c->_sgabMax.x);
			assert(c->_sgabMin.y < c->_sgabMax.y);
			assert(c->_sgabMin.x >= 0 && c->_sgabMin.y >= 0);
			assert(c->_sgabMax.x < c->_sgab->max.x && c->_sgabMax.y < c->_sgab->max.y);

			// calc covered cells
			auto crIdxFrom = c->_sgabMin.template As<int32_t>() / cellSize;
			auto crIdxTo = c->_sgabMax.template As<int32_t>() / cellSize;
			auto numCoveredCells = (crIdxTo.x - crIdxFrom.x + 1) * (crIdxTo.y - crIdxFrom.y + 1);

			auto& ccis = c->_sgabCoveredCellInfos;
			if (numCoveredCells == ccis.size()
				&& crIdxFrom == c->_sgabCRIdxFrom
				&& crIdxTo == c->_sgabCRIdxTo) {
				return;
			}

			// unlink
			for (auto& ci : ccis) {

				if (ci.prev) {	// isn't header
					ci.prev->next = ci.next;
					if (ci.next) {
						ci.next->prev = ci.prev;
					}
				} else {
					cells[ci.idx] = ci.next;
					if (ci.next) {
						ci.next->prev = {};
					}
				}
			}
			ccis.clear();

			// link
			ccis.reserve(numCoveredCells);
			for (auto rIdx = crIdxFrom.y; rIdx <= crIdxTo.y; rIdx++) {
				for (auto cIdx = crIdxFrom.x; cIdx <= crIdxTo.x; cIdx++) {
					size_t idx = rIdx * numCols + cIdx;
					assert(idx <= cells.size());
					auto ci = &ccis.emplace_back(ItemCellInfo{ c, idx, nullptr, cells[idx] });
					if (cells[idx]) {
						cells[idx]->prev = ci;
					}
					cells[idx] = ci;
				}
			}

			// store idxs
			c->_sgabCRIdxFrom = crIdxFrom;
			c->_sgabCRIdxTo = crIdxTo;
		}

		void ClearResults() {
			for (auto& o : results) {
				o->_sgabFlag = 0;
			}
			results.clear();
		}

		template<typename F>
		void ForeachPoint(XY_t const& p, F&& func) {
			auto crIdx = p.template As<int32_t>() / cellSize;
			if (crIdx.x < 0 || crIdx.x >= numCols
				|| crIdx.y < 0 || crIdx.y >= numRows) return;
			auto c = cells[crIdx.y * numCols + crIdx.x];
			while (c) {
				auto&& s = c->self;
				auto next = c->next;

				if (!(s->_sgabMax.x < p.x || p.x < s->_sgabMin.x || s->_sgabMax.y < p.y || p.y < s->_sgabMin.y)) {
					func(s);
				}
				c = next;
			}
		}

		// return true: success( aabb in area )
		XX_INLINE bool TryLimitAABB(FromTo<XY_t>& aabb) {
			if (aabb.from.x < 0) aabb.from.x = 0;
			if (aabb.from.y < 0) aabb.from.y = 0;
			if (aabb.to.x >= max.x) aabb.to.x = max.x - 1;
			if (aabb.to.y >= max.y) aabb.to.y = max.y - 1;
			return aabb.from.x < aabb.to.x && aabb.from.y < aabb.to.y;
		}

		// fill items to results. need ClearResults()
		// auto guard = xx::MakeSimpleScopeGuard([&] { sg.ClearResults(); });
		void ForeachAABB(XY_t const& minXY, XY_t const& maxXY, Item* except = nullptr) {
			assert(minXY.x < maxXY.x);
			assert(minXY.y < maxXY.y);
			assert(minXY.x >= 0 && minXY.y >= 0);
			assert(maxXY.x < max.x && maxXY.y < max.y);

			// calc covered cells
			auto crIdxFrom = minXY.template As<int32_t>() / cellSize;
			auto crIdxTo = maxXY.template As<int32_t>() / cellSize;

			// except set flag
			if (except) {
				except->_sgabFlag = 1;
			}

			if (crIdxFrom.x == crIdxTo.x || crIdxFrom.y == crIdxTo.y) {
				// 1-2 row, 1-2 col: do full rect check
				for (auto rIdx = crIdxFrom.y; rIdx <= crIdxTo.y; rIdx++) {
					for (auto cIdx = crIdxFrom.x; cIdx <= crIdxTo.x; cIdx++) {
						auto c = cells[rIdx * numCols + cIdx];
						while (c) {
							auto&& s = c->self;
							if (!(s->_sgabMax.x < minXY.x || maxXY.x < s->_sgabMin.x || s->_sgabMax.y < minXY.y || maxXY.y < s->_sgabMin.y)) {
								if (!s->_sgabFlag) {
									s->_sgabFlag = 1;
									results.push_back(s);
								}
							}
							c = c->next;
						}
					}
				}
			} else {
				// first row: check AB
				auto rIdx = crIdxFrom.y;

				// first cell: check top left AB
				auto cIdx = crIdxFrom.x;
				auto c = cells[rIdx * numCols + cIdx];
				while (c) {
					auto&& s = c->self;
					if (s->_sgabMax.x > minXY.x && s->_sgabMax.y > minXY.y) {
						if (!s->_sgabFlag) {
							s->_sgabFlag = 1;
							results.push_back(s);
						}
					}
					c = c->next;
				}

				// middle cells: check top AB
				for (++cIdx; cIdx < crIdxTo.x; cIdx++) {
					c = cells[rIdx * numCols + cIdx];
					while (c) {
						auto&& s = c->self;
						if (s->_sgabMax.y > minXY.y) {
							if (!s->_sgabFlag) {
								s->_sgabFlag = 1;
								results.push_back(s);
							}
						}
						c = c->next;
					}
				}

				// last cell: check top right AB
				if (cIdx == crIdxTo.x) {
					auto c = cells[rIdx * numCols + cIdx];
					while (c) {
						auto&& s = c->self;
						if (s->_sgabMin.x < maxXY.x && s->_sgabMax.y > minXY.y) {
							if (!s->_sgabFlag) {
								s->_sgabFlag = 1;
								results.push_back(s);
							}
						}
						c = c->next;
					}
				}

				// middle rows: first & last col check AB
				for (++rIdx; rIdx < crIdxTo.y; rIdx++) {

					// first cell: check left AB
					cIdx = crIdxFrom.x;
					c = cells[rIdx * numCols + cIdx];
					while (c) {
						auto&& s = c->self;
						if (s->_sgabMax.x > minXY.x) {
							if (!s->_sgabFlag) {
								s->_sgabFlag = 1;
								results.push_back(s);
							}
						}
						c = c->next;
					}

					// middle cols: no check
					for (; cIdx < crIdxTo.x; cIdx++) {
						c = cells[rIdx * numCols + cIdx];
						while (c) {
							auto&& s = c->self;
							if (!s->_sgabFlag) {
								s->_sgabFlag = 1;
								results.push_back(s);
							}
							c = c->next;
						}
					}

					// last cell: check right AB
					if (cIdx == crIdxTo.x) {
						auto c = cells[rIdx * numCols + cIdx];
						while (c) {
							auto&& s = c->self;
							if (s->_sgabMin.x < maxXY.x) {
								if (!s->_sgabFlag) {
									s->_sgabFlag = 1;
									results.push_back(s);
								}
							}
							c = c->next;
						}
					}
				}

				// last row: check AB
				if (rIdx == crIdxTo.y) {

					// first cell: check left bottom AB
					cIdx = crIdxFrom.x;
					c = cells[rIdx * numCols + cIdx];
					while (c) {
						auto&& s = c->self;
						if (s->_sgabMax.x > minXY.x && s->_sgabMin.y < maxXY.y) {
							if (!s->_sgabFlag) {
								s->_sgabFlag = 1;
								results.push_back(s);
							}
						}
						c = c->next;
					}

					// middle cells: check bottom AB
					for (++cIdx; cIdx < crIdxTo.x; cIdx++) {
						c = cells[rIdx * numCols + cIdx];
						while (c) {
							auto&& s = c->self;
							if (s->_sgabMin.y < maxXY.y) {
								if (!s->_sgabFlag) {
									s->_sgabFlag = 1;
									results.push_back(s);
								}
							}
							c = c->next;
						}
					}

					// last cell: check right bottom AB
					if (cIdx == crIdxTo.x) {
						auto c = cells[rIdx * numCols + cIdx];
						while (c) {
							auto&& s = c->self;
							if (s->_sgabMin.x < maxXY.x && s->_sgabMin.y < maxXY.y) {
								if (!s->_sgabFlag) {
									s->_sgabFlag = 1;
									results.push_back(s);
								}
							}
							c = c->next;
						}
					}
				}
			}

			// except clear flag
			if (except) {
				except->_sgabFlag = 0;
			}

		}
	};

}
