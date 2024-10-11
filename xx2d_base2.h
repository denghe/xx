#pragma once
#include <xx2d_base1.h>
#include <xx2d_chartexcache.h>
#include <xx2d_tiledmap_sede.h>
#include <xx2d_fpsviewer.h>
#include <xx2d_quad.h>
#include <xx2d_repeat_quad.h>
#include <xx2d_linestrip.h>
#include <xx2d_dynamictexturepacker.h>
#include <xx2d_scale9sprite.h>
#include <xx2d_camera.h>
#include <xx2d_scene.h>
#include <xx2d_scissor.h>

#ifndef __EMSCRIPTEN__
#include <xx2d_bitmapdc.h>
#endif

namespace xx {

    template<typename T> concept Has_AsyncInit = requires(T t) { { t.AsyncInit() } -> std::same_as<Task<>>; };

    struct EngineBase2 : EngineBase1 {
        XX_INLINE static EngineBase2& Instance() { return *(EngineBase2*)gEngine; }

        // default font cache
        CharTexCache<24> ctcDefault;

        FpsViewer fpsViewer;
        bool showFps{ true };

        // current scene
        Shared<SceneBase> scene;

        // scene utils
        template<std::derived_from<SceneBase> T>
        Task<> AsyncSwitchTo() {
            auto newScene = MakeShared<T>();
            if constexpr (Has_AsyncInit<T>) {
                co_await newScene->AsyncInit();
            }
            newScene->Init();
            scene.Reset();
            scene = std::move(newScene);
            co_return;
        }

        template<std::derived_from<SceneBase> T>
        void DelaySwitchTo() {
            tasks.Add(AsyncSwitchTo<T>());
        }

        // default Update & Draw for scene
        void BeforeUpdate() {
            if (!scene) return;
            scene->BeforeUpdate();
        }

        void Update() {
            if (!scene) return;
            scene->Update();
            scene->tasks();
        }

        void Draw() {
            if (!scene) return;
            scene->Draw();
        }


        // other utils
        std::optional<std::pair<MoveDirections, XY>> GetKeyboardMoveInc() {
            union Dirty {
                struct {
                    union {
                        struct {
                            uint8_t a, d;
                        };
                        uint16_t ad;
                    };
                    union {
                        struct {
                            uint8_t s, w;
                        };
                        uint16_t sw;
                    };
                };
                uint32_t all{};
            } flags;
            int32_t n = 0;

            if (KeyDown(KeyboardKeys::A)) { flags.a = 255; ++n; }
            if (KeyDown(KeyboardKeys::S)) { flags.s = 255; ++n; }
            if (KeyDown(KeyboardKeys::D)) { flags.d = 255; ++n; }
            if (KeyDown(KeyboardKeys::W)) { flags.w = 255; ++n; }

            if (n > 2) {
                if (flags.ad > 255 << 8) {
                    flags.ad = 0;
                    n -= 2;
                }
                if (flags.sw > 255 << 8) {
                    flags.sw = 0;
                    n -= 2;
                }
            }
            if (!n) return {};

            XY v{};
            MoveDirections md;
            constexpr const float SQR2_2 = 0.70710677f;

            if (n == 2) {
                if (flags.w) {
                    md = MoveDirections::Up;
                    if (flags.d) {
                        (int&)md |= (int)MoveDirections::Right;
                        v = { SQR2_2, -SQR2_2 };	// up right
                    } else if (flags.a) {
                        (int&)md |= (int)MoveDirections::Left;
                        v = { -SQR2_2, -SQR2_2 };	// up left
                    }
                } else if (flags.s) {
                    md = MoveDirections::Down;
                    if (flags.d) {
                        (int&)md |= (int)MoveDirections::Right;
                        v = { SQR2_2, SQR2_2 };		// right down
                    } else if (flags.a) {
                        (int&)md |= (int)MoveDirections::Left;
                        v = { -SQR2_2, SQR2_2 };	// left down
                    }
                }
            } else if (n == 1) {
                if (flags.w) {
                    md = MoveDirections::Up;
                    v.y = -1;	// up
                } else if (flags.s) {
                    md = MoveDirections::Down;
                    v.y = 1;	// down
                } else if (flags.a) {
                    md = MoveDirections::Left;
                    v.x = -1;	// left
                } else if (flags.d) {
                    md = MoveDirections::Right;
                    v.x = 1;	// right
                }
            }

            return std::make_pair(md, v);
        }


    };

}
