#pragma once
#include "xx2d.h"
#include <xx2d_shader_texvert.h>
#include <spine/spine.h>

namespace xx {

	struct SpineListener : spine::AnimationStateListenerObject {
		using HandlerType = std::function<void(spine::AnimationState* state, spine::EventType type, spine::TrackEntry* entry, spine::Event* e)>;
		HandlerType h;
		SpineListener() = default;
		template<typename CB>
		void SetCallBack(CB&& cb) { h = std::forward<CB>(cb); }
		void callback(spine::AnimationState* state, spine::EventType type, spine::TrackEntry* entry, spine::Event* e) override;
	};

	// todo: more enhance like rotate ??
	struct SpinePlayer {
		spine::AnimationStateData animationStateData;
		spine::Skeleton skeleton;
		spine::AnimationState animationState;
		SpineListener spineListener;							// can SetCallBack( func )
		float timeScale{ 1 };
		mutable bool usePremultipliedAlpha{ false };

		SpinePlayer(spine::SkeletonData* skeletonData);
		SpinePlayer() = delete;
		SpinePlayer(SpinePlayer const&) = delete;
		SpinePlayer& operator=(SpinePlayer const&) = delete;

		spine::Vector<spine::Animation*>& GetAnimations();
		spine::Animation* FindAnimation(std::string_view name);
		spine::Bone* FindBone(std::string_view name);
		SpinePlayer& Update(float delta);
		GLVertTexture AnimToTexture(spine::Animation* anim, float frameDelay);
		GLVertTexture AnimToTexture(std::string_view animName, float frameDelay);
		void Draw(float cameraScale = 1.f);
		SpinePlayer& SetTimeScale(float t);
		SpinePlayer& SetUsePremultipliedAlpha(bool b);
		SpinePlayer& SetPosition(float x, float y);
		SpinePlayer& SetPosition(XY const& pos);		// whole
		SpinePlayer& SetScale(XY const& scale);			// whole
		SpinePlayer& SetFirstRotation(float r);			// first bone
		SpinePlayer& SetFirstScale(XY const& scale);	// first bone
		SpinePlayer& SetMix(spine::Animation* from, spine::Animation* to, float duration);
		SpinePlayer& SetMix(std::string_view fromName, std::string_view toName, float duration);
		SpinePlayer& SetAnimation(size_t trackIndex, spine::Animation* anim, bool loop);
		SpinePlayer& SetAnimation(size_t trackIndex, std::string_view animationName, bool loop);
		SpinePlayer& AddAnimation(size_t trackIndex, spine::Animation* anim, bool loop, float delay);
		SpinePlayer& AddAnimation(size_t trackIndex, std::string_view animationName, bool loop, float delay);

	protected:
		mutable spine::Vector<float> worldVertices;
		mutable spine::Vector<float> tempUvs;
		mutable spine::Vector<spine::Color> tempColors;
		mutable spine::Vector<unsigned short> quadIndices;
		mutable spine::SkeletonClipping clipper;
	};

	struct SpineTextureLoader : public spine::TextureLoader {
		void load(spine::AtlasPage& page, const spine::String& path) override;
		void unload(void* texture) override;
	};

	struct SpineExtension : spine::DefaultSpineExtension {
	protected:
		char* _readFile(const spine::String& pathStr, int* length) override;
	};

	// spine's global env
	struct SpineEnv {
		SpineExtension ext;
		SpineTextureLoader textureLoader;

		// key: file path
		std::unordered_map<std::string, Ref<GLTexture>, StdStringHash, std::equal_to<>> textures;								// need preload
		std::unordered_map<std::string, Data, StdStringHash, std::equal_to<>> fileDatas;										// need preload Atlas & SkeletonData files
		std::unordered_map<std::string, std::unique_ptr<spine::Atlas>, StdStringHash, std::equal_to<>> atlass;					// fill by AddAtlas
		std::unordered_map<std::string, std::unique_ptr<spine::SkeletonData>, StdStringHash, std::equal_to<>> skeletonDatas;	// fill by AddSkeletonData

		spine::Atlas* AddAtlas(std::string_view atlasFileName);

		template<bool skeletonFileIsJson>
		spine::SkeletonData* AddSkeletonData(spine::Atlas* atlas, std::string_view skeletonFileName, float scale = 1.f);

		void Init();

		template<bool skeletonFileIsJson = false>
		Task<> AsyncLoad(std::string const& baseFileNameWithPath, spine::SkeletonData*& sd, xx::Ref<xx::GLTexture>& tex, float scale = 1.f);
	};
	inline SpineEnv gSpineEnv;


	/*****************************************************************************************************************************************************************************************/
	/*****************************************************************************************************************************************************************************************/


	inline SpinePlayer::SpinePlayer(spine::SkeletonData* skeletonData)
		: animationStateData(skeletonData)
		, skeleton(skeletonData)
		, animationState(&animationStateData)
	{
		worldVertices.ensureCapacity(1000);
		tempUvs.ensureCapacity(16);
		tempColors.ensureCapacity(16);

		quadIndices.add(0);
		quadIndices.add(1);
		quadIndices.add(2);
		quadIndices.add(2);
		quadIndices.add(3);
		quadIndices.add(0);

		//skeleton->setToSetupPose();
	}

	inline SpinePlayer& SpinePlayer::Update(float delta) {
		animationState.update(delta * timeScale);
		animationState.apply(skeleton);
		skeleton.updateWorldTransform();
		return *this;
	}

	inline void SpinePlayer::Draw(float cameraScale) {
		auto& eg = EngineBase1::Instance();
		auto&& shader = eg.ShaderBegin(eg.shaderSpine38);

		// Early out if skeleton is invisible
		if (skeleton.getColor().a == 0) return;

		RGBA8 c{};
		GLTexture* tex{};

		for (size_t i = 0, e = skeleton.getSlots().size(); i < e; ++i) {

			auto&& slot = *skeleton.getDrawOrder()[i];
			auto&& attachment = slot.getAttachment();
			if (!attachment) continue;

			if (slot.getColor().a == 0 || !slot.getBone().isActive()) {
				clipper.clipEnd(slot);
				continue;
			}

			spine::Vector<float>* vertices = &worldVertices;
			spine::Vector<float>* uvs{};
			spine::Vector<unsigned short>* indices{};
			size_t indicesCount{};
			spine::Color* attachmentColor{};

			auto&& attachmentRTTI = attachment->getRTTI();
			if (attachmentRTTI.isExactly(spine::RegionAttachment::rtti)) {

				auto&& regionAttachment = (spine::RegionAttachment*)attachment;
				attachmentColor = &regionAttachment->getColor();

				if (attachmentColor->a == 0) {
					clipper.clipEnd(slot);
					continue;
				}

				worldVertices.setSize(8, 0);
				regionAttachment->computeWorldVertices(slot.getBone(), worldVertices, 0, 2);
				uvs = &regionAttachment->getUVs();
				indices = &quadIndices;
				indicesCount = 6;
				tex = (GLTexture*)((spine::AtlasRegion*)regionAttachment->getRendererObject())->page->getRendererObject();
			}
			else if (attachmentRTTI.isExactly(spine::MeshAttachment::rtti)) {

				auto&& mesh = (spine::MeshAttachment*)attachment;
				attachmentColor = &mesh->getColor();

				if (attachmentColor->a == 0) {
					clipper.clipEnd(slot);
					continue;
				}

				worldVertices.setSize(mesh->getWorldVerticesLength(), 0);
				mesh->computeWorldVertices(slot, 0, mesh->getWorldVerticesLength(), worldVertices.buffer(), 0, 2);
				uvs = &mesh->getUVs();
				indices = &mesh->getTriangles();
				indicesCount = mesh->getTriangles().size();
				tex = (GLTexture*)((spine::AtlasRegion*)mesh->getRendererObject())->page->getRendererObject();
			}
			else if (attachmentRTTI.isExactly(spine::ClippingAttachment::rtti)) {

				spine::ClippingAttachment* clip = (spine::ClippingAttachment*)slot.getAttachment();
				clipper.clipStart(slot, clip);
				continue;

			}
			else
				continue;

			auto&& skc = skeleton.getColor();
			auto&& slc = slot.getColor();
			c.r = (uint8_t)(skc.r * slc.r * attachmentColor->r * 255);
			c.g = (uint8_t)(skc.g * slc.g * attachmentColor->g * 255);
			c.b = (uint8_t)(skc.b * slc.b * attachmentColor->b * 255);
			c.a = (uint8_t)(skc.a * slc.a * attachmentColor->a * 255);

			std::pair<uint32_t, uint32_t> blend;

			// normal, additive, multiply, screen
			static const constexpr std::array<std::pair<uint32_t, uint32_t>, 4> nonpmaBlendFuncs{ std::pair<uint32_t, uint32_t>
			{ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA }, { GL_SRC_ALPHA, GL_ONE }, { GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA }, { GL_ONE, GL_ONE_MINUS_SRC_COLOR } };

			// pma: normal, additive, multiply, screen
			static const constexpr std::array<std::pair<uint32_t, uint32_t>, 4> pmaBlendFuncs{ std::pair<uint32_t, uint32_t>
			{ GL_ONE, GL_ONE_MINUS_SRC_ALPHA }, { GL_ONE, GL_ONE }, { GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA }, { GL_ONE, GL_ONE_MINUS_SRC_COLOR } };

			if (!usePremultipliedAlpha) {
				blend = nonpmaBlendFuncs[slot.getData().getBlendMode()];
			}
			else {
				blend = pmaBlendFuncs[slot.getData().getBlendMode()];
			}

			if (eg.blend[0] != blend.first || eg.blend[1] != blend.second || eg.blend[2] != GL_FUNC_ADD) {
				eg.ShaderEnd();
				eg.GLBlendFunc({ blend.first, blend.second, GL_FUNC_ADD });
			}

			if (clipper.isClipping()) {
				clipper.clipTriangles(worldVertices, *indices, *uvs, 2);
				vertices = &clipper.getClippedVertices();
				uvs = &clipper.getClippedUVs();
				indices = &clipper.getClippedTriangles();
				indicesCount = clipper.getClippedTriangles().size();
			}

			auto vs = shader.Draw(tex->GetValue(), (int32_t)indicesCount);
			XY size{ (float)tex->Width(), (float)tex->Height() };
			for (size_t ii = 0; ii < indicesCount; ++ii) {
				auto index = (*indices)[ii] << 1;
				auto&& vertex = vs[ii];
				vertex.pos.x = (*vertices)[index] * cameraScale;
				vertex.pos.y = (*vertices)[index + 1] * cameraScale;
				vertex.uv.x = (*uvs)[index] * size.x;
				vertex.uv.y = (*uvs)[index + 1] * size.y;
				(uint32_t&)vertex.color = (uint32_t&)c;
				//xx::CoutN("{ .pos = {", vertex.pos, " }, .uv = { ", vertex.uv, "}, .color = xx::RGBA8_White }");
			}

			clipper.clipEnd(slot);
		}

		clipper.clipEnd();
	}

	XX_INLINE spine::Vector<spine::Animation*>& SpinePlayer::GetAnimations() {
		return animationStateData.getSkeletonData()->getAnimations();
	}

	XX_INLINE spine::Animation* SpinePlayer::FindAnimation(std::string_view name) {
		return animationStateData.getSkeletonData()->findAnimation(name);
	}

	XX_INLINE spine::Bone* SpinePlayer::FindBone(std::string_view name) {
		return skeleton.findBone(name);
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetMix(spine::Animation* from, spine::Animation* to, float duration) {
		animationStateData.setMix(from, to, duration);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetMix(std::string_view fromName, std::string_view toName, float duration) {
		animationStateData.setMix(fromName, toName, duration);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetTimeScale(float t) {
		timeScale = t;
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetPosition(XY const& xy) {
		return SetPosition(xy.x, xy.y);
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetScale(XY const& xy) {
		skeleton.setScaleX(xy.x);
		skeleton.setScaleY(xy.y);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetFirstRotation(float r) {
		skeleton.getBones()[0]->setRotation(r);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetFirstScale(XY const& scale) {
		auto& b = skeleton.getBones()[0];
		b->setScaleX(scale.x);
		b->setScaleY(scale.y);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetPosition(float x, float y) {
		skeleton.setPosition(x, -y);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetAnimation(size_t trackIndex, std::string_view animationName, bool loop) {
		animationState.setAnimation(trackIndex, animationName, loop);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetAnimation(size_t trackIndex, spine::Animation* anim, bool loop) {
		animationState.setAnimation(trackIndex, anim, loop);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::AddAnimation(size_t trackIndex, spine::Animation* anim, bool loop, float delay) {
		animationState.addAnimation(trackIndex, anim, loop, delay);
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::AddAnimation(size_t trackIndex, std::string_view animationName, bool loop, float delay) {
		animationState.addAnimation(trackIndex, animationName, loop, delay);
		return *this;
	}

	// todo: multi anim pack to 1 tex
	inline GLVertTexture SpinePlayer::AnimToTexture(std::string_view animName, float frameDelay) {
		return AnimToTexture(animationStateData.getSkeletonData()->findAnimation(animName), frameDelay);
	}

	inline GLVertTexture SpinePlayer::AnimToTexture(spine::Animation* anim, float frameDelay) {
		auto& eg = EngineBase1::Instance();
		auto bak = eg.blend;
		SetPosition(0, 0).SetAnimation(0, anim, true);
		eg.ShaderEnd();
		Update(frameDelay);
		Draw();	// draw once for get vert size
		auto&& shader = eg.ShaderBegin(eg.shaderSpine38);
		auto vc = shader.vertsCount;

		auto rowByteSize = vc * 32;			// sizeof(float) * 8
		auto texWidth = rowByteSize / 16;	// sizeof(RGBA32F)
		texWidth = (texWidth + 7) & ~7u;	// align 8
		rowByteSize = texWidth * 16;

		auto numFrames = int32_t(anim->getDuration() / frameDelay);
		auto texHeight = numFrames;
		texHeight = (texHeight + 7) & ~7u;	// align 8

		auto d = std::make_unique<char[]>(rowByteSize * texHeight);
		auto vs = shader.verts.get();
		for (int i = 0; i < numFrames; ++i) {
			auto bp = (xx::TexData*)(d.get() + rowByteSize * i);
			for (int j = 0; j < vc; ++j) {
				auto& p = bp[j];
				auto& v = vs[j];
				p.x = v.pos.x;
				p.y = v.pos.y;
				p.u = v.uv.x;
				p.v = v.uv.y;
				static constexpr auto _255_1 = 1.f / 255.f;
				p.r = v.color.r * _255_1;
				p.g = v.color.g * _255_1;
				p.b = v.color.b * _255_1;
				p.a = v.color.a * _255_1;
			}
			shader.vertsCount = 0;
			shader.lastTextureId = 0;
			Update(frameDelay);
			Draw();
		}
		shader.vertsCount = 0;
		shader.lastTextureId = 0;
		eg.GLBlendFunc(bak);
		return xx::LoadGLVertTexture(d.get(), texWidth, texHeight, vc, numFrames);
	}

	/*****************************************************************************************************************************************************************************************/
	/*****************************************************************************************************************************************************************************************/


	inline char* SpineExtension::_readFile(const spine::String& pathStr, int* length) {
		std::string_view fn(pathStr.buffer(), pathStr.length());
		auto&& iter = gSpineEnv.fileDatas.find(fn);
		*length = (int)iter->second.len;
		return (char*)iter->second.buf;
	}

	/*****************************************************************************************************************************************************************************************/
	/*****************************************************************************************************************************************************************************************/


	inline void SpineTextureLoader::load(spine::AtlasPage& page, const spine::String& path) {
		std::string_view fn(path.buffer(), path.length());

		auto&& iter = gSpineEnv.textures.find(fn);
		auto&& tex = iter->second;
		tex->SetGLTexParm(
			page.magFilter == spine::TextureFilter_Linear ? GL_LINEAR : GL_NEAREST,
			(page.uWrap == spine::TextureWrap_Repeat && page.vWrap == spine::TextureWrap_Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE
		);

		page.width = tex->Width();
		page.height = tex->Height();
		++tex.GetHeader()->sharedCount;			// unsafe: ref
		page.setRendererObject(tex.pointer);
	}

	inline void SpineTextureLoader::unload(void* texture) {
		xx::Ref<xx::GLTexture> tex;
		tex.pointer = (xx::GLTexture*)texture;	// unsafe: deref
	}

	/*****************************************************************************************************************************************************************************************/
	/*****************************************************************************************************************************************************************************************/


	inline void SpineListener::callback(spine::AnimationState* state, spine::EventType type, spine::TrackEntry* entry, spine::Event* e) {
		h(state, type, entry, e);
	}

	/*****************************************************************************************************************************************************************************************/
	/*****************************************************************************************************************************************************************************************/


	inline void SpineEnv::Init() {
		spine::SpineExtension::setInstance(&ext);
	}

	template<bool skeletonFileIsJson>
	inline Task<> SpineEnv::AsyncLoad(std::string const& baseFileNameWithPath, spine::SkeletonData*& sd, xx::Ref<xx::GLTexture>& tex, float scale) {
		assert(spine::SpineExtension::getInstance());	// forget gSpineEnv.Init() ?
		auto fnTex = baseFileNameWithPath + ".png";
		auto fnAtlas = baseFileNameWithPath + ".atlas";
		// todo: error check?
		auto& eg = EngineBase3::Instance();
		textures.emplace(fnTex, co_await eg.AsyncLoadTextureFromUrl(fnTex));
		fileDatas.emplace(fnAtlas, co_await eg.AsyncDownloadFromUrl(fnAtlas));
		auto a = AddAtlas(fnAtlas);
		if constexpr (skeletonFileIsJson) {
			auto fnJson = baseFileNameWithPath + ".json";
			fileDatas.emplace(fnJson, co_await eg.AsyncDownloadFromUrl(fnJson));
			sd = AddSkeletonData<true>(a, fnJson, scale);
		}
		else {
			auto fnSkel = baseFileNameWithPath + ".skel";
			fileDatas.emplace(fnSkel, co_await eg.AsyncDownloadFromUrl(fnSkel));
			sd = AddSkeletonData<false>(a, fnSkel, scale);
		}
		tex = textures[fnTex];
		fileDatas.clear();
	}

	inline spine::Atlas* SpineEnv::AddAtlas(std::string_view atlasFileName) {
		auto r = atlass.emplace(atlasFileName, std::make_unique<spine::Atlas>(atlasFileName, &gSpineEnv.textureLoader));
		if (!r.second) return nullptr;
		return r.first->second.get();
	}

	template<bool skeletonFileIsJson>
	inline spine::SkeletonData* SpineEnv::AddSkeletonData(spine::Atlas* atlas, std::string_view skeletonFileName, float scale) {
		std::conditional_t<skeletonFileIsJson, spine::SkeletonJson, spine::SkeletonBinary> parser(atlas);
		parser.setScale(scale);
		auto sd = parser.readSkeletonDataFile(skeletonFileName);
		assert(sd);
		auto r = skeletonDatas.emplace(skeletonFileName, sd);
		if (!r.second) return nullptr;
		return r.first->second.get();
	}

}
