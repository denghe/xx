#pragma once
#include "xx_space_.h"

namespace xx {
	// space grid circle int contaner
	// tips: Spacei32<Item> xxx need put above the Item container code

	template<typename Item>
	struct Spacei32;

	// for inherit
	template<typename Derived>
	struct Spacei32Item {
		Spacei32<Derived>* _sgc{};          // weak ref
		Derived* _sgcPrev{}, * _sgcNext{};  // weak ref
		int32_t _sgcIdx{ -1 };              // index at Sapce.cells
		int32_t _x{}, _y{}, _radius{};		// need fill / sync before Add / Update
	};

	template<typename Item>
	struct Spacei32 {
		int32_t numRows{}, numCols{}, cellSize{};
		XYi max{};

		struct Cell {
			Item* item;
			int32_t count;
		};
		int32_t cellsLen{};
		std::unique_ptr<Cell[]> cells;

		// unsafe
		void Clear() {
			assert(cells);
			memset(cells.get(), 0, sizeof(Cell) * cellsLen);
		}

		void Init(int32_t numRows_, int32_t numCols_, int32_t cellSize_) {
			assert(!cells);
			assert(numRows_ > 0 && numCols_ > 0 && cellSize_ > 0);
			numRows = numRows_;
			numCols = numCols_;
			cellSize = cellSize_;
			max.x = cellSize_ * numCols_;
			max.y = cellSize_ * numRows_;

			cellsLen = numRows * numCols;
			cells = std::make_unique<Cell[]>(cellsLen);
			Clear();
		}

		void Add(Item* c) {
			assert(c);
			assert(!c->_sgc);
			assert(c->_sgcIdx == -1);
			assert(!c->_sgcPrev);
			assert(!c->_sgcNext);
			assert(c->_x >= 0 && c->_x < max.x);
			assert(c->_y >= 0 && c->_y < max.y);
			assert(c->_radius * 2 <= cellSize);
			c->_sgc = this;

			// calc rIdx & cIdx
			auto idx = PosToCIdx(c->_x, c->_y);
			assert(idx <= cellsLen);
			assert(!cells[idx].item || !cells[idx].item->_sgcPrev);

			// link
			if (cells[idx].item) {
				cells[idx].item->_sgcPrev = c;
			}
			c->_sgcNext = cells[idx].item;
			c->_sgcIdx = idx;
			cells[idx].item = c;
			assert(!cells[idx].item->_sgcPrev);
			assert(c->_sgcNext != c);
			assert(c->_sgcPrev != c);

			// stat
			++cells[idx].count;
		}

		void Remove(Item* c) {
			assert(c);
			assert(c->_sgc == this);
			assert(!c->_sgcPrev && cells[c->_sgcIdx].item == c || c->_sgcPrev->_sgcNext == c && cells[c->_sgcIdx].item != c);
			assert(!c->_sgcNext || c->_sgcNext->_sgcPrev == c);
			//assert(cells[c->_sgcIdx].item include c);

			// unlink
			if (c->_sgcPrev) {	// isn't header
				assert(cells[c->_sgcIdx].item != c);
				c->_sgcPrev->_sgcNext = c->_sgcNext;
				if (c->_sgcNext) {
					c->_sgcNext->_sgcPrev = c->_sgcPrev;
					c->_sgcNext = {};
				}
				c->_sgcPrev = {};
			} else {
				assert(cells[c->_sgcIdx].item == c);
				cells[c->_sgcIdx].item = c->_sgcNext;
				if (c->_sgcNext) {
					c->_sgcNext->_sgcPrev = {};
					c->_sgcNext = {};
				}
			}
			assert(cells[c->_sgcIdx].item != c);

			// stat
			--cells[c->_sgcIdx].count;

			// clear
			c->_sgcIdx = -1;
			c->_sgc = {};
		}

		void Update(Item* c) {
			assert(c);
			assert(c->_sgc == this);
			assert(c->_sgcIdx > -1);
			assert(c->_sgcNext != c);
			assert(c->_sgcPrev != c);
			assert(c->_radius * 2 <= cellSize);
			//assert(cells[c->_sgcIdx].item include c);

			auto idx = PosToCIdx(c->_x, c->_y);
			if (idx == c->_sgcIdx) return;	// no change
			assert(!cells[idx].item || !cells[idx].item->_sgcPrev);
			assert(!cells[c->_sgcIdx].item || !cells[c->_sgcIdx].item->_sgcPrev);

			// stat
			--cells[c->_sgcIdx].count;

			// unlink
			if (c->_sgcPrev) {	// isn't header
				assert(cells[c->_sgcIdx].item != c);
				c->_sgcPrev->_sgcNext = c->_sgcNext;
				if (c->_sgcNext) {
					c->_sgcNext->_sgcPrev = c->_sgcPrev;
					//c->_sgcNext = {};
				}
				//c->_sgcPrev = {};
			} else {
				assert(cells[c->_sgcIdx].item == c);
				cells[c->_sgcIdx].item = c->_sgcNext;
				if (c->_sgcNext) {
					c->_sgcNext->_sgcPrev = {};
					//c->_sgcNext = {};
				}
			}
			//c->_sgcIdx = -1;
			assert(cells[c->_sgcIdx].item != c);
			assert(idx != c->_sgcIdx);

			// link
			if (cells[idx].item) {
				cells[idx].item->_sgcPrev = c;
			}
			c->_sgcPrev = {};
			c->_sgcNext = cells[idx].item;
			cells[idx].item = c;
			c->_sgcIdx = idx;
			assert(!cells[idx].item->_sgcPrev);
			assert(c->_sgcNext != c);
			assert(c->_sgcPrev != c);

			// stat
			++cells[idx].count;
		}

		XX_INLINE int32_t PosToCIdx(int32_t x, int32_t y) {
			assert(x >= 0 && x < max.x);
			assert(y >= 0 && y < max.y);
			auto c = x / cellSize;
			assert(c >= 0 && c < numCols);
			auto r = y / cellSize;
			assert(r >= 0 && r < numRows);
			return r * numCols + c;
		}

		// return x: col index   y: row index
		XX_INLINE XYi PosToCrIdx(int32_t x, int32_t y) {
			assert(x >= 0 && x < max.x);
			assert(y >= 0 && y < max.y);
			return { x / cellSize, y / cellSize };
		}

		// return cell's index
		XX_INLINE int32_t CrIdxToCIdx(XYi crIdx) {
			return crIdx.y * numCols + crIdx.x;
		}

		// cell's index to pos( left top corner )
		XX_INLINE XYi CIdxToPos(int32_t cidx) {
			assert(cidx >= 0 && cidx < cellsLen);
			auto row = cidx / numCols;
			auto col = cidx - row * numCols;
			return { col * cellSize, row * cellSize };
		}

		// cell's index to cell center pos
		XX_INLINE XYi CIdxToCenterPos(int32_t cidx) {
			return CIdxToPos(cidx) + cellSize / 2;
		}

		XX_INLINE XYi CrIdxToPos(int32_t colIdx, int32_t rowIdx) {
			return CIdxToPos(rowIdx * numCols + colIdx);
		}

		XX_INLINE XYi CrIdxToCenterPos(int32_t colIdx, int32_t rowIdx) {
			return CIdxToCenterPos(rowIdx * numCols + colIdx);
		}


		/*******************************************************************************************************/
		/*******************************************************************************************************/
		// search functions
		using T = Item;

		// .ForeachCell([](T* o)->void {  all  });
		// .ForeachCell([](T* o)->bool {  break  });
		template <typename F, typename R = std::invoke_result_t<F, T*>>
		XX_INLINE void ForeachCell(int32_t cidx, F&& func) {
			auto c = cells[cidx].item;
			while (c) {
				auto nex = c->_sgcNext;
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}
		}

		// ring diffuse foreach ( usually for update logic )
		// .ForeachByRange([](T* o)->void {  all  });
		// .ForeachByRange([](T* o)->bool {  break  });
		template <bool enableExcept = false, typename F, typename R = std::invoke_result_t<F, T*>>
		void ForeachByRange(xx::SpaceGridRingDiffuseData const& d, int32_t x, int32_t y, int32_t maxDistance, F&& func, T* except = {}) {
			auto cIdxBase = x / cellSize;
			if (cIdxBase < 0 || cIdxBase >= numCols) return;
			auto rIdxBase = y / cellSize;
			if (rIdxBase < 0 || rIdxBase >= numRows) return;
			auto searchRange = maxDistance + cellSize;

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int32_t i = 1, e = lens.len; i < e; i++) {
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int32_t j = 0; j < size; ++j) {
					auto& tmp = idxs[offsets + j];
					auto cIdx = cIdxBase + tmp.x;
					if (cIdx < 0 || cIdx >= numCols) continue;
					auto rIdx = rIdxBase + tmp.y;
					if (rIdx < 0 || rIdx >= numRows) continue;
					auto cidx = rIdx * numCols + cIdx;

					auto c = cells[cidx].item;
					while (c) {
						auto nex = c->_sgcNext;
						if constexpr (enableExcept) {
							if (c == except) {
								c = nex;
								continue;
							}
						}
						if constexpr (std::is_void_v<R>) {
							func(c);
						} else {
							if (func(c)) return;
						}
						c = nex;
					}
				}
				if (lens[i].radius > searchRange) break;
			}
		}

		// foreach target cell + round 8 = 9 cells
		// .Foreach9All([](T& o)->void {  all  });
		// .Foreach9All([](T& o)->bool {  break  });
		template <bool enableExcept = false, typename F, typename R = std::invoke_result_t<F, T*>>
		void Foreach9All(int32_t x, int32_t y, F&& func, T* except = {}) {
			auto cIdx = x / cellSize;
			if (cIdx < 0 || cIdx >= numCols) return;
			auto rIdx = y / cellSize;
			if (rIdx < 0 || rIdx >= numRows) return;

			// 5
			auto idx = rIdx * numCols + cIdx;
			auto c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 6
			++cIdx;
			if (cIdx >= numCols) return;
			++idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 3
			++rIdx;
			if (rIdx >= numRows) return;
			idx += numCols;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 2
			--idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 1
			cIdx -= 2;
			if (cIdx < 0) return;
			--idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 4
			idx -= numCols;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 7
			rIdx -= 2;
			if (rIdx < 0) return;
			idx -= numCols;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 8
			++idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}

			// 9
			++idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;
				//if constexpr (enableExcept) {
				//	if (c == except) {
				//		c = nex;
				//		continue;
				//	}
				//}
				if constexpr (std::is_void_v<R>) {
					func(c);
				} else {
					if (func(c)) return;
				}
				c = nex;
			}
		}

		// foreach target cell + round 8 = 9 cells find first cross and return
		template<bool enableExcept = false>
		T* FindFirstCrossBy9(int32_t x, int32_t y, int32_t radius, T* except = {}) {
			auto cIdx = x / cellSize;
			if (cIdx < 0 || cIdx >= numCols) return nullptr;
			auto rIdx = y / cellSize;
			if (rIdx < 0 || rIdx >= numRows) return nullptr;

			// 5
			int32_t idx = rIdx * numCols + cIdx;
			auto c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 6
			++cIdx;
			if (cIdx >= numCols) return nullptr;
			++idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 3
			++rIdx;
			if (rIdx >= numRows) return nullptr;
			idx += numCols;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 2
			--idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 1
			cIdx -= 2;
			if (cIdx < 0) return nullptr;
			--idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 4
			idx -= numCols;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 7
			rIdx -= 2;
			if (rIdx < 0) return nullptr;
			idx -= numCols;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 8
			++idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 9
			++idx;
			c = cells[idx].item;
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->_x - x;
				auto vy = c->_y - y;
				auto r = c->_radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			return nullptr;
		}


		// ring diffuse search   nearest edge   best one and return
		template<bool enableExcept = false>
		T* FindNearestByRange(xx::SpaceGridRingDiffuseData const& d, int32_t x, int32_t y, int32_t maxDistance, T* except = {}) {
			auto cIdxBase = x / cellSize;
			if (cIdxBase < 0 || cIdxBase >= numCols) return nullptr;
			auto rIdxBase = y / cellSize;
			if (rIdxBase < 0 || rIdxBase >= numRows) return nullptr;
			auto searchRange = maxDistance + cellSize;

			T* rtv{};
			int32_t maxV{};

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int32_t i = 1, e = lens.len; i < e; i++) {
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int32_t j = 0; j < size; ++j) {
					auto& tmp = idxs[offsets + j];
					auto cIdx = cIdxBase + tmp.x;
					if (cIdx < 0 || cIdx >= numCols) continue;
					auto rIdx = rIdxBase + tmp.y;
					if (rIdx < 0 || rIdx >= numRows) continue;
					auto cidx = rIdx * numCols + cIdx;

					auto c = cells[cidx].item;
					while (c) {
						auto nex = c->_sgcNext;
						if constexpr (enableExcept) {
							if (c == except) {
								c = nex;
								continue;
							}
						}

						auto vx = c->_x - x;
						auto vy = c->_y - y;
						auto dd = vx * vx + vy * vy;
						auto r = maxDistance + c->_radius;
						auto v = r * r - dd;
						if (v > maxV) {
							rtv = c;
							maxV = v;
						}

						c = nex;
					}
				}
				if (lens[i].radius > searchRange) break;
			}

			return rtv;
		}


		// search result container. first: edge distance
		xx::Listi32<std::pair<int32_t, T*>> result_FindNearestN;

		// ring diffuse search   nearest edge   best N and return
		// maxDistance: search limit( edge distance )
		template<bool enableExcept = false>
		int32_t FindNearestNByRange(xx::SpaceGridRingDiffuseData const& d, int32_t x, int32_t y, int32_t maxDistance, int32_t n, T* except = {}) {
			auto cIdxBase = x / cellSize;
			if (cIdxBase < 0 || cIdxBase >= numCols) return 0;
			auto rIdxBase = y / cellSize;
			if (rIdxBase < 0 || rIdxBase >= numRows) return 0;
			auto searchRange = maxDistance + cellSize;

			auto& os = result_FindNearestN;
			os.Clear();

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int32_t i = 1, e = lens.len; i < e; i++) {
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int32_t j = 0; j < size; ++j) {
					auto& tmp = idxs[offsets + j];
					auto cIdx = cIdxBase + tmp.x;
					if (cIdx < 0 || cIdx >= numCols) continue;
					auto rIdx = rIdxBase + tmp.y;
					if (rIdx < 0 || rIdx >= numRows) continue;
					auto cidx = rIdx * numCols + cIdx;

					auto c = cells[cidx];
					while (c) {
						auto nex = c->_sgcNext;
						if constexpr (enableExcept) {
							if (c == except) {
								c = nex;
								continue;
							}
						}

						auto vx = c->_x - x;
						auto vy = c->_y - y;
						auto dd = vx * vx + vy * vy;
						auto r = maxDistance + c->_radius;
						auto v = r * r - dd;

						if (v > 0) {
							if (os.len < n) {
								os.Emplace(v, c);
								if (os.len == n) {
									Quick_Sort(0, os.len - 1);
								}
							} else {
								if (os[0].distance < v) {
									os[0] = { v, c };
									Quick_Sort(0, os.len - 1);
								}
							}
						}

						c = nex;
					}
				}
				if (lens[i].radius > searchRange) break;
			}
			return os.len;
		}

	protected:
		// sort result_FindNearestN
		void Quick_Sort(int32_t left, int32_t right) {
			if (left < right) {
				int32_t pivot = Partition(left, right);
				if (pivot > 1) {
					Quick_Sort(left, pivot - 1);
				}
				if (pivot + 1 < right) {
					Quick_Sort(pivot + 1, right);
				}
			}
		}

		// sort result_FindNearestN
		XX_INLINE int32_t Partition(int32_t left, int32_t right) {
			auto& arr = result_FindNearestN;
			auto pivot = arr[left];
			while (true) {
				while (arr[left].distance < pivot.first) {
					left++;
				}
				while (arr[right].distance > pivot.first) {
					right--;
				}
				if (left < right) {
					if (arr[left].first == arr[right].first) return right;
					auto temp = arr[left];
					arr[left] = arr[right];
					arr[right] = temp;
				} else return right;
			}
		}

	};

}
