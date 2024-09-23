#pragma once
#include "xx_xy.h"

namespace xx {

	// require XYf Item::pos
	// require float Item::radius

    template<typename Item>
    struct SpaceGrid2;

    // for inherit
    template<typename Derived>
    struct SpaceGrid2Item {
        SpaceGrid2<Derived> *_sgc{};
        Derived *_sgcPrev{}, *_sgcNext{};
        int32_t _sgcIdx{ -1 };
    };

    template<typename Item>
    struct SpaceGrid2 {
        int32_t numRows{}, numCols{}, cellSize{};
        double _1_cellSize{}; // = 1 / cellSize
        XYi max{};
        int32_t cellsLen{};
        std::unique_ptr<Item*[]> cells;

		// unsafe
		void Clear() {
			assert(cells);
			memset(cells.get(), 0, sizeof(Item*) * cellsLen);
		}

        void Init(int32_t const& numRows_, int32_t numCols_, int32_t cellSize_) {
            assert(!cells);
            assert(numRows_ > 0 && numCols_ > 0 && cellSize_ > 0);
            numRows = numRows_;
            numCols = numCols_;
            cellSize = cellSize_;
            _1_cellSize = 1. / cellSize_;
            max.x = cellSize_ * numCols_;
            max.y = cellSize_ * numRows_;

            cellsLen = numRows * numCols;
            cells = std::make_unique<Item*[]>(cellsLen);
			Clear();
        }

        void Add(Item* c) {
            assert(c);
            assert(!c->_sgc);
            assert(c->_sgcIdx == -1);
            assert(!c->_sgcPrev);
            assert(!c->_sgcNext);
            assert(c->pos.x >= 0 && c->pos.x < max.x);
            assert(c->pos.y >= 0 && c->pos.y < max.y);
			c->_sgc = this;

            // calc rIdx & cIdx
            auto idx = PosToCIdx(c->pos);
            assert(idx <= cellsLen);
            assert(!cells[idx] || !cells[idx]->_sgcPrev);

            // link
            if (cells[idx]) {
                cells[idx]->_sgcPrev = c;
            }
            c->_sgcNext = cells[idx];
            c->_sgcIdx = idx;
            cells[idx] = c;
            assert(!cells[idx]->_sgcPrev);
            assert(c->_sgcNext != c);
            assert(c->_sgcPrev != c);

            //// stat
            //++numItems;
        }

        void Remove(Item* c) {
            assert(c);
            assert(c->_sgc == this);
            assert(!c->_sgcPrev && cells[c->_sgcIdx] == c || c->_sgcPrev->_sgcNext == c && cells[c->_sgcIdx] != c);
            assert(!c->_sgcNext || c->_sgcNext->_sgcPrev == c);
            //assert(cells[c->_sgcIdx] include c);

            // unlink
            if (c->_sgcPrev) {	// isn't header
                assert(cells[c->_sgcIdx] != c);
                c->_sgcPrev->_sgcNext = c->_sgcNext;
                if (c->_sgcNext) {
                    c->_sgcNext->_sgcPrev = c->_sgcPrev;
                    c->_sgcNext = {};
                }
                c->_sgcPrev = {};
            } else {
                assert(cells[c->_sgcIdx] == c);
                cells[c->_sgcIdx] = c->_sgcNext;
                if (c->_sgcNext) {
                    c->_sgcNext->_sgcPrev = {};
                    c->_sgcNext = {};
                }
            }
            assert(cells[c->_sgcIdx] != c);
            c->_sgcIdx = -1;
			c->_sgc = {};

            //// stat
            //--numItems;
        }

        void Update(Item* c) {
            assert(c);
            assert(c->_sgc == this);
            assert(c->_sgcIdx > -1);
            assert(c->_sgcNext != c);
            assert(c->_sgcPrev != c);
            //assert(cells[c->_sgcIdx] include c);

            auto idx = PosToCIdx(c->pos);
            if (idx == c->_sgcIdx) return;	// no change
            assert(!cells[idx] || !cells[idx]->_sgcPrev);
            assert(!cells[c->_sgcIdx] || !cells[c->_sgcIdx]->_sgcPrev);

            // unlink
            if (c->_sgcPrev) {	// isn't header
                assert(cells[c->_sgcIdx] != c);
                c->_sgcPrev->_sgcNext = c->_sgcNext;
                if (c->_sgcNext) {
                    c->_sgcNext->_sgcPrev = c->_sgcPrev;
                    //c->_sgcNext = {};
                }
                //c->_sgcPrev = {};
            } else {
                assert(cells[c->_sgcIdx] == c);
                cells[c->_sgcIdx] = c->_sgcNext;
                if (c->_sgcNext) {
                    c->_sgcNext->_sgcPrev = {};
                    //c->_sgcNext = {};
                }
            }
            //c->_sgcIdx = -1;
            assert(cells[c->_sgcIdx] != c);
            assert(idx != c->_sgcIdx);

            // link
            if (cells[idx]) {
                cells[idx]->_sgcPrev = c;
            }
            c->_sgcPrev = {};
            c->_sgcNext = cells[idx];
            cells[idx] = c;
            c->_sgcIdx = idx;
            assert(!cells[idx]->_sgcPrev);
            assert(c->_sgcNext != c);
            assert(c->_sgcPrev != c);
        }


        XX_FORCE_INLINE int32_t PosToCIdx(XYf const& p) {
            assert(p.x >= 0 && p.x < cellSize * numCols);
            assert(p.y >= 0 && p.y < cellSize * numRows);
            auto c = int32_t(p.x * _1_cellSize);
            assert(c >= 0 && c < numCols);
            auto r = int32_t(p.y * _1_cellSize);
            assert(r >= 0 && r < numRows);
            return r * numCols + c;
        }

        // return x: col index   y: row index
        XX_FORCE_INLINE XYi PosToCrIdx(XYf const& p) {
            assert(p.x >= 0 && p.x < cellSize * numCols);
            assert(p.y >= 0 && p.y < cellSize * numRows);
            return { p.x * _1_cellSize, p.y * _1_cellSize };
        }

        // return cell's index
        XX_FORCE_INLINE int32_t CrIdxToCIdx(XYi const& crIdx) {
            return crIdx.y * numCols + crIdx.x;
        }

        // cell's index to pos( left top corner )
        XX_FORCE_INLINE XYf CIdxToPos(int32_t cidx) {
            assert(cidx >= 0 && cidx < cellsLen);
            auto row = cidx / numCols;
            auto col = cidx - row * numCols;
            return { float(col * cellSize), float(row * cellSize) };
        }

        // cell's index to cell center pos
        XX_FORCE_INLINE XYf CIdxToCenterPos(int32_t cidx) {
            return CIdxToPos(cidx) + float(cellSize) * 0.5f;
        }

        XX_FORCE_INLINE XYf CrIdxToPos(int32_t colIdx, int32_t rowIdx) {
            return CIdxToPos(rowIdx * numCols + colIdx);
        }

        XX_FORCE_INLINE XYf CrIdxToCenterPos(int32_t colIdx, int32_t rowIdx) {
            return CIdxToCenterPos(rowIdx * numCols + colIdx);
        }


		/*******************************************************************************************************/
		/*******************************************************************************************************/
		// search functions
		// todo: more test & bug fix ( because copy from c# )
		using T = Item;

		// .ForeachCell([](T* o)->void {  all  });
		// .ForeachCell([](T* o)->bool {  break  });
		template <typename F, typename R = std::invoke_result_t<F, T*>>
		XX_FORCE_INLINE void ForeachCell(int32_t cidx, F&& func) {
			auto c = cells[cidx];
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
		void ForeachByRange(SpaceGridRingDiffuseData const& d, float x, float y, float maxDistance, F&& func, T* except = {}) {
			int cIdxBase = (int)(x * _1_cellSize);
			if (cIdxBase < 0 || cIdxBase >= numCols) return;
			int rIdxBase = (int)(y * _1_cellSize);
			if (rIdxBase < 0 || rIdxBase >= numRows) return;
			auto searchRange = maxDistance + cellSize;

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int i = 1, e = lens.len; i < e; i++) {
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int j = 0; j < size; ++j) {
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
		void Foreach9All(float x, float y, F&& func, T* except = {}) {
			int cIdx = (int)(x * _1_cellSize);
			if (cIdx < 0 || cIdx >= numCols) return;
			int rIdx = (int)(y * _1_cellSize);
			if (rIdx < 0 || rIdx >= numRows) return;

			// 5
			int idx = rIdx * numCols + cIdx;
			auto c = cells[idx];
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
			c = cells[idx];
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

			// 3
			++rIdx;
			if (rIdx >= numRows) return;
			idx += numCols;
			c = cells[idx];
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

			// 2
			--idx;
			c = cells[idx];
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

			// 1
			cIdx -= 2;
			if (cIdx < 0) return;
			--idx;
			c = cells[idx];
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

			// 4
			idx -= numCols;
			c = cells[idx];
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

			// 7
			rIdx -= 2;
			if (rIdx < 0) return;
			idx -= numCols;
			c = cells[idx];
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

			// 8
			++idx;
			c = cells[idx];
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

			// 9
			++idx;
			c = cells[idx];
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

		// foreach target cell + round 8 = 9 cells find first cross and return ( tested )
		template<bool enableExcept = false>
		T* FindFirstCrossBy9(float x, float y, float radius, T* except = {}) {
			int cIdx = (int)(x * _1_cellSize);
			if (cIdx < 0 || cIdx >= numCols) return nullptr;
			int rIdx = (int)(y * _1_cellSize);
			if (rIdx < 0 || rIdx >= numRows) return nullptr;

			// 5
			int idx = rIdx * numCols + cIdx;
			auto c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 6
			++cIdx;
			if (cIdx >= numCols) return nullptr;
			++idx;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 3
			++rIdx;
			if (rIdx >= numRows) return nullptr;
			idx += numCols;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 2
			--idx;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 1
			cIdx -= 2;
			if (cIdx < 0) return nullptr;
			--idx;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 4
			idx -= numCols;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 7
			rIdx -= 2;
			if (rIdx < 0) return nullptr;
			idx -= numCols;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 8
			++idx;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			// 9
			++idx;
			c = cells[idx];
			while (c) {
				auto nex = c->_sgcNext;

				if constexpr (enableExcept) {
					if (c == except) {
						c = nex;
						continue;
					}
				}
				auto vx = c->pos.x - x;
				auto vy = c->pos.y - y;
				auto r = c->radius + radius;
				if (vx * vx + vy * vy < r * r) return c;

				c = nex;
			}

			return nullptr;
		}


		// ring diffuse search   nearest edge   best one and return
		template<bool enableExcept = false>
		T* FindNearestByRange(SpaceGridRingDiffuseData const& d, float x, float y, float maxDistance, T* except = {}) {
			int cIdxBase = (int)(x * _1_cellSize);
			if (cIdxBase < 0 || cIdxBase >= numCols) return nullptr;
			int rIdxBase = (int)(y * _1_cellSize);
			if (rIdxBase < 0 || rIdxBase >= numRows) return nullptr;
			auto searchRange = maxDistance + cellSize;

			T* rtv{};
			float maxV{};

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int i = 1, e = lens.len; i < e; i++) {
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int j = 0; j < size; ++j) {
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

						auto vx = c->pos.x - x;
						auto vy = c->pos.y - y;
						auto dd = vx * vx + vy * vy;
						auto r = maxDistance + c->radius;
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
		Listi32<std::pair<float, T*>> result_FindNearestN;

		// ring diffuse search   nearest edge   best N and return
		// maxDistance: search limit( edge distance )
		template<bool enableExcept = false>
		int FindNearestNByRange(SpaceGridRingDiffuseData const& d, float x, float y, float maxDistance, int n, T* except = {}) {
			int cIdxBase = (int)(x * _1_cellSize);
			if (cIdxBase < 0 || cIdxBase >= numCols) return 0;
			int rIdxBase = (int)(y * _1_cellSize);
			if (rIdxBase < 0 || rIdxBase >= numRows) return 0;
			auto searchRange = maxDistance + cellSize;

			auto& os = result_FindNearestN;
			os.Clear();

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int i = 1, e = lens.len; i < e; i++) {
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int j = 0; j < size; ++j) {
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

						auto vx = c->pos.x - x;
						auto vy = c->pos.y - y;
						auto dd = vx * vx + vy * vy;
						auto r = maxDistance + c->radius;
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
		void Quick_Sort(int left, int right) {
			if (left < right) {
				int pivot = Partition(left, right);
				if (pivot > 1) {
					Quick_Sort(left, pivot - 1);
				}
				if (pivot + 1 < right) {
					Quick_Sort(pivot + 1, right);
				}
			}
		}

		// sort result_FindNearestN
		XX_FORCE_INLINE int Partition(int left, int right) {
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
