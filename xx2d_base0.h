﻿#pragma once
#include <xx2d_includes.h>
#include <xx2d_gdesign.h>
#include <xx2d_curvemovepath.h>
#ifndef DISABLE_ENGINE_AUDIO
#include <xx2d_audio.h>
#endif

namespace xx {

    enum class MouseEvents : int {
        Unknown = 0, Down, Move, Up, Cancel
    };

    struct Mouse {
        XY pos{};
        std::array<bool, 16> btnStates{};

        bool Pressed(int32_t n) const {
            return btnStates[n];
        }

        bool PressedMBLeft() const {
            return btnStates[0];
        }

        bool PressedMBRight() const {
#ifdef __EMSCRIPTEN__
            return btnStates[2];
#else
            return btnStates[1];
#endif
        }

        //MouseEvents event{};
        //void* eventHandler{};
        //void ClearEvent() { event = MouseEvents::Unknown; }
        //void ClearEventAndSetHandler(void* h) { event = MouseEvents::Unknown; eventHandler = h; }
    };

    struct MouseEventHandlerNode;

    struct Shader;
    struct EngineBase0 {
        Tasks tasks;
        Task<> drawTask;
        Rnd rnd;

#ifndef DISABLE_ENGINE_AUDIO
        // audio
        Audio audio;
#endif

        static constexpr XY anchor7{ 0, 1   }, anchor8{ 0.5, 1   }, anchor9{ 1, 1   };
        static constexpr XY anchor4{ 0, 0.5 }, anchor5{ 0.5, 0.5 }, anchor6{ 1, 0.5 };
        static constexpr XY anchor1{ 0, 0   }, anchor2{ 0.5, 0   }, anchor3{ 1, 0   };

        // fill after Init()
        float framePerSeconds{}, maxFrameDelay{ 0.1f };
        XY windowSize{}, windowSize_2{};
        XY worldMinXY{}, worldMaxXY{}, worldSize{};             // for node easy access
        XY pos7{}, pos8{}, pos9{};
        XY pos4{}, pos5{}, pos6{};
        XY pos1{}, pos2{}, pos3{};
        void SetWindowSize(float w, float h) {
            if (w < 1.f) w = 1.f;
            if (h < 1.f) h = 1.f;
            windowSize = { w, h };
            windowSize_2 = windowSize / 2;
            worldSize = windowSize;
            worldMinXY = -windowSize_2;
            worldMaxXY = windowSize_2;
            pos7 = { -windowSize_2.x, +windowSize_2.y }; pos8 = { 0, +windowSize_2.y }; pos9 = { +windowSize_2.x, +windowSize_2.y };
            pos4 = { -windowSize_2.x, 0               }; pos5 = { 0, 0               }; pos6 = { +windowSize_2.x, 0               };
            pos1 = { -windowSize_2.x, -windowSize_2.y }; pos2 = { 0, -windowSize_2.y }; pos3 = { +windowSize_2.x, -windowSize_2.y };
        }

        RGBA8 clearColor{};
        float flipY{ 1 };   // -1: flip  for ogl frame buffer
        Shader* shader{};

        // events
        Weak<MouseEventHandlerNode> mouseEventHandler{};	// current( focus )
        SpaceGridAB2<MouseEventHandlerNode, XY> mouseEventHandlers;	// mouse down event receivers

        std::array<bool, 512> keyboardKeysStates{};
        std::array<double, 512> keyboardKeysDelays{};

        Mouse mouse;
        bool touchMode{};

        double nowSecs{}, delta{};
        double timePool{};
        int frameNumber{};
        float time{};               // low precision
        EM_BOOL running{ EM_TRUE };


        /**********************************************************************************/
        // file system

        std::string rootPath;
        std::vector<std::string> searchPaths;
        std::filesystem::path tmpPath;

        // convert dir to search path format
        inline static std::string ToSearchPath(std::string_view dir) {
            std::string s;

            dir = Trim(dir);
            if (dir.empty()) {
                CoutN("dir is empty");
                xx_assert(false);
            }

            // replace all \ to /, .\ or ./ to empty
            for (size_t i = 0, siz = dir.size(); i < siz; i++) {
                if (dir[i] == '.' && (dir[i + 1] == '\\' || dir[i + 1] == '/')) {
                    ++i;
                    continue;
                } else if (dir[i] == '\\') {
                    s.push_back('/');
                } else {
                    s.push_back(dir[i]);
                }
            }
            if (s.empty()) {
                CoutN("dir is empty");
                xx_assert(false);
            }

            // make sure / at the end
            if (s.back() != '/') {
                s.push_back('/');
            }

            return s;
        }

        inline static bool IsAbsolute(std::string_view const& s) {
            return s[0] == '/' || (s.size() > 1 && s[1] == ':');
        }

        // add relative base dir to searchPaths
        void SearchPathAdd(std::string_view dir, bool insertToFront = false) {
            auto s = ToSearchPath(dir);
            if (!IsAbsolute(s)) {
                s.insert(0, rootPath);
            }

            if (insertToFront) {
                searchPaths.insert(searchPaths.begin(), std::move(s));
            } else {
                searchPaths.push_back(std::move(s));
            }
        }

        // search paths revert to default
        void SearchPathReset() {
            searchPaths.clear();
            searchPaths.push_back(rootPath);
        }

        // search file by searchPaths + fn. not found return ""
        std::string GetFullPath(std::string_view fn, bool fnIsFileName = true) {
            // prepare
            fn = Trim(fn);

            // is absolute path?
            if (IsAbsolute(fn))
                return std::string(fn);

            // foreach search path find
            for (size_t i = 0, e = searchPaths.size(); i < e; ++i) {
                tmpPath = (std::u8string&)searchPaths[i];
                tmpPath /= (std::u8string_view&)fn;
                if (std::filesystem::exists(tmpPath)) {
                    if (fnIsFileName) {
                        if (std::filesystem::is_regular_file(tmpPath)) goto LabReturn;
                    } else {
                        if (std::filesystem::is_directory(tmpPath)) goto LabReturn;
                    }
                }
                continue;
            LabReturn:
                return U8AsString(tmpPath.u8string());
            }
            // not found
            return {};
        }

        // read all data by full path
        template<bool autoDecompress = false>
        Data LoadFileDataWithFullPath(std::string_view const& fp) {
            Data d;
            if (int r = ReadAllBytes((std::u8string_view&)fp, d)) {
                CoutN("file read error. r = ", r, ", fn = ", fp);
                xx_assert(false);
            }
            if (d.len == 0) {
                CoutN("file content is empty. fn = ", fp);
                xx_assert(false);
            }
            if constexpr (autoDecompress) {
                if (d.len >= 4 && d[0] == 0x28 && d[1] == 0xB5 && d[2] == 0x2F && d[3] == 0xFD) {	// zstd
                    Data d2;
                    ZstdDecompress(d, d2);
                    return d2;
                }
            }
            return d;
        }

        // read all data by GetFullPath( fn )
        template<bool returnFullPath = true, bool autoDecompress = false>
        auto LoadFileData(std::string_view const& fn) {
            auto p = GetFullPath(fn);
            if (p.empty()) {
                CoutN("fn can't find: ", fn);
                xx_assert(false);
            }
            auto d = LoadFileDataWithFullPath<autoDecompress>(p);
            if constexpr (returnFullPath) {
                return std::pair<Data, std::string>{ std::move(d), std::move(p) };
            } else {
                return std::move(d);
            }
        }

        // detect file format by content header
        SupportedFileFormats DetectFileFormat(Data_r const& d) {
            std::string_view buf(d);
            if (buf.starts_with("\x1a\x45\xdf\xa3"sv)) {
                return SupportedFileFormats::Webm;
            } else if (buf.starts_with("XXMV 1.0"sv)) {
                return SupportedFileFormats::Xxmv;
            } else if (buf.starts_with("PKM 20"sv)) {
                return SupportedFileFormats::Pkm2;
            } else if (buf.starts_with("\x13\xab\xa1\x5c"sv)) {
                return SupportedFileFormats::Astc;
            } else if (buf.starts_with("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"sv)) {
                return SupportedFileFormats::Png;
            } else if (buf.starts_with("\xFF\xD8"sv)) {
                return SupportedFileFormats::Jpg;
                // ... more ?
            } else if (buf.starts_with("\x28\xB5\x2F\xFD"sv)) {
                return SupportedFileFormats::Zstd;
            } else {
                return SupportedFileFormats::Unknown;
            }
        }

        EngineBase0();
    };

    inline EngineBase0* gEngine{};

    inline EngineBase0::EngineBase0() {
        gEngine = this;
    }


}
