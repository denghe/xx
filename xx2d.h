#pragma once
#ifdef __EMSCRIPTEN__
#include <xx2d_engine.h>
#else
#include <xx2d_engine_desktop.h>
#endif


/*****************************************************************/
/*****************************************************************/
// pch.h example:
/*


#pragma once
#ifndef XX2D_PROJECT_PCH_H_
#define XX2D_PROJECT_PCH_H_

#include <xx2d.h>

#endif


*/


/*****************************************************************/
/*****************************************************************/
// pch.cpp example:
/*


#include <pch.h>
#include <xx2d__cpp.inc>


*/

/*****************************************************************/
/*****************************************************************/
// game_looper.h example:
/*

#pragma once
#include "pch.h"

constexpr GDesign<1280, 720, 60> gDesign;

struct GameLooper : Engine<GameLooper>, decltype(gDesign) {
	void Update();
	xx::Task<> MainTask();
	void Draw();
};
extern GameLooper gLooper;


*/

/*****************************************************************/
/*****************************************************************/
// game_looper.cpp example:
/*


#include "pch.h"
#include "game_looper.h"

GameLooper gLooper;

#ifdef __EMSCRIPTEN__
int32_t main() {
	gLooper.EngineInit();
	emscripten_request_animation_frame_loop([](double ms, void*)->EM_BOOL {
		return gLooper.JsLoopCallback(ms);
	}, nullptr);
}
#else
int32_t main() {
	gLooper.showFps = true;
	gLooper.title = "xx2d_project";
	gLooper.Init();
	gLooper.Run();
}
#endif

// Update   MainTask   Draw ...

*/
