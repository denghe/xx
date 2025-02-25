﻿#pragma once
#include <xx2d_camera.h>

// tiled map xml version data loader & container. full supported to version 1.9x( compress algorithm only support zstandard )
// https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#

namespace xx::TMX {

	struct alignas(8) RefBase {};	// for all Ref<T>

	enum class PropertyTypes : uint8_t {
		Bool,
		Color,
		Float,
		File,
		Int,
		Object,
		String,
		MAX_VALUE_UNKNOWN
	};

	struct Object;
	struct Property {
		PropertyTypes type = PropertyTypes::MAX_VALUE_UNKNOWN;
		std::string name;
		std::variant<bool, RGBA8, int64_t, double, std::unique_ptr<std::string>, Object*> value;
	};

	/**********************************************************************************/

	enum class ObjectTypes : uint8_t {
		Point,
		Ellipse,
		Polygon,
		Rectangle,
		Tile,
		Text,
		MAX_VALUE_UNKNOWN
	};

	struct alignas(8) Object : RefBase {
		ObjectTypes type = ObjectTypes::MAX_VALUE_UNKNOWN;
		uint32_t id = 0;
		std::string name;
		std::string class_;
		double x = 0;
		double y = 0;
		double rotation = false;
		bool visible = true;
		std::vector<Property> properties;	// <properties> <property <property
	};

	struct Object_Point : Object {};

	struct Object_Rectangle : Object {
		uint32_t width = 0;
		uint32_t height = 0;
	};

	struct Object_Ellipse : Object_Rectangle {};

	struct Pointi {
		int32_t x = 0, y = 0;
	};

	struct Pointf {
		double x = 0, y = 0;
	};

	struct Object_Polygon : Object {
		std::vector<Pointf> points;
	};

	struct Object_Tile : Object_Rectangle {
		uint32_t gid = 0;					// & 0x3FFFFFFFu;
		bool flippingHorizontal = false;	// (gid >> 31) & 1
		bool flippingVertical = false;		// (gid >> 30) & 1
	};

	enum class HAligns : uint8_t {
		Left,
		Center,
		Right,
		Justify,
		MAX_VALUE_UNKNOWN
	};

	enum class VAligns : uint8_t {
		Top,
		Center,
		Bottom,
		MAX_VALUE_UNKNOWN
	};

	struct Object_Text : Object_Rectangle {
		std::string fontfamily;
		uint32_t pixelsize = 16;
		RGBA8 color = { 0, 0, 0, 255 };
		bool wrap = false;
		bool bold = false;
		bool italic = false;
		bool underline = false;
		bool strikeout = false;
		bool kerning = true;
		HAligns halign = HAligns::Left;
		VAligns valign = VAligns::Top;
		std::string text;
	};

	/**********************************************************************************/

	enum class LayerTypes : uint8_t {
		TileLayer,
		ObjectLayer,
		ImageLayer,
		GroupLayer,
		MAX_VALUE_UNKNOWN
	};

	struct Layer : RefBase {
		LayerTypes type = LayerTypes::MAX_VALUE_UNKNOWN;
		uint32_t id = 0;
		std::string name;
		std::string class_;	// class
		bool visible = true;
		bool locked = false;
		double opacity = 1;	// 0 ~ 1
		std::optional<RGBA8> tintColor;	// tintcolor
		double horizontalOffset = 0;	// offsetx
		double verticalOffset = 0;	// offsety
		Pointf parallaxFactor = { 1, 1 };	// parallaxx, parallaxy
		std::vector<Property> properties;	// <properties> <property <property
	};

	struct Chunk {
		std::vector<uint32_t> gids;
		uint32_t height = 0;
		uint32_t width = 0;
		Pointi pos;	// x, y
	};

	struct Layer_Tile : Layer {
		std::vector<Chunk> chunks;	// when map.infinite == true
		std::vector<uint32_t> gids;	// when map.infinite == false
	};

	enum class DrawOrders : uint8_t {
		TopDown,
		Index,
		MAX_VALUE_UNKNOWN
	};

	struct Layer_Object : Layer {
		std::optional<RGBA8> color;
		DrawOrders draworder = DrawOrders::TopDown;
		std::vector<Ref<Object>> objects;
	};

	struct Layer_Group : Layer {
		std::vector<Ref<Layer>> layers;
	};

	struct Image : RefBase {
		std::string source;
		uint32_t width;
		uint32_t height;
		std::optional<RGBA8> transparentColor;	// trans="ff00ff"
		Ref<GLTexture> texture;
	};

	struct Layer_Image : Layer {
		Ref<Image> image;
		bool repeatX = false;	// repeatx
		bool repeatY = false;	// repeaty
	};

	/**********************************************************************************/

	struct Frame {
		uint32_t tileId = 0;	// tileid
		uint32_t gid = 0;
		uint32_t duration = 0;
	};
	struct Tile {
		uint32_t id = 0;
		std::string class_;
		Ref<Image> image;	// <image ...>
		Ref<Layer_Object> collisions;	// <objectgroup> <object <object
		std::vector<Frame> animation;	// <animation> <frame <frame
		std::vector<Property> properties;	// <properties> <property <property
	};

	struct WangTile {
		uint32_t tileId = 0;	// tileid
		uint32_t gid = 0;
		std::vector<uint8_t> wangIds;	// wangid
	};

	struct WangColor {
		std::string name;
		RGBA8 color = { 0, 0, 0, 255 };
		uint32_t tile = 0;
		double probability = 1;
		std::vector<Property> properties;	// <properties> <property <property
	};

	enum class WangSetTypes : uint8_t {
		Corner,
		Edge,
		Mixed,
		MAX_VALUE_UNKNOWN
	};

	struct WangSet {
		std::string name;
		WangSetTypes type = WangSetTypes::Corner;
		uint32_t tile = 0;
		std::vector<WangTile> wangTiles;	// <wangtile
		std::vector<std::unique_ptr<WangColor>> wangColors;	// <wangcolor
		std::vector<Property> properties;	// <properties> <property <property
	};

	struct Transformations {
		bool flipHorizontally = false;	// hflip
		bool flipVertically = false;	// vflip
		bool rotate = false;
		bool preferUntransformedTiles = false;	// preferuntransformed
	};

	enum class Orientations : uint8_t {
		Orthogonal,
		Isometric,
		Staggered,	// not for Tileset
		Hexagonal,	// not for Tileset
		MAX_VALUE_UNKNOWN
	};

	enum class ObjectAlignment : uint8_t {
		Unspecified,
		TopLeft,
		Top,
		TopRight,
		Left,
		Center,
		Right,
		BottomLeft,
		Bottom,
		BottomRight,
		MAX_VALUE_UNKNOWN
	};

	enum class TileRenderSizes : uint8_t {
		TileSize,
		MapGridSize,
		MAX_VALUE_UNKNOWN
	};

	enum class FillModes : uint8_t {
		Stretch,
		PreserveAspectFit,
		MAX_VALUE_UNKNOWN
	};

	struct Tileset : RefBase {
		uint32_t firstgid;	// .tmx map/tileset.firstgid
		std::string source;	// .tmx map/tileset.source

		// following fields in source .tsx file
		std::string name;
		std::string class_;	// class
		ObjectAlignment objectAlignment = ObjectAlignment::Unspecified;	// objectalignment
		Pointi drawingOffset;	// <tileoffset x= y=
		TileRenderSizes tileRenderSize = TileRenderSizes::TileSize; // tilerendersize
		FillModes fillMode = FillModes::Stretch;	// fillmode
		std::optional<RGBA8> backgroundColor;	// backgroundcolor
		Orientations orientation = Orientations::Orthogonal;	// <grid orientation=
		uint32_t gridWidth = 0;	// <grid width=
		uint32_t gridHeight = 0;	// <grid height=
		uint32_t columns = 0;
		Transformations allowedTransformations;	// <transformations ...
		Ref<Image> image;
		uint32_t tilewidth = 0;
		uint32_t tileheight = 0;
		uint32_t margin = 0;
		uint32_t spacing = 0;
		std::vector<Property> properties;	// <properties> <property <property

		std::string version;
		std::string tiledversion;
		uint32_t tilecount = 0;

		std::vector<std::unique_ptr<WangSet>> wangSets;	// <wangsets>
		std::vector<std::unique_ptr<Tile>> tiles;	// <tile <tile <tile
	};

	/**********************************************************************************/

	enum class RenderOrders : uint8_t {
		RightDown,
		RightUp,
		LeftDown,
		LeftUp,
		MAX_VALUE_UNKNOWN
	};

	enum class Encodings : uint8_t {
		Csv,
		Base64,	// support Compressions
		Xml,	// deprecated
		MAX_VALUE_UNKNOWN
	};

	enum class Compressions : uint8_t {
		Uncompressed,
		Gzip,
		Zlib,
		Zstd,
		MAX_VALUE_UNKNOWN
	};

	enum class StaggerAxiss : uint8_t {
		X,
		Y,
		MAX_VALUE_UNKNOWN
	};

	enum class StaggerIndexs : uint8_t {
		Odd,
		Even,
		MAX_VALUE_UNKNOWN
	};

	struct TileLayerFormat {
		Encodings encoding;
		Compressions compression;
	};

	/****************************************************/
	// ext for fast search gid & get texture & quad data
	struct GidInfo {
		Tileset* tileset{};
		Tile* tile{};	// maybe nullptr
		Image* image{};
		bool IsSingleImage() const {
			return tile && tile->image == image;
		}
		void* ud{};	// user data
		operator bool() const {
			return image;
		}

		Ref<Anim> anim;
		Ref<::xx::Frame> frame;
		Ref<::xx::Frame> const& GetFrame() const {
			if (anim) return anim->GetCurrentAnimFrame().frame;
			else return frame;
		}
	};
	/****************************************************/

	struct Map {
		std::string class_;	// class
		Orientations orientation = Orientations::Orthogonal;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t tileWidth = 0;	// tilewidth
		uint32_t tileHeight = 0;	// tileheight
		bool infinite = false;
		uint32_t tileSideLength = 0;	// hexsidelength
		StaggerAxiss staggeraxis = StaggerAxiss::Y;	// staggeraxis
		StaggerIndexs staggerindex = StaggerIndexs::Odd;	// staggerindex
		Pointf parallaxOrigin;	// parallaxoriginx, parallaxoriginy
		TileLayerFormat tileLayerFormat = { Encodings::Csv,  Compressions::Uncompressed };	// for every layer's data
		uint32_t outputChunkWidth = 16;
		uint32_t outputChunkHeight = 16;
		RenderOrders renderOrder = RenderOrders::RightDown;	// renderorder
		int32_t compressionLevel = -1;	// compressionlevel
		std::optional<RGBA8> backgroundColor;	// backgroundcolor
		std::vector<Property> properties;	// <properties> <property <property

		std::string version;
		std::string tiledVersion;	// tiledversion
		uint32_t nextLayerId = 0;	// nextlayerid
		uint32_t nextObjectId = 0;	// nextobjectid
		std::vector<Ref<Tileset>> tilesets;
		std::vector<Ref<Layer>> layers;


		/****************************************************/
		// calc utils. before call, camera need Calc()
		int GetMinColumnIndex(Camera const& cam, int offset = 0) const;
		int GetMaxColumnIndex(Camera const& cam, int offset = 0) const;
		int GetMinRowIndex(Camera const& cam, int offset = 0) const;
		int GetMaxRowIndex(Camera const& cam, int offset = 0) const;
		XY GetBasePos(Camera const& cam) const;
		XY GetScaledTileSize(Camera const& cam) const;

		/****************************************************/
		// ext
		std::vector<Ref<Image>> images;													// all textures here
		std::vector<GidInfo> gidInfos;													// all gid info here. index == gid
		std::vector<Anim*> anims;														// point to all gid info's anim for easy update anims
		std::vector<Layer*> flatLayers;													// extract layers tree here for easy search by name

		void FillExts();																// fill above containers

		template<std::convertible_to<Layer> LT>
		LT* FindLayer(std::string_view const& name) const;								// find layer by name( from flatLayers )

		void FillFlatLayers(std::vector<Ref<Layer>>& ls);

		GidInfo* GetGidInfo(Layer* L, uint32_t rowIdx, uint32_t colIdx) const;	// for Layer_Tile gidInfos[rowIdx * map.w + colIdx]
	
		/****************************************************/
	};

	XX_INLINE int Map::GetMinColumnIndex(Camera const& camera, int offset) const {
		int r = int(camera.minX / tileWidth) + offset;
		return r < 0 ? 0 : r;
	}

	XX_INLINE int Map::GetMaxColumnIndex(Camera const& camera, int offset) const {
		int r = int(camera.maxX / tileWidth) + offset;
		return r > (int)width ? (int)width : r;
	}

	XX_INLINE int Map::GetMinRowIndex(Camera const& camera, int offset) const {
		int r = int(camera.minY / tileHeight) + offset;
		return r < 0 ? 0 : r;
	}

	XX_INLINE int Map::GetMaxRowIndex(Camera const& camera, int offset) const {
		int r = int(camera.maxY / tileHeight) + offset;
		return r > (int)height ? (int)height : r;
	}

	XX_INLINE XY Map::GetBasePos(Camera const& camera) const {
		return XY{ -camera.original.x, float(-(int)tileHeight) + camera.original.y } * camera.scale;
	}

	XX_INLINE XY Map::GetScaledTileSize(Camera const& camera) const {
		return { tileWidth * camera.scale, tileHeight * camera.scale };
	}

	inline void Map::FillFlatLayers(std::vector<Ref<Layer>>& ls) {
		for (auto& l : ls) {
			if (l->type == LayerTypes::GroupLayer) {
				FillFlatLayers(((Layer_Group&)*l).layers);
			} else {
				flatLayers.push_back(l.pointer);
			}
		}
	}

	template<std::convertible_to<Layer> LT>
	LT* Map::FindLayer(std::string_view const& name) const {
		for (auto& l : flatLayers) {
			if (l->name == name) {
				if constexpr (std::is_same_v<LT, Layer_Group>) xx_assert(l->type == LayerTypes::GroupLayer);
				if constexpr (std::is_same_v<LT, Layer_Image>) xx_assert(l->type == LayerTypes::ImageLayer);
				if constexpr (std::is_same_v<LT, Layer_Object>) xx_assert(l->type == LayerTypes::ObjectLayer);
				if constexpr (std::is_same_v<LT, Layer_Tile>) xx_assert(l->type == LayerTypes::TileLayer);
				return (LT*)l;
			}
		}
		return nullptr;
	}

	XX_INLINE GidInfo* Map::GetGidInfo(Layer* L, uint32_t rowIdx, uint32_t colIdx) const {
		assert(L);
		assert(L->type == LayerTypes::TileLayer);
		assert(rowIdx < height);
		assert(colIdx < width);
		assert(!infinite);	// if (infinite) todo
		auto gid = ((Layer_Tile&)*L).gids[rowIdx * width + colIdx];
		if (!gid) return nullptr;
		return (GidInfo*)&gidInfos[gid];
	}

	// need fill images GLTexture first if needed
	inline void Map::FillExts() {
		gidInfos.clear();
		anims.clear();
		if (tilesets.empty()) return;

		uint32_t maxGid{};
		for (auto&& tileset : tilesets) {
			for (auto&& o : tileset->tiles) {
				auto gid = tileset->firstgid + o->id;
				if (gid > maxGid) {
					maxGid = gid;
				}
			}
		}
		if (!maxGid) return;
		gidInfos.resize(maxGid + 1);

		// fill info
		for (auto&& tileset : tilesets) {

			TMX::Image* img = nullptr;
			uint32_t numRows = 1, numCols = tileset->tilecount;
			if (tileset->image) {
				numCols = tileset->columns;
				numRows = tileset->tilecount / numCols;
				img = tileset->image;
			}

			// tileset? single picture?
			if (img) {
				for (uint32_t y = 0; y < numRows; ++y) {
					for (uint32_t x = 0; x < numCols; ++x) {
						auto id = y * numCols + x;
						auto gid = tileset->firstgid + id;
						auto& info = gidInfos[gid];

						info.tileset = tileset;
						info.tile = nullptr;
						info.image = img;

						for (auto& t : tileset->tiles) {	// search tile
							if (t->id == id) {
								info.tile = t.get();
								if (t->image) {
									info.image = t->image;	// override to single image
								}
								break;
							}
						}

						auto& f = info.frame.Emplace();
						f->tex = info.image->texture;
						if (f->tex) {
							f->key = f->tex->FileName();
						}
						f->anchor = { 0.5, 0.5 };
						auto u = (float)tileset->margin + (tileset->spacing + tileset->tilewidth) * x;
						auto v = (float)tileset->margin + (tileset->spacing + tileset->tileheight) * y;
						auto w = (float)tileset->tilewidth;
						auto h = (float)tileset->tileheight;
						f->anchor = { 0, tileHeight / h };
						f->spriteSize = { w, h };
						f->textureRect = { (uint16_t)u, (uint16_t)v, (uint16_t)w, (uint16_t)h };
					}
				}
			} else {
				for (auto& tile : tileset->tiles) {
					auto gid = tileset->firstgid + tile->id;
					auto& info = gidInfos[gid];

					info.tileset = tileset;
					info.tile = tile.get();
					info.image = tile->image;

					auto& f = info.frame.Emplace();
					f->tex = info.image->texture;
					if (f->tex) {
						f->key = f->tex->FileName();
					}
					f->anchor = { 0.5, 0.5 };
					auto w = (float)info.image->width;
					auto h = (float)info.image->height;
					f->anchor = { 0, tileHeight / h };
					f->spriteSize = { w, h };
					f->textureRect = { 0, 0, (uint16_t)w, (uint16_t)h };
				}
			}

		}

		// fill anim
		for (size_t gid = 1, siz = gidInfos.size(); gid < siz; ++gid) {
			auto& info = gidInfos[gid];
			if (info.tile) {
				auto& tas = info.tile->animation;
				if (auto&& numAnims = tas.size()) {
					auto& afs = info.anim.Emplace()->animFrames;
					afs.resize(numAnims);
					for (size_t x = 0; x < numAnims; ++x) {
						afs[x].frame = gidInfos[tas[x].gid].frame;
						afs[x].durationSeconds = tas[x].duration / 1000.f;
					}
					anims.emplace_back(gidInfos[gid].anim.pointer);
				}
			}
		}

		// fill flatLayers
		FillFlatLayers(layers);
	}
}
