#pragma once
#include "xx_space_.h"
#include "xx_tinylist.h"

namespace xx {
	// space grid index system for AABB bounding box( int32_t version ). coordinate (0, 0) at left-top, +x = right, +y = buttom
	// tips: SpaceABi32<Item> xxx need put above the Item container code

	template<typename Item>
	struct SpaceABi32;

	template<typename T>
	struct SpaceABi32ItemCellInfo {
		T* self{};									// weak ref
		int32_t idx{};								// cell index
		SpaceABi32ItemCellInfo* prev{}, * next{};	// weak ref
	};

	// for inherit
	template<typename Derived>
	struct SpaceABi32Item {
		using SGABCoveredCellInfo = SpaceABi32ItemCellInfo<Derived>;
		SpaceABi32<Derived>* _sgc{};				// weak ref
		FromTo<XYi> _sgcIdx{};						// backup last calculate result
		xx::TinyList<SGABCoveredCellInfo> _sgcCoveredCellInfos;
		size_t _sgcFlag{};							// avoid duplication when Foreach
		FromTo<XYi> _aabb{};						// need fill / sync before Add / Update

		XX_INLINE void Fill_aabb(XYi const& pos, XYi siz) {
			assert(pos.x >= 0 && pos.y >= 0);
			siz.x >>= 1;
			siz.y >>= 1;
			assert(siz.x > 0 && siz.y > 0);
			_aabb.from = pos - siz;
			_aabb.to = pos + siz;
		}
	};

	template<typename Item>
	struct SpaceABi32 {
		using ItemCellInfo = SpaceABi32ItemCellInfo<Item>;
		using VT = XYi::ElementType;
		int32_t numRows{}, numCols{};
		XYi cellSize, max;

		struct Cell {
			ItemCellInfo* item;
			int32_t count;
		};
		int32_t cellsLen{};
		std::unique_ptr<Cell[]> cells;

		xx::Listi32<Item*> results;	// tmp store Foreach items

		// unsafe
		void Clear() {
			assert(cells);
			memset(cells.get(), 0, sizeof(Cell) * cellsLen);
		}

		void Init(int32_t numRows_, int32_t numCols_, XYi const& cellSize_) {
			assert(!cells);
			assert(numRows_ > 0 && numCols_ > 0 && cellSize_.x > 0 && cellSize_.y > 0);
			numRows = numRows_;
			numCols = numCols_;
			cellSize = cellSize_;
			max.x = cellSize_.x * numCols_;
			max.y = cellSize_.y * numRows_;

			cellsLen = numRows * numCols;
			cells = std::make_unique<Cell[]>(cellsLen);
			Clear();
		}

		void Add(Item* c) {
			assert(c);
			assert(!c->_sgc);
			assert(c->_sgcCoveredCellInfos.Empty());
			assert(c->_aabb.from.x < c->_aabb.to.x);
			assert(c->_aabb.from.y < c->_aabb.to.y);
			assert(c->_aabb.from.x >= 0 && c->_aabb.from.y >= 0);
			assert(c->_aabb.to.x < max.x && c->_aabb.to.y < max.y);
			c->_sgc = this;

			// calc covered cells
			auto crIdxFrom = c->_aabb.from / cellSize;
			auto crIdxTo = c->_aabb.to / cellSize;
			auto numMaxCoveredCells = (crIdxTo.x - crIdxFrom.x + 2) * (crIdxTo.y - crIdxFrom.y + 2);

			// store idxs
			c->_sgcIdx.from = crIdxFrom;
			c->_sgcIdx.to = crIdxTo;

			// link
			auto& ccis = c->_sgcCoveredCellInfos;
			ccis.Reserve(numMaxCoveredCells);
			for (auto rIdx = crIdxFrom.y; rIdx <= crIdxTo.y; rIdx++) {
				for (auto cIdx = crIdxFrom.x; cIdx <= crIdxTo.x; cIdx++) {
					auto idx = rIdx * numCols + cIdx;
					assert(idx <= cellsLen);
					auto ci = &ccis.Emplace(ItemCellInfo{ c, idx, nullptr, cells[idx].item });
					if (cells[idx].item) {
						cells[idx].item->prev = ci;
					}
					cells[idx].item = ci;
					++cells[idx].count;		// sync stat
				}
			}
		}

		void Remove(Item* c) {
			assert(c);
			assert(c->_sgc == this);
			assert(!c->_sgcCoveredCellInfos.Empty());

			// unlink
			auto& ccis = c->_sgcCoveredCellInfos;
			for (auto e = ccis.Len(), i = 0; i < e; ++i) {
				auto& ci = ccis[i];
				if (ci.prev) {	// isn't header
					ci.prev->next = ci.next;
					if (ci.next) {
						ci.next->prev = ci.prev;
					}
				} else {
					cells[ci.idx].item = ci.next;
					--cells[ci.idx].count;		// sync stat
					if (ci.next) {
						ci.next->prev = {};
					}
				}
			}

			// clear
			c->_sgc = {};
			assert(!c->_sgcFlag);
			c->_sgcCoveredCellInfos.Clear();
		}

		void Update(Item* c) {
			assert(c);
			assert(c->_sgc == this);
			assert(c->_sgcCoveredCellInfos.size());
			assert(c->_aabb.from.x < c->_aabb.to.x);
			assert(c->_aabb.from.y < c->_aabb.to.y);
			assert(c->_aabb.from.x >= 0 && c->_aabb.from.y >= 0);
			assert(c->_aabb.to.x < c->_sgc->max.x && c->_aabb.to.y < c->_sgc->max.y);

			// calc covered cells
			auto crIdxFrom = c->_aabb.from / cellSize;
			auto crIdxTo = c->_aabb.to / cellSize;
			auto numCoveredCells = (crIdxTo.x - crIdxFrom.x + 1) * (crIdxTo.y - crIdxFrom.y + 1);

			auto& ccis = c->_sgcCoveredCellInfos;
			auto e = ccis.Len();
			if (numCoveredCells == e
				&& crIdxFrom == c->_sgcIdx.from
				&& crIdxTo == c->_sgcIdx.to) {
				return;
			}

			// unlink
			for (auto i = 0; i < e; ++i) {
				auto& ci = ccis[i];
				if (ci.prev) {	// isn't header
					ci.prev->next = ci.next;
					if (ci.next) {
						ci.next->prev = ci.prev;
					}
				} else {
					cells[ci.idx].item = ci.next;
					--cells[ci.idx].count;		// sync stat
					if (ci.next) {
						ci.next->prev = {};
					}
				}
			}
			ccis.Clear();

			// link
			ccis.Reserve(numCoveredCells);
			for (auto rIdx = crIdxFrom.y; rIdx <= crIdxTo.y; rIdx++) {
				for (auto cIdx = crIdxFrom.x; cIdx <= crIdxTo.x; cIdx++) {
					size_t idx = rIdx * numCols + cIdx;
					assert(idx <= cellsLen);
					auto ci = &ccis.Emplace(ItemCellInfo{ c, idx, nullptr, cells[idx].item });
					if (cells[idx].item) {
						cells[idx].item->prev = ci;
					}
					cells[idx].item = ci;
					++cells[idx].count;		// sync stat
				}
			}

			// store idxs
			c->_sgcIdx.from = crIdxFrom;
			c->_sgcIdx.to = crIdxTo;
		}

		// 0 mean empty, 1 exists, 2 out of range
		int ExistsPoint(XYi const& p) {
			if (p.x < 0 || p.y < 0 || p.x >= max.x || p.y >= max.y) return 2;
			auto crIdx = p / cellSize;
			auto c = cells[crIdx.y * numCols + crIdx.x].item;
			while (c) {
				auto&& s = c->self;
				if (!(s->_aabb.to.x < p.x || p.x < s->_aabb.from.x || s->_aabb.to.y < p.y || p.y < s->_aabb.from.y)) return 1;
				c = c->next;
			}
			return 0;
		}

		void ClearResults() {
			for (auto& o : results) {
				o->_sgcFlag = 0;
			}
			results.Clear();
		}

		template<typename F>
		void ForeachPoint(XYi const& p, F&& func) {
			auto crIdx = p / cellSize;
			if (crIdx.x < 0 || crIdx.x >= numCols
				|| crIdx.y < 0 || crIdx.y >= numRows) return;
			auto c = cells[crIdx.y * numCols + crIdx.x].item;
			while (c) {
				auto&& s = c->self;
				auto next = c->next;

				if (!(s->_aabb.to.x < p.x || p.x < s->_aabb.from.x || s->_aabb.to.y < p.y || p.y < s->_aabb.from.y)) {
					func(s);
				}
				c = next;
			}
		}

		// return true: success( aabb in area )
		XX_INLINE bool TryLimitAABB(FromTo<XYi>& aabb) {
			if (aabb.from.x < 0) aabb.from.x = 0;
			if (aabb.from.y < 0) aabb.from.y = 0;
			if (aabb.to.x >= max.x) aabb.to.x = max.x - 1;
			if (aabb.to.y >= max.y) aabb.to.y = max.y - 1;
			return aabb.from.x < aabb.to.x && aabb.from.y < aabb.to.y;
		}

		// fill items to results. need ClearResults()
		// auto guard = xx::MakeSimpleScopeGuard([&] { sg.ClearResults(); });
		void ForeachAABB(FromTo<XYi>& aabb, Item* except = nullptr) {
			assert(aabb.from.x < aabb.to.x);
			assert(aabb.from.y < aabb.to.y);
			assert(aabb.from.x >= 0 && aabb.from.y >= 0);
			assert(aabb.to.x < max.x && aabb.to.y < max.y);

			// calc covered cells
			auto crIdxFrom = aabb.from / cellSize;
			auto crIdxTo = aabb.to / cellSize;

			// except set flag
			if (except) {
				except->_sgcFlag = 1;
			}

			if (crIdxFrom.x == crIdxTo.x || crIdxFrom.y == crIdxTo.y) {
				// 1-2 row, 1-2 col: do full rect check
				for (auto rIdx = crIdxFrom.y; rIdx <= crIdxTo.y; rIdx++) {
					for (auto cIdx = crIdxFrom.x; cIdx <= crIdxTo.x; cIdx++) {
						auto c = cells[rIdx * numCols + cIdx].item;
						while (c) {
							auto&& s = c->self;
							if (!(s->_aabb.to.x < aabb.from.x || aabb.to.x < s->_aabb.from.x || s->_aabb.to.y < aabb.from.y || aabb.to.y < s->_aabb.from.y)) {
								if (!s->_sgcFlag) {
									s->_sgcFlag = 1;
									results.Emplace(s);
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
				auto c = cells[rIdx * numCols + cIdx].item;
				while (c) {
					auto&& s = c->self;
					if (s->_aabb.to.x > aabb.from.x && s->_aabb.to.y > aabb.from.y) {
						if (!s->_sgcFlag) {
							s->_sgcFlag = 1;
							results.Emplace(s);
						}
					}
					c = c->next;
				}

				// middle cells: check top AB
				for (++cIdx; cIdx < crIdxTo.x; cIdx++) {
					c = cells[rIdx * numCols + cIdx].item;
					while (c) {
						auto&& s = c->self;
						if (s->_aabb.to.y > aabb.from.y) {
							if (!s->_sgcFlag) {
								s->_sgcFlag = 1;
								results.Emplace(s);
							}
						}
						c = c->next;
					}
				}

				// last cell: check top right AB
				if (cIdx == crIdxTo.x) {
					auto c = cells[rIdx * numCols + cIdx].item;
					while (c) {
						auto&& s = c->self;
						if (s->_aabb.from.x < aabb.to.x && s->_aabb.to.y > aabb.from.y) {
							if (!s->_sgcFlag) {
								s->_sgcFlag = 1;
								results.Emplace(s);
							}
						}
						c = c->next;
					}
				}

				// middle rows: first & last col check AB
				for (++rIdx; rIdx < crIdxTo.y; rIdx++) {

					// first cell: check left AB
					cIdx = crIdxFrom.x;
					c = cells[rIdx * numCols + cIdx].item;
					while (c) {
						auto&& s = c->self;
						if (s->_aabb.to.x > aabb.from.x) {
							if (!s->_sgcFlag) {
								s->_sgcFlag = 1;
								results.Emplace(s);
							}
						}
						c = c->next;
					}

					// middle cols: no check
					for (; cIdx < crIdxTo.x; cIdx++) {
						c = cells[rIdx * numCols + cIdx].item;
						while (c) {
							auto&& s = c->self;
							if (!s->_sgcFlag) {
								s->_sgcFlag = 1;
								results.Emplace(s);
							}
							c = c->next;
						}
					}

					// last cell: check right AB
					if (cIdx == crIdxTo.x) {
						auto c = cells[rIdx * numCols + cIdx].item;
						while (c) {
							auto&& s = c->self;
							if (s->_aabb.from.x < aabb.to.x) {
								if (!s->_sgcFlag) {
									s->_sgcFlag = 1;
									results.Emplace(s);
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
					c = cells[rIdx * numCols + cIdx].item;
					while (c) {
						auto&& s = c->self;
						if (s->_aabb.to.x > aabb.from.x && s->_aabb.from.y < aabb.to.y) {
							if (!s->_sgcFlag) {
								s->_sgcFlag = 1;
								results.Emplace(s);
							}
						}
						c = c->next;
					}

					// middle cells: check bottom AB
					for (++cIdx; cIdx < crIdxTo.x; cIdx++) {
						c = cells[rIdx * numCols + cIdx].item;
						while (c) {
							auto&& s = c->self;
							if (s->_aabb.from.y < aabb.to.y) {
								if (!s->_sgcFlag) {
									s->_sgcFlag = 1;
									results.Emplace(s);
								}
							}
							c = c->next;
						}
					}

					// last cell: check right bottom AB
					if (cIdx == crIdxTo.x) {
						auto c = cells[rIdx * numCols + cIdx].item;
						while (c) {
							auto&& s = c->self;
							if (s->_aabb.from.x < aabb.to.x && s->_aabb.from.y < aabb.to.y) {
								if (!s->_sgcFlag) {
									s->_sgcFlag = 1;
									results.Emplace(s);
								}
							}
							c = c->next;
						}
					}
				}
			}

			// except clear flag
			if (except) {
				except->_sgcFlag = 0;
			}

		}
	};

}
