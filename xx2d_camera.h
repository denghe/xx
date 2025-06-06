﻿#pragma once
#include <xx2d_base0.h>

namespace xx {

	struct Camera {
		// need set
		float scale{ 1 }, zoom{ 1 };
		XY original{};												// logic center position
		XY maxFrameSize{};											// for calculate safe area

		// calc fills
		float width_2{}, height_2{};								// logic size: window size * zoom
		float minX{}, maxX{}, minY{}, maxY{};						// 4 corner pos
		float safeMinX{}, safeMaxX{}, safeMinY{}, safeMaxY{};		// corner + max tex/frame size

		XX_INLINE Camera& SetMaxFrameSize(XY const& maxFrameSize_) {
			maxFrameSize = maxFrameSize_;
			return *this;
		}

		XX_INLINE Camera& SetOriginal(XY const& original_) {
			original = original_;
			return *this;
		}

		XX_INLINE Camera& SetScale(float scale_) {
			scale = scale_;
			zoom = 1 / scale_;
			return *this;
		}

		XX_INLINE Camera& SetZoom(float zoom_) {
			zoom = zoom_;
			scale = 1 / zoom_;
			return *this;
		}

		XX_INLINE Camera& AddScale(float offset) {
			return SetScale(scale + offset);
		}

		XX_INLINE Camera& IncreaseScale(float v, float maxScale) {
			scale += v;
			if (scale > maxScale) scale = maxScale;
			zoom = 1 / scale;
			return *this;
		}

		XX_INLINE Camera& DecreaseScale(float v, float minScale) {
			scale -= v;
			if (scale < minScale) scale = minScale;
			zoom = 1 / scale;
			return *this;
		}

		// need set original & set scale or zoom
		void Calc() {
			width_2 = gEngine->windowSize_2.x * zoom;
			height_2 = gEngine->windowSize_2.y * zoom;

			minX = original.x - width_2;
			maxX = original.x + width_2;
			minY = original.y - height_2;
			maxY = original.y + height_2;

			safeMinX = minX - maxFrameSize.x;
			safeMaxX = maxX + maxFrameSize.x;
			safeMinY = minY - maxFrameSize.y;
			safeMaxY = maxY + maxFrameSize.y;
		}

		// following funcs Calc() is required

		template<typename XY_t>
		XX_INLINE bool InArea(XY_t const& pos) const {
			return pos.x >= safeMinX && pos.x <= safeMaxX
				&& pos.y >= safeMinY && pos.y <= safeMaxY;
		}

		// need calc
		XX_INLINE XY ToGLPos(XY const& logicPos) const {
			return (logicPos - original).FlipY() * scale;
		}
		XX_INLINE XY ToGLPos(float x, float y) const {
			return ToGLPos({ x, y });
		}

		XX_INLINE XY ToLogicPos(XY const& glPos) const {
			return (glPos * zoom).FlipY() + original;
		}

		XX_INLINE XY ToLogicPos(XY const& glPos, float globalScale) const {
			return (glPos / (scale * globalScale)).FlipY() + original;
		}




		// calc row col index range for space grid circle
		/*
			int32_t rowFrom, rowTo, colFrom, colTo;
			camera.FillRowColIdxRange(gCfg.physNumRows, gCfg.physNumCols, gCfg.physCellSize,
				rowFrom, rowTo, colFrom, colTo);

			for (int32_t rowIdx = rowFrom; rowIdx < rowTo; ++rowIdx) {
				for (int32_t colIdx = colFrom; colIdx < colTo; ++colIdx) {
					auto idx = sgcPhysItems.CrIdxToCellIdx({ colIdx, rowIdx });
					sgcPhysItems.ForeachWithoutBreak(idx, [&](ScenePhysItem* o) {
						iys.Emplace(o, o->posY);
					});
				}
			}

			grids.Foreach([&]<typename T>(SGS::SG<T>&grid) {
				for (int32_t rowIdx = rowFrom; rowIdx < rowTo; ++rowIdx) {
					for (int32_t colIdx = colFrom; colIdx < colTo; ++colIdx) {
						auto idx = grid.CrIdxToCIdx({ colIdx, rowIdx });
						grid.ForeachCell(idx, [&](T& o) {
							bases.Emplace(&o);
						});
					}
				}
			});


		*/
		template<int32_t rowFromOffset = 1, int32_t rowToOffset = 2, int32_t colFromOffset = 1, int32_t colToOffset = 2>
		void FillRowColIdxRange(int32_t physNumRows, int32_t physNumCols, int32_t physCellSize, int32_t& rowFrom, int32_t& rowTo, int32_t& colFrom, int32_t& colTo) {
			int32_t halfNumRows = int32_t(gEngine->windowSize.y / scale) / physCellSize / 2;
			int32_t posRowIndex = (int32_t)original.y / physCellSize;
			rowFrom = posRowIndex - halfNumRows - rowFromOffset;
			rowTo = posRowIndex + halfNumRows + rowToOffset;
			if (rowFrom < 0) {
				rowFrom = 0;
			}
			if (rowTo > physNumRows) {
				rowTo = physNumRows;
			}

			int32_t halfNumCols = int32_t(gEngine->windowSize.x / scale) / physCellSize / 2;
			int32_t posColIndex = (int32_t)original.x / physCellSize;
			colFrom = posColIndex - halfNumCols - colFromOffset;
			colTo = posColIndex + halfNumCols + colToOffset;
			if (colFrom < 0) {
				colFrom = 0;
			}
			if (colTo > physNumCols) {
				colTo = physNumCols;
			}
		}

	};

}
