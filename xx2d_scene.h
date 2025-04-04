﻿#pragma once
#include <xx2d_includes.h>

namespace xx {

	struct alignas(8) SceneBase {
		xx::Tasks tasks;
		virtual void Init() {};			// enter: gLooper.scene == old
		virtual ~SceneBase() {}				//  exit: gLooper.scene == new
		virtual void BeforeUpdate() {};
		virtual void Update() {};
		virtual void Draw() {};
	};

#ifdef ENABLE_ENGINE_IMGUI

	struct ImGuiSceneBase : SceneBase {
		virtual void ImGuiUpdate() = 0;
		virtual void Init() override {
			EngineBase1::Instance().imguiUpdate = [this] { this->ImGuiUpdate(); };
		}
		virtual ~ImGuiSceneBase() override {
			EngineBase1::Instance().imguiUpdate = {};
		}
	};

#endif

}
