#pragma once
#include "xx_blocklink.h"
#include "xx_xy.h"

namespace xx
{
	template <typename T>
	struct SpaceNode : BlockLinkVI
	{
		int32_t nex, pre, cidx;
		T value;
	};

	struct SpaceCountRadius
	{
		int32_t count;
		float radius;
	};

	struct SpaceRingDiffuseData
	{
		Listi32<SpaceCountRadius> lens;
		Listi32<XYi> idxs;

		void Init(int gridNumRows, int cellSize)
		{
			lens.Emplace(0, 0.f);
			idxs.Emplace();
			std::unordered_set<uint64_t> set;
			set.insert(0);
			for (float radius = 0; radius < cellSize * gridNumRows; radius += cellSize)
			{
				auto lenBak = idxs.len;
				auto radians = std::asin(0.5f / radius) * 2;
				auto step = (int)(M_PI * 2 / radians);
				auto inc = M_PI * 2 / step;
				for (int i = 0; i < step; ++i)
				{
					auto a = inc * i;
					auto cos = std::cos(a);
					auto sin = std::sin(a);
					auto ix = (int)(cos * radius) / cellSize;
					auto iy = (int)(sin * radius) / cellSize;
					auto key = ((uint64_t)iy << 32) + (uint64_t)ix;
					if (set.insert(key).second)
					{
						idxs.Emplace(ix, iy);
					}
				}
				if (idxs.len > lenBak)
				{
					lens.Emplace(idxs.len, radius);
				}
			}
		}
	};

	template <typename T>
	using SpaceWeak = BlockLinkWeak<T, SpaceNode>;

	// requires
	// T has member: XY pos
	template <typename T, typename ST = BlockLink<T, SpaceNode>>
	struct SpaceGrid : protected ST
	{
		using ST::ST;
		using NodeType = typename ST::NodeType;

		using ST::Count;
		using ST::Reserve;
		using ST::TryGet;

		int32_t numRows{}, numCols{}, cellSize{};
		double _1_cellSize{}; // = 1 / cellSize
		XYi max{};

	protected:
		int32_t cellsLen{};
		std::unique_ptr<int32_t[]> cells;

	public:
		void Init(int32_t numRows_, int32_t numCols_, int32_t cellSize_)
		{
			assert(!cells);
			assert(numRows_ > 0 && numCols_ > 0 && cellSize_ > 0);
			numRows = numRows_;
			numCols = numCols_;
			cellSize = cellSize_;
			_1_cellSize = 1. / cellSize_;
			max.x = cellSize_ * numCols_;
			max.y = cellSize_ * numRows_;

			cellsLen = numRows * numCols;
			cells = std::make_unique<int32_t[]>(cellsLen);
			memset(cells.get(), -1, sizeof(int32_t) * cellsLen); // -1 mean empty
		}

		template <bool freeBuf = false, bool resetVersion = false>
		void Clear()
		{
			if (!cells) return;
			ST::template Clear<freeBuf, resetVersion>();
			memset(cells.get(), -1, sizeof(int32_t) * cellsLen);
		}

		// Emplace + Init( args ) + cells[ pos ] = o
		template <typename... Args>
		NodeType& EmplaceNodeInit(Args&&... args)
		{
			assert(cells);
			auto& o = ST::EmplaceCore();
			o.value.Init(std::forward<Args>(args)...);

			auto cidx = PosToCIdx(o.value.pos);
			auto head = cells[cidx]; // backup
			if (head >= 0)
			{
				ST::RefNode(head).pre = o.index;
			}
			cells[cidx] = o.index; // assign new
			o.nex = head;
			o.pre = -1;
			o.cidx = cidx;
			return o;
		}

		template <typename... Args>
		T& EmplaceInit(Args&&... args)
		{
			return EmplaceNodeInit(std::forward<Args>(args)...).value;
		}

	protected:
		XX_FORCE_INLINE void Free(NodeType& o)
		{
			assert(o.pre != o.index && o.nex != o.index && o.cidx >= 0);

			if (o.index == cells[o.cidx])
			{
				cells[o.cidx] = o.nex;
			}
			if (o.pre >= 0)
			{
				ST::RefNode(o.pre).nex = o.nex;
			}
			if (o.nex >= 0)
			{
				ST::RefNode(o.nex).pre = o.pre;
			}
			//o.pre = -1;
			//o.cidx = -1;

			ST::Free(o);
		}

	public:
		void Remove(T const& v)
		{
			auto o = container_of(&v, NodeType, value);
			Free(*o);
		}

		bool Remove(BlockLinkVI const& vi)
		{
			if (vi.version >= -2 || vi.index < 0 || vi.index >= this->len) return false;
			auto& o = ST::RefNode(vi.index);
			if (o.version != vi.version) return false;
			Free(o);
			return true;
		}

		void Update(T& v)
		{
			auto& o = *container_of(&v, NodeType, value);
			assert(o.index >= 0);
			assert(o.pre != o.index);
			assert(o.nex != o.index);
			auto cidx = PosToCIdx(v.pos);
			if (cidx == o.cidx) return; // no change

			// unlink
			if (o.index != cells[o.cidx])
			{
				// isn't head
				ST::RefNode(o.pre).nex = o.nex;
				if (o.nex >= 0)
				{
					ST::RefNode(o.nex).pre = o.pre;
					//o.nex = -1;
				}
				//o.pre = -1;
			}
			else
			{
				// is head
				assert(o.pre == -1);
				cells[o.cidx] = o.nex;
				if (o.nex >= 0)
				{
					ST::RefNode(o.nex).pre = -1;
					//o.nex = -1;
				}
			}
			//o.cidx = -1;

			// relink
			if (cells[cidx] >= 0)
			{
				ST::RefNode(cells[cidx]).pre = o.index;
			}
			o.nex = cells[cidx];
			o.pre = -1;
			cells[cidx] = o.index;
			o.cidx = cidx;
		}

		// very slowly when row cells large size
		// foreach by flags ( copy ForeachFlags to here )
		// .Foreach([](T& o)->void {    });
		// .Foreach([](T& o)->xx::ForeachResult {    });
		template <typename F, typename R = std::invoke_result_t<F, T&>>
		void Foreach(F&& func)
		{
			if (ST::len <= 0) return;

			for (int32_t i = 0, n = ST::blocks.len - 1; i <= n; ++i)
			{
				auto& block = *(typename ST::Block*)ST::blocks[i];
				auto& flags = block.flags;
				if (!flags) continue;

				auto left = ST::len & 0b111111;
				int32_t e = (i < n || !left) ? 64 : left;
				for (int32_t j = 0; j < e; ++j)
				{
					auto& o = block.buf[j];
					auto bit = uint64_t(1) << j;
					if ((flags & bit) == 0)
					{
						assert(o.version >= -2);
						continue;
					}
					assert(o.version < -2);

					if constexpr (std::is_void_v<R>)
					{
						func(o.value);
					}
					else
					{
						auto r = func(o.value);
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
			}
		}

		XX_FORCE_INLINE int32_t PosToCIdx(XYf const& p)
		{
			assert(p.x >= 0 && p.x < cellSize * numCols);
			assert(p.y >= 0 && p.y < cellSize * numRows);
			auto c = int32_t(p.x * _1_cellSize);
			assert(c >= 0 && c < numCols);
			auto r = int32_t(p.y * _1_cellSize);
			assert(r >= 0 && r < numRows);
			return r * numCols + c;
		}

		// return x: col index   y: row index
		XX_FORCE_INLINE XYi PosToCrIdx(XYf const& p)
		{
			assert(p.x >= 0 && p.x < cellSize * numCols);
			assert(p.y >= 0 && p.y < cellSize * numRows);
			return {p.x * _1_cellSize, p.y * _1_cellSize};
		}

		// return cell's index
		XX_FORCE_INLINE int32_t CrIdxToCIdx(XYi const& crIdx)
		{
			return crIdx.y * numCols + crIdx.x;
		}

		// cell's index to pos( left top corner )
		XX_FORCE_INLINE XYf CIdxToPos(int32_t cidx)
		{
			assert(cidx >= 0 && cidx < cellsLen);
			auto row = cidx / numCols;
			auto col = cidx - row * numCols;
			return {float(col * cellSize), float(row * cellSize)};
		}

		// cell's index to cell center pos
		XX_FORCE_INLINE XYf CIdxToCenterPos(int32_t cidx)
		{
			return CIdxToPos(cidx) + float(cellSize) * 0.5f;
		}

		XX_FORCE_INLINE XYf CrIdxToPos(int32_t colIdx, int32_t rowIdx)
		{
			return CIdxToPos(rowIdx * numCols + colIdx);
		}

		XX_FORCE_INLINE XYf CrIdxToCenterPos(int32_t colIdx, int32_t rowIdx)
		{
			return CIdxToCenterPos(rowIdx * numCols + colIdx);
		}

		T* TryGetCellItemByPos(XY const& p)
		{
			if (p.x < 0 || p.x >= max.x || p.y < 0 || p.y >= max.y) return nullptr;
			auto cidx = PosToCIdx(p);
			auto idx = cells[cidx];
			if (idx < 0) return nullptr;
			return &ST::RefNode(idx).value;
		}

		// // .Foreach9([](T& o)->void {  all  });
		// // .Foreach([](T& o)->bool {  break  });
		// // .Foreach([](T& o)->xx::ForeachResult {    });
		// // return is Break or RemoveAndBreak
		// template <typename F, typename R = std::invoke_result_t<F, T&>>
		// XX_FORCE_INLINE bool ForeachCell(int32_t cidx, F&& func)
		// {
		// 	auto idx = cells[cidx];
		// 	while (idx >= 0)
		// 	{
		// 		auto& o = ST::RefNode(idx);
		// 		auto nex = o.nex;
		// 		if constexpr (std::is_void_v<R>)
		// 		{
		// 			func(o.value);
		// 		}
		// 		else
		// 		{
		// 			auto r = func(o.value);
		// 			if constexpr (std::is_same_v<R, bool>)
		// 			{
		// 				if (r) return true;
		// 			}
		// 			else
		// 			{
		// 				switch (r)
		// 				{
		// 				case ForeachResult::Continue: break;
		// 				case ForeachResult::RemoveAndContinue:
		// 					Free(o);
		// 					break;
		// 				case ForeachResult::Break: return true;
		// 				case ForeachResult::RemoveAndBreak:
		// 					Free(o);
		// 					return true;
		// 				default:
		// 					XX_ASSUME(false);
		// 				}
		// 			}
		// 		}
		// 		idx = nex;
		// 	}
		// 	return false;
		// }

		
		// constexpr static std::array<XYi, 9> offsets9 = {
		// 	XYi
		// 	{0, 0},
		// 	{-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}
		// };
		//
		// // foreach target cell + round 8 = 9 cells
		// // .Foreach9([](T& o)->void {  all  });
		// // .Foreach9([](T& o)->bool {  break  });
		// // .Foreach9([](T& o)->xx::ForeachResult {    });
		// template <typename F, typename R = std::invoke_result_t<F, T&>>
		// void Foreach9(XYf const& pos, F&& func)
		// {
		// 	auto crIdxBase = PosToCrIdx(pos);
		// 	for (auto offset : offsets9)
		// 	{
		// 		auto crIdx = crIdxBase + offset;
		// 		if (crIdx.x < 0 || crIdx.x >= numCols) continue;
		// 		if (crIdx.y < 0 || crIdx.y >= numRows) continue;
		// 		auto cidx = CrIdxToCIdx(crIdx);
		//
		// 		auto idx = cells[cidx];
		// 		while (idx >= 0)
		// 		{
		// 			auto& o = ST::RefNode(idx);
		// 			auto nex = o.nex;
		// 			if constexpr (std::is_void_v<R>)
		// 			{
		// 				func(o.value);
		// 			}
		// 			else
		// 			{
		// 				auto r = func(o.value);
		// 				if constexpr (std::is_same_v<R, bool>)
		// 				{
		// 					if (r) return;
		// 				}
		// 				else
		// 				{
		// 					switch (r)
		// 					{
		// 					case ForeachResult::Continue: break;
		// 					case ForeachResult::RemoveAndContinue:
		// 						Free(o);
		// 						break;
		// 					case ForeachResult::Break: return;
		// 					case ForeachResult::RemoveAndBreak:
		// 						Free(o);
		// 						return;
		// 					default:
		// 						XX_ASSUME(false);
		// 					}
		// 				}
		// 			}
		// 			idx = nex;
		// 		}
		// 	}
		// }

		// // can't break
		// template <typename F, typename R = std::invoke_result_t<F, T&>>
		// void ForeachByRange(SpaceRingDiffuseData const& d, XYf const& pos, float maxDistance, F&& func)
		// {
		// 	auto crIdxBase = PosToCrIdx(pos); // calc grid col row index
		// 	float rr = maxDistance * maxDistance;
		// 	auto& lens = d.lens;
		// 	auto& idxs = d.idxs;
		// 	for (int i = 1; i < lens.len; i++)
		// 	{
		// 		auto offsets = &idxs[lens[i - 1].count];
		// 		auto size = lens[i].count - lens[i - 1].count;
		//
		// 		for (int j = 0; j < size; ++j)
		// 		{
		// 			auto crIdx = crIdxBase + offsets[j];
		// 			if (crIdx.x < 0 || crIdx.x >= numCols) continue;
		// 			if (crIdx.y < 0 || crIdx.y >= numRows) continue;
		// 			auto cidx = CrIdxToCIdx(crIdx);
		// 			ForeachCell(cidx, [&](T& m)-> void
		// 			{
		// 				auto v = m.pos - pos;
		// 				if (v.x * v.x + v.y * v.y < rr)
		// 				{
		// 					func(m); // todo: check func's args. send v, rr to func ?
		// 				}
		// 			});
		// 		}
		//
		// 		if (lens[i].radius > maxDistance) break; // limit search range
		// 	}
		// }


		// ring diffuse foreach ( usually for update logic )
		template <typename F, typename R = std::invoke_result_t<F, T&>>
		void ForeachByRange(SpaceRingDiffuseData const& d, float x, float y, float maxDistance, F&& func)
		{
			int cIdxBase = (int)(x * _1_cellSize);
			if (cIdxBase < 0 || cIdxBase >= numCols) return;
			int rIdxBase = (int)(y * _1_cellSize);
			if (rIdxBase < 0 || rIdxBase >= numRows) return;
			auto searchRange = maxDistance + cellSize;

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int i = 1, e = lens.len; i < e; i++)
			{
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int j = 0; j < size; ++j)
				{
					auto& tmp = idxs[offsets + j];
					auto cIdx = cIdxBase + tmp.x;
					if (cIdx < 0 || cIdx >= numCols) continue;
					auto rIdx = rIdxBase + tmp.y;
					if (rIdx < 0 || rIdx >= numRows) continue;
					auto cidx = rIdx * numCols + cIdx;

					auto idx = cells[cidx];
					while (idx >= 0)
					{
						auto& c = ST::RefNode(idx);
						auto& o = c.value;

						if constexpr (std::is_void_v<R>)
						{
							func(o);
						}
						else
						{
							auto r = func(o);
							if constexpr (std::is_same_v<R, bool>)
							{
								if (r) return;
							}
							else
							{
								switch (r)
								{
								case ForeachResult::Continue: break;
								case ForeachResult::RemoveAndContinue:
									Free(c);
									break;
								case ForeachResult::Break: return;
								case ForeachResult::RemoveAndBreak:
									Free(c);
									return;
								default:
									XX_ASSUME(false);
								}
							}
						}

						idx = c.nex;
					}
				}
				if (lens[i].radius > searchRange) break;
			}
		}

		

		/*******************************************************************************************************/
		/*******************************************************************************************************/
		// other impls ( special & faster ? )

		// todo: more test & bug fix ( because copy from c# )

		// foreach target cell + round 8 = 9 cells
		// .Foreach9All([](T& o)->void {  all  });
		// .Foreach9All([](T& o)->bool {  break  });
		// .Foreach9All([](T& o)->xx::ForeachResult {    });
		template <typename F, typename R = std::invoke_result_t<F, T&>>
		void Foreach9All(float x, float y, F&& func)
		{
			int cIdx = (int)(x * _1_cellSize);
			if (cIdx < 0 || cIdx >= numCols) return;
			int rIdx = (int)(y * _1_cellSize);
			if (rIdx < 0 || rIdx >= numRows) return;

			// 5
			int idx = rIdx * numCols + cIdx;
			auto i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 6
			++cIdx;
			if (cIdx >= numCols) return;
			++idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 3
			++rIdx;
			if (rIdx >= numRows) return;
			idx += numCols;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 2
			--idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 1
			cIdx -= 2;
			if (cIdx < 0) return;
			--idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 4
			idx -= numCols;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 7
			rIdx -= 2;
			if (rIdx < 0) return;
			idx -= numCols;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 8
			++idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}

			// 9
			++idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& o = ST::RefNode(i);
				auto nex = o.nex;
				if constexpr (std::is_void_v<R>)
				{
					func(o.value);
				}
				else
				{
					auto r = func(o.value);
					if constexpr (std::is_same_v<R, bool>)
					{
						if (r) return;
					}
					else
					{
						switch (r)
						{
						case ForeachResult::Continue: break;
						case ForeachResult::RemoveAndContinue:
							Free(o);
							break;
						case ForeachResult::Break: return;
						case ForeachResult::RemoveAndBreak:
							Free(o);
							return;
						default:
							XX_ASSUME(false);
						}
					}
				}
				i = nex;
			}
		}

		// foreach target cell + round 8 = 9 cells find first cross and return ( tested )
		// required: XY T::pos
		// required: float T::radius
		T* FindFirstCrossBy9(float x, float y, float radius)
		{
			int cIdx = (int)(x * _1_cellSize);
			if (cIdx < 0 || cIdx >= numCols) return nullptr;
			int rIdx = (int)(y * _1_cellSize);
			if (rIdx < 0 || rIdx >= numRows) return nullptr;

			// 5
			int idx = rIdx * numCols + cIdx;
			auto i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 6
			++cIdx;
			if (cIdx >= numCols) return nullptr;
			++idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 3
			++rIdx;
			if (rIdx >= numRows) return nullptr;
			idx += numCols;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 2
			--idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 1
			cIdx -= 2;
			if (cIdx < 0) return nullptr;
			--idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 4
			idx -= numCols;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 7
			rIdx -= 2;
			if (rIdx < 0) return nullptr;
			idx -= numCols;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 8
			++idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			// 9
			++idx;
			i = cells[idx];
			while (i >= 0)
			{
				auto& c = ST::RefNode(i);
				auto& o = c.value;

				auto vx = o.pos.x - x;
				auto vy = o.pos.y - y;
				auto r = o.radius + radius;
				if (vx * vx + vy * vy < r * r)
				{
					return &o;
				}

				i = c.nex;
			}

			return nullptr;
		}


		// ring diffuse search   nearest edge   best one and return
		// required: float T::radius
		T* FindNearestByRange(SpaceRingDiffuseData const& d, float x, float y, float maxDistance)
		{
			int cIdxBase = (int)(x * _1_cellSize);
			if (cIdxBase < 0 || cIdxBase >= numCols) return nullptr;
			int rIdxBase = (int)(y * _1_cellSize);
			if (rIdxBase < 0 || rIdxBase >= numRows) return nullptr;
			auto searchRange = maxDistance + cellSize;

			T* rtv = nullptr;
			float maxV{};

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int i = 1, e = lens.len; i < e; i++)
			{
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int j = 0; j < size; ++j)
				{
					auto& tmp = idxs[offsets + j];
					auto cIdx = cIdxBase + tmp.x;
					if (cIdx < 0 || cIdx >= numCols) continue;
					auto rIdx = rIdxBase + tmp.y;
					if (rIdx < 0 || rIdx >= numRows) continue;
					auto cidx = rIdx * numCols + cIdx;

					auto idx = cells[cidx];
					while (idx >= 0)
					{
						auto& c = ST::RefNode(idx);
						auto& o = c.value;

						auto vx = o.pos.x - x;
						auto vy = o.pos.y - y;
						auto dd = vx * vx + vy * vy;
						auto r = maxDistance + o.radius;
						auto v = r * r - dd;
						if (v > maxV)
						{
							rtv = &o;
							maxV = v;
						}

						idx = c.nex;
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
		// required: float T::radius
		int FindNearestNByRange(SpaceRingDiffuseData const& d, float x, float y, float maxDistance, int n)
		{
			int cIdxBase = (int)(x * _1_cellSize);
			if (cIdxBase < 0 || cIdxBase >= numCols) return 0;
			int rIdxBase = (int)(y * _1_cellSize);
			if (rIdxBase < 0 || rIdxBase >= numRows) return 0;
			auto searchRange = maxDistance + cellSize;

			auto& os = result_FindNearestN;
			os.Clear();

			auto& lens = d.lens;
			auto& idxs = d.idxs;
			for (int i = 1, e = lens.len; i < e; i++)
			{
				auto offsets = lens[i - 1].count;
				auto size = lens[i].count - lens[i - 1].count;
				for (int j = 0; j < size; ++j)
				{
					auto& tmp = idxs[offsets + j];
					auto cIdx = cIdxBase + tmp.x;
					if (cIdx < 0 || cIdx >= numCols) continue;
					auto rIdx = rIdxBase + tmp.y;
					if (rIdx < 0 || rIdx >= numRows) continue;
					auto cidx = rIdx * numCols + cIdx;

					auto idx = cells[cidx];
					while (idx >= 0)
					{
						auto& c = ST::RefNode(idx);
						auto& o = c.value;

						auto vx = o.x - x;
						auto vy = o.y - y;
						auto dd = vx * vx + vy * vy;
						auto r = maxDistance + o.radius;
						auto v = r * r - dd;

						if (v > 0)
						{
							if (os.len < n)
							{
								os.Emplace(v, &o);
								if (os.len == n)
								{
									Quick_Sort(0, os.len - 1);
								}
							}
							else
							{
								if (os[0].distance < v)
								{
									os[0] = {v, &o};
									Quick_Sort(0, os.len - 1);
								}
							}
						}

						idx = c.nex;
					}
				}
				if (lens[i].radius > searchRange) break;
			}
			return os.len;
		}

	protected:
		// sort result_FindNearestN
		void Quick_Sort(int left, int right)
		{
			if (left < right)
			{
				int pivot = Partition(left, right);
				if (pivot > 1)
				{
					Quick_Sort(left, pivot - 1);
				}
				if (pivot + 1 < right)
				{
					Quick_Sort(pivot + 1, right);
				}
			}
		}

		// sort result_FindNearestN
		XX_FORCE_INLINE int Partition(int left, int right)
		{
			auto& arr = result_FindNearestN;
			auto pivot = arr[left];
			while (true)
			{
				while (arr[left].distance < pivot.first)
				{
					left++;
				}
				while (arr[right].distance > pivot.first)
				{
					right--;
				}
				if (left < right)
				{
					if (arr[left].first == arr[right].first) return right;
					auto temp = arr[left];
					arr[left] = arr[right];
					arr[right] = temp;
				}
				else return right;
			}
		}
	};
}
