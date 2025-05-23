﻿#pragma once
#include <xx2d_base2.h>
#include <xx2d_label.h>
#include <xx2d_richlabel.h>
#include <xx2d_iconbutton.h>
#include <xx2d_scrollview.h>

namespace xx {

    template<typename T> concept Has_Init = requires(T t) { { t.Init() } -> std::same_as<void>; };
    template<typename T> concept Has_AfterInit = requires(T t) { { t.AfterInit() } -> std::same_as<void>; };
    template<typename T> concept Has_BeforeUpdate = requires(T t) { { t.BeforeUpdate() } -> std::same_as<void>; };
    template<typename T> concept Has_Update = requires(T t) { { t.Update() } -> std::same_as<void>; };
    template<typename T> concept Has_Draw = requires(T t) { { t.Draw() } -> std::same_as<void>; };
    template <typename T> concept Has_MainTask = requires(T t) { { t.MainTask() } -> std::same_as<Task<>>; };
    template <typename T> concept Has_DrawTask = requires(T t) { { t.DrawTask() } -> std::same_as<Task<>>; };

    template <typename T> concept Has_OnKeyPress = requires(T t) { { t.OnKeyPress(std::declval<EmscriptenKeyboardEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnKeyDown = requires(T t) { { t.OnKeyDown(std::declval<EmscriptenKeyboardEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnKeyUp = requires(T t) { { t.OnKeyUp(std::declval<EmscriptenKeyboardEvent const&>()) } -> std::same_as<EM_BOOL>; };

    template <typename T> concept Has_OnMouseDown = requires(T t) { { t.OnMouseDown(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnMouseUp = requires(T t) { { t.OnMouseUp(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnClick = requires(T t) { { t.OnClick(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnDblClick = requires(T t) { { t.OnDblClick(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnMouseMove = requires(T t) { { t.OnMouseMove(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnMouseEnter = requires(T t) { { t.OnMouseEnter(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnMouseLeave = requires(T t) { { t.OnMouseLeave(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnMouseOver = requires(T t) { { t.OnMouseOver(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnMouseOut = requires(T t) { { t.OnMouseOut(std::declval<EmscriptenMouseEvent const&>()) } -> std::same_as<EM_BOOL>; };

    template <typename T> concept Has_OnTouchStart = requires(T t) { { t.OnTouchStart(std::declval<EmscriptenTouchEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnTouchMove = requires(T t) { { t.OnTouchMove(std::declval<EmscriptenTouchEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnTouchEnd = requires(T t) { { t.OnTouchEnd(std::declval<EmscriptenTouchEvent const&>()) } -> std::same_as<EM_BOOL>; };
    template <typename T> concept Has_OnTouchCancel = requires(T t) { { t.OnTouchCancel(std::declval<EmscriptenTouchEvent const&>()) } -> std::same_as<EM_BOOL>; };

    struct EngineBase3 : EngineBase2 {
        XX_INLINE static EngineBase3& Instance() { return *(EngineBase3*)gEngine; }

        // task utils
        Task<> AsyncSleep(double secs) {
            auto e = nowSecs + secs;
            do {
                co_yield 0;
            } while (nowSecs < e);
        }

#ifdef __EMSCRIPTEN__

        template<bool showLog = false, int timeoutSeconds = 30>
        Task<Ref<GLTexture>> AsyncLoadTextureFromUrl(std::string_view url_) {
            std::string url__(url_);
            auto url = url__.c_str();
            if constexpr (showLog) {
                printf("LoadTextureFromUrl( %s ) : begin. nowSecs = %f\n", url, nowSecs);
            }
            auto i = GLGenTextures<true>();
            int tw{}, th{};
            load_texture_from_url(i, url, &tw, &th);
            auto elapsedSecs = nowSecs + timeoutSeconds;
            while (nowSecs < elapsedSecs) {
                co_yield 0;
                if (tw) {
                    if constexpr (showLog) {
                        printf("LoadTextureFromUrl( %s ) : loaded. nowSecs = %f, size = %d, %d\n", url, nowSecs, tw, th);
                    }
                    co_return MakeRef<GLTexture>(i, tw, th, url);
                }
            }
            if constexpr (showLog) {
                printf("LoadTextureFromUrl( %s ) : timeout. timeoutSeconds = %d\n", url, timeoutSeconds);
            }
            co_return Ref<GLTexture>{};
        }

        template<bool showLog = false, int timeoutSeconds = 30>
        Task<std::vector<Ref<GLTexture>>> AsyncLoadTexturesFromUrls(std::initializer_list<std::string_view> urls) {
            std::vector<Ref<GLTexture>> rtv;
            rtv.resize(urls.size());
            size_t counter = 0;
            for (size_t i = 0; i < urls.size(); ++i) {
                tasks.Add([this, &counter, tar = &rtv[i], url = *(urls.begin() + i)]()->Task<> {
                    *tar = co_await AsyncLoadTextureFromUrl<showLog, timeoutSeconds>(url);
                    ++counter;
                    });
            }
            while (counter < urls.size()) co_yield 0; // wait all
            co_return rtv;
        }

        template<bool showLog = false, int timeoutSeconds = 30>
        Task<Ref<Frame>> AsyncLoadFrameFromUrl(std::string_view url) {
            co_return Frame::Create(co_await AsyncLoadTextureFromUrl<showLog, timeoutSeconds>(url));
        }

        // todo: timeout support
        template<bool autoDecompress = false>
        Task<Data> AsyncDownloadFromUrl(std::string url) {
            emscripten_fetch_attr_t attr;
            emscripten_fetch_attr_init(&attr);
            strcpy(attr.requestMethod, "GET");
            attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

            using UD = std::pair<bool*, Data*>;

            attr.onsuccess = [](emscripten_fetch_t* fetch) {
                UD& ud = *(UD*)fetch->userData;
                *ud.first = true;
                ud.second->WriteBuf(fetch->data, fetch->numBytes);
                emscripten_fetch_close(fetch);
                };

            attr.onerror = [](emscripten_fetch_t* fetch) {
                UD& ud = *(UD*)fetch->userData;
                *ud.first = true;
                emscripten_fetch_close(fetch);
                };

            Data sd;
            bool callbacked = false;
            UD ud = { &callbacked, &sd };
            attr.userData = &ud;

            emscripten_fetch(&attr, url.c_str());

            while (!callbacked) {
                co_yield 0;
            }

            if constexpr (autoDecompress) {
                TryZstdDecompress(sd);
            }
            co_return std::move(sd);
        }

        // blist == texture packer export cocos plist file's bin version, use xx2d's tools: plist 2 blist convert
        template<bool autoDecompress = false>
        Task<Ref<TexturePacker>> AsyncLoadTexturePackerFromUrl(std::string blistUrl) {
            auto blistData = co_await AsyncDownloadFromUrl<autoDecompress>(blistUrl);
            if (!blistData.len) co_return Ref<TexturePacker>{};

            auto tp = MakeRef<TexturePacker>();
            int r = tp->Load(blistData, blistUrl);
            xx_assert(!r);

            auto tex = co_await AsyncLoadTextureFromUrl<autoDecompress>(tp->realTextureFileName.c_str());
            xx_assert(tex);

            for (auto& f : tp->frames) {
                f->tex = tex;
            }
            co_return tp;
        }

        // bmx == tiledmap editor store tmx file's bin version, use xx2d's tools: tmx 2 bmx convert
        template<bool autoDecompress = false>
        Task<Ref<TMX::Map>> AsyncLoadTiledMapFromUrl(std::string bmxUrl, std::string root = "", bool loadTextures = false, bool fillExts = true) {
            auto map = MakeRef<TMX::Map>();
            std::string fullPath;
            // download bmx & fill
            {
                auto sd = co_await AsyncDownloadFromUrl<autoDecompress>(bmxUrl);
                xx_assert(sd.len);

                TmxData td;
                td = std::move(sd);
                auto r = td.Read(*map);
                xx_assert(!r);
            }

            if (root.empty()) {
                std::string_view sv(bmxUrl);
                if (auto&& i = sv.find_last_of("/"); i != sv.npos) {
                    root = sv.substr(0, i + 1);
                }
            }

            // download textures
            if (loadTextures) {
                auto n = map->images.size();
                for (auto& img : map->images) {
                    tasks.Add([this, &n, img = img, url = root + img->source]()->Task<> {
                        img->texture = co_await AsyncLoadTextureFromUrl(url.c_str());
                        --n;
                        });
                }
                while (n) co_yield 0;	// wait all
            }

            // fill ext data for easy use
            if (fillExts) {
                map->FillExts();
            }

            co_return map;
        }

#else
        template<bool showLog = false, int timeoutSeconds = 30>
        Task<Ref<GLTexture>> AsyncLoadTextureFromUrl(std::string_view url_) {
            co_return LoadTexture(url_);
        }

        template<bool autoDecompress = false>
        Task<Data> AsyncDownloadFromUrl(std::string url) {
            co_return LoadFileData<false, autoDecompress>(url);
        }
        // todo: more simulate
#endif
    };

}
