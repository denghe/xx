#pragma once
#include "xx2d.h"
#include <spine/spine.h>

namespace xx {

	struct SpineSkeleton {
		spine::Skeleton* skeleton{};
		spine::AnimationState* state{};
		float timeScale{ 1 };
		mutable bool usePremultipliedAlpha{ true };

		SpineSkeleton(spine::SkeletonData* skeletonData, spine::AnimationStateData* stateData = 0);
		~SpineSkeleton();

		void Update(float delta);
		void Draw();

	protected:
		mutable bool ownsAnimationStateData{};
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
		SpineSkeleton spineSkeleton;
		SpineListener spineListener;	// can SetCallBack( func )

		SpinePlayer(spine::SkeletonData* skeletonData);
		SpinePlayer() = delete;
		SpinePlayer(SpinePlayer const&) = delete;
		SpinePlayer& operator=(SpinePlayer const&) = delete;

		SpinePlayer& SetMix(std::string_view fromName, std::string_view toName, float duration);
		SpinePlayer& SetTimeScale(float t);
		SpinePlayer& SetUsePremultipliedAlpha(bool b);
		SpinePlayer& SetPosition(float x, float y);
		SpinePlayer& AddAnimation(size_t trackIndex, std::string_view animationName, bool loop, float delay);
		SpinePlayer& Update(float delta);
		void Draw();
	};

	struct SpineExtension : spine::DefaultSpineExtension {
	protected:
		char* _readFile(const spine::String& pathStr, int* length) override;
	};


	// spine's global env
	struct SpineEnv {
		SpineExtension ext;
		SpineTextureLoader textureLoader;
		spine::String tmpStr, tmpStr2;

		// key: file path
		std::unordered_map<std::string, Ref<GLTexture>, StdStringHash, std::equal_to<>> textures;								// need preload
		std::unordered_map<std::string, Data, StdStringHash, std::equal_to<>> fileDatas;										// need preload Atlas & SkeletonData files
		std::unordered_map<std::string, std::unique_ptr<spine::Atlas>, StdStringHash, std::equal_to<>> atlass;					// fill by AddAtlas
		std::unordered_map<std::string, std::unique_ptr<spine::SkeletonData>, StdStringHash, std::equal_to<>> skeletonDatas;	// fill by AddSkeletonData

		spine::Atlas* AddAtlas(std::string_view atlasFileName);

		template<bool skeletonFileIsJson>
		spine::SkeletonData* AddSkeletonData(spine::Atlas* atlas, std::string_view skeletonFileName, float scale = 1.f);

		void Init();

	};
	inline SpineEnv gSpineEnv;


	/*****************************************************************************************************************************************************************************************/
	/*****************************************************************************************************************************************************************************************/


	inline char* SpineExtension::_readFile(const spine::String& pathStr, int* length) {
		std::string_view fn(pathStr.buffer(), pathStr.length());
		auto&& iter = gSpineEnv.fileDatas.find(fn);
		*length = (int)iter->second.len;
		return (char*)iter->second.buf;
	}


	inline SpineSkeleton::SpineSkeleton(spine::SkeletonData* skeletonData, spine::AnimationStateData* stateData) {

		spine::Bone::setYDown(true);

		worldVertices.ensureCapacity(1000);
		skeleton = new (__FILE__, __LINE__) spine::Skeleton(skeletonData);
		tempUvs.ensureCapacity(16);
		tempColors.ensureCapacity(16);

		ownsAnimationStateData = stateData == 0;
		if (ownsAnimationStateData) stateData = new (__FILE__, __LINE__) spine::AnimationStateData(skeletonData);

		state = new (__FILE__, __LINE__) spine::AnimationState(stateData);

		quadIndices.add(0);
		quadIndices.add(1);
		quadIndices.add(2);
		quadIndices.add(2);
		quadIndices.add(3);
		quadIndices.add(0);
	}

	inline SpineSkeleton::~SpineSkeleton() {
		if (ownsAnimationStateData) delete state->getData();
		delete state;
		delete skeleton;
	}

	inline void SpineSkeleton::Update(float delta) {
		state->update(delta * timeScale);
		state->apply(*skeleton);
		skeleton->updateWorldTransform();
	}

	inline void SpineSkeleton::Draw() {

		auto& eg = EngineBase1::Instance();
		auto&& shader = eg.ShaderBegin(eg.shaderSpine38);

		// Early out if skeleton is invisible
		if (skeleton->getColor().a == 0) return;

		RGBA8 c{};
		GLTexture* tex{};

		for (size_t i = 0, e = skeleton->getSlots().size(); i < e; ++i) {

			auto&& slot = *skeleton->getDrawOrder()[i];
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

			auto&& skc = skeleton->getColor();
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
				vertex.pos.x = (*vertices)[index];
				vertex.pos.y = (*vertices)[index + 1];
				vertex.uv.x = (*uvs)[index] * size.x;
				vertex.uv.y = (*uvs)[index + 1] * size.y;
				(uint32_t&)vertex.color = (uint32_t&)c;
				//xx::CoutN("{ .pos = {", vertex.pos, " }, .uv = { ", vertex.uv, "}, .color = xx::RGBA8_White }");
			}

			clipper.clipEnd(slot);
		}

		clipper.clipEnd();
	}


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
		page.setRendererObject(tex.pointer);
	}

	inline void SpineTextureLoader::unload(void* texture) {
		// do nothing. because texture from cache
	}


	inline void SpineListener::callback(spine::AnimationState* state, spine::EventType type, spine::TrackEntry* entry, spine::Event* e) {
		h(state, type, entry, e);
	}


	inline void SpineEnv::Init() {
		spine::SpineExtension::setInstance(&ext);
	}

	inline spine::Atlas* SpineEnv::AddAtlas(std::string_view atlasFileName) {
		auto& fn = tmpStr;
		fn._buffer = (char*)atlasFileName.data();
		fn._length = atlasFileName.length();
		auto r = atlass.emplace(std::string(atlasFileName), std::make_unique<spine::Atlas>(fn, &gSpineEnv.textureLoader));
		fn._buffer = nullptr;
		fn._length = 0;
		if (!r.second) return nullptr;
		return r.first->second.get();
	}

	template<bool skeletonFileIsJson>
	inline spine::SkeletonData* SpineEnv::AddSkeletonData(spine::Atlas* atlas, std::string_view skeletonFileName, float scale) {
		auto& fn = tmpStr;
		fn._buffer = (char*)skeletonFileName.data();
		fn._length = skeletonFileName.length();
		std::conditional_t<skeletonFileIsJson, spine::SkeletonJson, spine::SkeletonBinary> parser(atlas);
		parser.setScale(scale);
		auto r = skeletonDatas.emplace(std::string(skeletonFileName), parser.readSkeletonDataFile(fn));
		fn._buffer = nullptr;
		fn._length = 0;
		if (!r.second) return nullptr;
		return r.first->second.get();
	}


	inline SpinePlayer::SpinePlayer(spine::SkeletonData* skeletonData)
		: animationStateData(skeletonData)
		, spineSkeleton(skeletonData, &animationStateData) 
	{
		spineSkeleton.skeleton->setToSetupPose();
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetMix(std::string_view fromName, std::string_view toName, float duration) {
		auto& s1 = gSpineEnv.tmpStr;
		s1._buffer = (char*)fromName.data();
		s1._length = fromName.length();
		auto& s2 = gSpineEnv.tmpStr2;
		s2._buffer = (char*)toName.data();
		s2._length = toName.length();
		animationStateData.setMix(s1, s2, duration);
		s1._buffer = nullptr;
		s1._length = 0;
		s2._buffer = nullptr;
		s2._length = 0;
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetTimeScale(float t) {
		spineSkeleton.timeScale = t;
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetUsePremultipliedAlpha(bool b) {
		spineSkeleton.usePremultipliedAlpha = b;
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::SetPosition(float x, float y) {
		spineSkeleton.skeleton->setPosition(x, -y);
		spineSkeleton.skeleton->updateWorldTransform();
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::AddAnimation(size_t trackIndex, std::string_view animationName, bool loop, float delay) {
		auto& s1 = gSpineEnv.tmpStr;
		s1._buffer = (char*)animationName.data();
		s1._length = animationName.length();
		spineSkeleton.state->addAnimation(trackIndex, s1, loop, delay);
		s1._buffer = nullptr;
		s1._length = 0;
		return *this;
	}

	XX_INLINE SpinePlayer& SpinePlayer::Update(float delta) {
		spineSkeleton.Update(delta);
		return *this;
	}

	XX_INLINE void SpinePlayer::Draw() {
		spineSkeleton.Draw();
	}
}
