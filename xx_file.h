﻿#pragma once
#include <xx_data.h>

namespace xx
{
	inline int ReadAllBytes(std::filesystem::path const& path, Data& d) noexcept {
		std::ifstream f(path, std::ifstream::binary);
		if (!f) return -1;						// not found? no permission? locked?
		auto sg = MakeScopeGuard([&] { f.close(); });
		f.seekg(0, f.end);
		auto&& siz = f.tellg();
		if ((uint64_t)siz > std::numeric_limits<size_t>::max()) return -2;	// too big
		f.seekg(0, f.beg);
		d.Clear();
		d.Reserve<true, false>(siz);
		f.read((char*)d.buf, siz);
		if (!f) return -3;						// only f.gcount() could be read
		d.len = siz;
		return 0;
	}

	inline std::pair<std::unique_ptr<uint8_t[]>, size_t> ReadAllBytes(std::filesystem::path const& path) {
		std::ifstream f(path, std::ifstream::binary);
		if (!f) return {};													// not found? no permission? locked?
        auto sg = MakeScopeGuard([&] { f.close(); });
		f.seekg(0, f.end);
		auto&& siz = f.tellg();
		if ((uint64_t)siz > std::numeric_limits<size_t>::max()) return {};	// too big
		f.seekg(0, f.beg);
		auto outBuf = std::make_unique_for_overwrite<uint8_t[]>(siz);		// maybe throw oom
		f.read((char*)outBuf.get(), siz);
		if (!f) return {};													// only f.gcount() could be read
		return { std::move(outBuf), siz };
	}

	inline int WriteAllBytes(std::filesystem::path const& path, char const* buf, size_t len) noexcept {
		std::ofstream f(path, std::ios::binary | std::ios::trunc);
		if (!f) return -1;						// no create permission? exists readonly?
        auto sg = MakeScopeGuard([&] { f.close(); });
		f.write(buf, len);
		if (!f) return -2;						// write error
		return 0;
	}

	inline int WriteAllBytes(std::filesystem::path const& path, Data const& bb) noexcept {
		return WriteAllBytes(path, (char*)bb.buf, bb.len);
	}

	template<size_t len>
	inline int WriteAllBytes(std::filesystem::path const& path, char const(&s)[len]) noexcept {
		return WriteAllBytes(path, s, len - 1);
	}

	inline std::filesystem::path GetPath_Current() {
		return std::filesystem::absolute("./");
	}

	inline std::filesystem::path GetPath_ProgramData() {
#ifdef _WIN32
		PWSTR path_tmp;
		auto r = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &path_tmp);
		auto sg = xx::MakeScopeGuard([&] { CoTaskMemFree(path_tmp); });
		if (r != S_OK) return {};
		return path_tmp;
#else
		return GetPath_Current();
#endif
	}

#ifdef _WIN32
	inline void ExecuteFile(LPCTSTR lpApplicationName) {
		// additional information
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// start the program up
		CreateProcess(lpApplicationName,   // the path
			nullptr,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);
		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
#endif

	// todo: more

	//std::cout << std::filesystem::current_path() << std::endl;
	//for (auto&& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path())) {
	//	std::cout << entry << std::endl;
	//	std::cout << std::filesystem::absolute(entry) << std::endl;
	//	std::cout << entry.is_regular_file() << std::endl;
	//}
	//try {
	//	auto siz = std::filesystem::file_size(L"中文文件名.txt");
	//	std::cout << siz << std::endl;
	//}
	//catch (std::filesystem::filesystem_error& ex) {
	//	std::cout << ex.what() << std::endl;
	//}

}
