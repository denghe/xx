#pragma once
#include "xx2d.h"
#include <spine/spine.h>

namespace spine {
	SpineExtension* getDefaultExtension();
}

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
		SpineListener(HandlerType h) : h(std::move(h)) {}
		void callback(spine::AnimationState* state, spine::EventType type, spine::TrackEntry* entry, spine::Event* e) override;
	};

	// todo: more enhance like rotate ??
	struct SpinePlayer {
		std::shared_ptr<spine::Atlas> atlas;	// can shrae?
		std::shared_ptr<spine::SkeletonData> skData;	// can share?

		std::optional<spine::AnimationStateData> asData;
		std::optional<SpineSkeleton> xxSkel;
		std::optional<SpineListener> xxListener;

		SpinePlayer& Init(SpinePlayer const& sharedRes);
		SpinePlayer& Init1(const spine::String& atlasName);
		SpinePlayer& Init2_Json(const spine::String& filename, float scale);
		SpinePlayer& Init2_Skel(const spine::String& filename, float scale);
		SpinePlayer& Init3();

		SpinePlayer& SetMix(const spine::String& fromName, const spine::String& toName, float duration);
		SpinePlayer& SetTimeScale(float t);
		SpinePlayer& SetUsePremultipliedAlpha(bool b);
		SpinePlayer& SetPosition(float x, float y);
		SpinePlayer& SetListener(SpineListener::HandlerType h);
		SpinePlayer& AddAnimation(size_t trackIndex, const spine::String& animationName, bool loop, float delay);
		SpinePlayer& Update(float delta);
		void Draw();
	};

	struct SpineExtension : spine::DefaultSpineExtension {
	protected:
		char* _readFile(const spine::String& pathStr, int* length) override;
	};


	struct string_hash {
		using is_transparent = void;
		[[nodiscard]] size_t operator()(const char* txt) const {
			return std::hash<std::string_view>{}(txt);
		}
		[[nodiscard]] size_t operator()(std::string_view txt) const {
			return std::hash<std::string_view>{}(txt);
		}
		[[nodiscard]] size_t operator()(const std::string& txt) const {
			return std::hash<std::string>{}(txt);
		}
	};

	// spine's global env
	struct SpineEnv {
		// need preload. for load( page, path ) search. path is key
		std::unordered_map<std::string, Ref<GLTexture>, string_hash, std::equal_to<>> texs;
		std::unordered_map<std::string, xx::Data, string_hash, std::equal_to<>> fileDatas;

		SpineTextureLoader textureLoader;
	};
	inline SpineEnv gSpineEnv;

	/****************************************************************************************************************/
	/****************************************************************************************************************/

	// normal, additive, multiply, screen
	inline static const constexpr std::array<std::pair<uint32_t, uint32_t>, 4> nonpmaBlendFuncs{ std::pair<uint32_t, uint32_t>
	{ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA }, { GL_SRC_ALPHA, GL_ONE }, { GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA }, { GL_ONE, GL_ONE_MINUS_SRC_COLOR } };

	// pma: normal, additive, multiply, screen
	inline static const constexpr std::array<std::pair<uint32_t, uint32_t>, 4> pmaBlendFuncs{ std::pair<uint32_t, uint32_t>
	{ GL_ONE, GL_ONE_MINUS_SRC_ALPHA }, { GL_ONE, GL_ONE }, { GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA }, { GL_ONE, GL_ONE_MINUS_SRC_COLOR } };

	char* SpineExtension::_readFile(const spine::String& pathStr, int* length) {
		std::string_view fn(pathStr.buffer(), pathStr.length());
		auto&& iter = gSpineEnv.fileDatas.find(fn);
		*length = iter->second.len;
		return (char*)iter->second.buf;
	}


	SpineSkeleton::SpineSkeleton(spine::SkeletonData* skeletonData, spine::AnimationStateData* stateData) {

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

	SpineSkeleton::~SpineSkeleton() {
		if (ownsAnimationStateData) delete state->getData();
		delete state;
		delete skeleton;
	}

	void SpineSkeleton::Update(float delta) {
		state->update(delta * timeScale);
		state->apply(*skeleton);
		skeleton->updateWorldTransform();
	}

	void SpineSkeleton::Draw() {

		auto& eg = EngineBase1::Instance();
		auto&& shader = eg.ShaderBegin(eg.shaderSpine38);

		// Early out if skeleton is invisible
		if (skeleton->getColor().a == 0) return;

		xx::RGBA8 c{};
		spine::AtlasPage* page{};

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
			int indicesCount{};
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
				page = ((spine::AtlasRegion*)regionAttachment)->page;

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
				page = ((spine::AtlasRegion*)mesh)->page;

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

			auto vp = shader.Draw(page->textureId, indicesCount);
			xx::XY size{ (float)page->width, (float)page->height };
			for (size_t ii = 0; ii < indicesCount; ++ii) {
				auto index = (*indices)[ii] << 1;
				auto&& vertex = vp[ii];
				vertex.pos.x = (*vertices)[index];
				vertex.pos.y = (*vertices)[index + 1];
				vertex.uv.u = (*uvs)[index] * size.x;
				vertex.uv.v = (*uvs)[index + 1] * size.y;
				(uint32_t&)vertex.color = (uint32_t&)c;
			}

			clipper.clipEnd(slot);
		}

		clipper.clipEnd();
	}


	void SpineTextureLoader::load(spine::AtlasPage& page, const spine::String& path) {

		std::string_view fn(path.buffer(), path.length());

		auto&& iter = gSpineEnv.texs.find(fn);
		auto&& tex = iter->second;
		tex->SetGLTexParm(
			page.magFilter == spine::TextureFilter_Linear ? GL_LINEAR : GL_NEAREST,
			(page.uWrap == spine::TextureWrap_Repeat && page.vWrap == spine::TextureWrap_Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE
		);

		page.width = tex->Width();
		page.height = tex->Height();
		page.textureId = tex->GetValue();
		tex.pointer = nullptr;
	}

	void SpineTextureLoader::unload(void* texture) {
		xx::Shared<xx::GLTexture> tex;
		tex.pointer = (xx::GLTexture*)texture;	// move back
	}



	void SpineListener::callback(spine::AnimationState* state, spine::EventType type, spine::TrackEntry* entry, spine::Event* e) {
		h(state, type, entry, e);
	}


	SpinePlayer& SpinePlayer::Init(SpinePlayer const& sharedRes) {
		this->atlas = sharedRes.atlas;
		this->skData = sharedRes.skData;
		return Init3();
	}

	SpinePlayer& SpinePlayer::Init1(const spine::String& atlasName) {
		atlas.reset(new spine::Atlas(atlasName, &gSpineEnv.textureLoader));
		return *this;
	}

	SpinePlayer& SpinePlayer::Init2_Json(const spine::String& filename, float scale) {
		spine::SkeletonJson parser(atlas.get());
		parser.setScale(scale);
		skData.reset(parser.readSkeletonDataFile(filename));	// todo: read  buf + len
		if (!skData) throw std::logic_error(xx::ToString("Init2_Json failed. fn = ", filename.buffer()));
		return *this;
	}

	SpinePlayer& SpinePlayer::Init2_Skel(const spine::String& filename, float scale) {
		spine::SkeletonBinary parser(atlas.get());
		parser.setScale(scale);
		skData.reset(parser.readSkeletonDataFile(filename));	// todo: read  buf + len
		if (!skData) throw std::logic_error(xx::ToString("Init2_Skel failed. fn = ", filename.buffer()));
		return *this;
	}

	SpinePlayer& SpinePlayer::Init3() {
		assert(atlas);
		assert(skData);
		asData.emplace(skData.get());
		xxSkel.emplace(skData.get(), &*asData);
		xxSkel->skeleton->setToSetupPose();
		return *this;
	}

	SpinePlayer& SpinePlayer::SetMix(const spine::String& fromName, const spine::String& toName, float duration) {
		asData->setMix(fromName, toName, duration);
		return *this;
	}

	SpinePlayer& SpinePlayer::SetTimeScale(float t) {
		xxSkel->timeScale = t;
		return *this;
	}

	SpinePlayer& SpinePlayer::SetUsePremultipliedAlpha(bool b) {
		xxSkel->usePremultipliedAlpha = b;
		return *this;
	}

	SpinePlayer& SpinePlayer::SetPosition(float x, float y) {
		xxSkel->skeleton->setPosition(x, -y);
		xxSkel->skeleton->updateWorldTransform();
		return *this;
	}

	SpinePlayer& SpinePlayer::SetListener(SpineListener::HandlerType h) {
		xxListener.emplace(std::move(h));
		xxSkel->state->setListener(&*xxListener);
		return *this;
	}

	SpinePlayer& SpinePlayer::AddAnimation(size_t trackIndex, const spine::String& animationName, bool loop, float delay) {
		xxSkel->state->addAnimation(trackIndex, animationName, loop, delay);
		return *this;
	}

	SpinePlayer& SpinePlayer::Update(float delta) {
		xxSkel->Update(delta);
		return *this;
	}

	void SpinePlayer::Draw() {
		xxSkel->Draw();
	}
}

namespace spine {
	SpineExtension* getDefaultExtension() {
		return new ::xx::SpineExtension();
	}
}
