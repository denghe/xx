﻿#pragma once
#include "xx_typetraits.h"
#pragma warning(disable: 4200)

namespace xx {

	template<typename T, typename SizeType = int32_t>
	struct TinyList {
		typedef T ChildType;
		using S = SizeType;
		struct Core {
			SizeType len, cap;
			/*alignas(T)*/ T buf[0];
		} *core{};
		static constexpr bool isBigT = alignof(SizeType) < alignof(T);
		static_assert(!isBigT || sizeof(SizeType) * 2 <= alignof(T));

		TinyList() = default;
		TinyList(TinyList const& o) = delete;
		TinyList& operator=(TinyList const& o) = delete;
		TinyList(TinyList&& o) noexcept : core(std::exchange(o.core, nullptr)) {}
		TinyList& operator=(TinyList&& o) noexcept {
			std::swap(core, o.core);
			return *this;
		}
		~TinyList() noexcept {
			Clear(true);
		}

		bool Empty() const {
			return !core || !core->len;
		}

		SizeType Count() const {
			return !core ? 0 : core->len;
		}

		T* Buf() const {
			return core ? (T*)core->buf : nullptr;
		}

		SizeType Len() const {
			return core ? core->len : 0;
		}

		TinyList Clone() const {
			TinyList rtv;
			rtv.Reserve(core->len);
			rtv.AddRange(*this);
			return rtv;
		}

		// func == [](auto& a, auto& b) { return a->xxx < b->xxx; }
		template<typename F>
		XX_INLINE void StdSort(F&& func) {
			if (!core) return;
			std::sort(core->buf, core->buf + core->len, std::forward<F>(func));
		}

		void Reserve(SizeType cap_) noexcept {
			if (auto newBuf = ReserveBegin(cap_)) {
				ReserveEnd(newBuf);
			}
		}

		Core* ReserveBegin(SizeType cap_) noexcept {
			assert(cap_ > 0);
			if (core && cap_ <= core->cap) return {};
			if (!core) {
				if constexpr (isBigT) {
					core = (Core*)operator new (sizeof(T) * (cap_ + 1), std::align_val_t(alignof(Core)));
				} else {
					core = (Core*)operator new (sizeof(Core) + sizeof(T) * cap_, std::align_val_t(alignof(Core)));
				}
				core->len = 0;
				core->cap = cap_;
				return {};
			}
			auto newCap = core->cap;
			do {
				newCap *= 2;
			} while (newCap < cap_);

			Core* newCore;
			if constexpr (isBigT) {
				newCore = (Core*)operator new (sizeof(T) * (newCap + 1), std::align_val_t(alignof(Core)));
			} else {
				newCore = (Core*)operator new (sizeof(Core) + sizeof(T) * newCap, std::align_val_t(alignof(Core)));
			}
			newCore->len = core->len;
			newCore->cap = core->cap;
			return newCore;
		}

		void ReserveEnd(Core* newCore) noexcept {
			auto& buf = core->buf;
			auto& len = core->len;
			auto& newBuf = newCore->buf;
			if constexpr (IsPod_v<T>) {
				::memcpy((void*)newBuf, (void*)buf, len * sizeof(T));
			} else {
				for (SizeType i = 0; i < len; ++i) {
					std::construct_at(&newBuf[i], std::move(buf[i]));
					std::destroy_at(&buf[i]);
				}
			}
			operator delete (core, std::align_val_t(alignof(Core)));
			core = newCore;
		}

		template<bool fillVal = false, int val = 0>
		void Resize(SizeType len_) noexcept {
			if (!core) {
				if (!len_) return;
				Reserve(len_);
				if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
					for (SizeType i = 0; i < len_; ++i) {
						std::construct_at(&core->buf[i]);
					}
				} else if constexpr (fillVal) {
					memset(core->buf, val, len_ * sizeof(T));
				}
				core->len = len_;
				return;
			}
			if (len_ == core->len) return;
			else if (len_ < core->len) {
				for (SizeType i = len_; i < core->len; ++i) {
					std::destroy_at(&core->buf[i]);
				}
			} else {	// len_ > len
				Reserve(len_);
				if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
					for (SizeType i = core->len; i < len_; ++i) {
						std::construct_at(&core->buf[i]);
					}
				} else if constexpr (fillVal) {
					memset(core->buf + core->len, val, (len_ - core->len) * sizeof(T));
				}
			}
			core->len = len_;
		}

		T const& operator[](SizeType idx) const noexcept {
			assert(core && idx >= 0 && idx < core->len);
			return core->buf[idx];
		}
		T& operator[](SizeType idx) noexcept {
			assert(core && idx >= 0 && idx < core->len);
			return core->buf[idx];
		}
		T const& At(SizeType idx) const noexcept {
			xx_assert(core && idx >= 0 && idx < core->len);
			return core->buf[idx];
		}
		T& At(SizeType idx) noexcept {
			xx_assert(core && idx >= 0 && idx < core->len);
			return core->buf[idx];
		}

		T& Top() noexcept {
			assert(core && core->len > 0);
			return core->buf[core->len - 1];
		}
		void Pop() noexcept {
			assert(core && core->len > 0);
			--core->len;
			std::destroy_at(&core->buf[core->len]);
		}
		T const& Top() const noexcept {
			assert(core && core->len > 0);
			return core->buf[core->len - 1];
		}
		bool TryPop(T& output) noexcept {
			if (!core || !core->len) return false;
			output = std::move(core->buf[--core->len]);
			std::destroy_at(&core->buf[core->len]);
			return true;
		}

		void Clear(bool freeBuf = false) noexcept {
			if (!core) return;
			if (core->len) {
				for (SizeType i = 0, len = core->len; i < len; ++i) {
					std::destroy_at(&core->buf[i]);
				}
				core->len = 0;
			}
			if (freeBuf) {
				operator delete (core, std::align_val_t(alignof(Core)));
				core = {};
			}
		}

		void Remove(T const& v) noexcept {
			assert(core);
			auto& len = core->len;
			auto& buf = core->buf;
			for (SizeType i = 0; i < len; ++i) {
				if (v == buf[i]) {
					RemoveAt(i);
					return;
				}
			}
		}

		void RemoveAt(SizeType idx) {
			assert(core);
			auto& len = core->len;
			auto& buf = core->buf;
			assert(idx >= 0 && idx < len);
			--len;
			std::destroy_at(&buf[idx]);
			if constexpr (IsPod_v<T>) {
				::memmove(buf + idx, buf + idx + 1, (len - idx) * sizeof(T));
			} else {
				for (SizeType i = idx; i < len; ++i) {
					std::construct_at(&buf[i], std::move(buf[i + 1]));
				}
				std::destroy_at(&buf[len]);
			}
		}

		void SwapRemoveAt(SizeType idx) {
			assert(core);
			auto& len = core->len;
			assert(idx >= 0 && idx < len);
			auto& buf = core->buf;
			std::destroy_at(&buf[idx]);
			--len;
			if (len != idx) {
				if constexpr (IsPod_v<T>) {
					::memcpy(&buf[idx], &buf[len], sizeof(T));
				} else {
					std::construct_at(&buf[idx], std::move(buf[len]));
				}
			}
		}

		XX_INLINE void PopBack() {
			assert(core && core->len);
			--core->len;
			if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
				std::destroy_at(&core->buf[core->len]);
			}
		}

		XX_INLINE T& Back() {
			assert(core && core->len);
			return core->buf[core->len - 1];
		}
		XX_INLINE T const& Back() const {
			assert(core && core->len);
			return core->buf[core->len - 1];
		}

		template<typename...Args>
		T& Emplace(Args&&...args) noexcept {
			if (auto newCore = ReserveBegin(core ? core->len + 1 : 4)) {
				auto& newBuf = newCore->buf;
				auto& len = newCore->len;
				std::construct_at(&newBuf[len], std::forward<Args>(args)...);
				ReserveEnd(newCore);
				return newBuf[len++];
			} else {
				return *std::construct_at(&core->buf[core->len++], std::forward<Args>(args)...);
			}
		}

		template<typename ...TS>
		void Add(TS&&...vs) noexcept {
			(Emplace(std::forward<TS>(vs)), ...);
		}

		void AddRange(T const* items, SizeType count) noexcept {
			assert(items && count > 0);
			if (auto newCore = ReserveBegin(core ? core->len + count : count)) {
				auto& newBuf = newCore->buf;
				auto& len = newCore->len;
				if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
					::memcpy(newBuf + len, items, count * sizeof(T));
				} else {
					for (SizeType i = 0; i < count; ++i) {
						std::construct_at(&newBuf[len + i], items[i]);
					}
				}
				ReserveEnd(newBuf);
				len += count;
			} else {
				auto& buf = core->buf;
				auto& len = core->len;
				if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
					::memcpy(buf + len, items, count * sizeof(T));
				} else {
					for (SizeType i = 0; i < count; ++i) {
						std::construct_at(&buf[len + i], items[i]);
					}
				}
				len += count;
			}
		}

		template<typename L>
		void AddRange(L const& list) noexcept {
			return AddRange(list.Buf(), list.Len());
		}

		SizeType Find(T const& v) const noexcept {
			for (SizeType i = 0; i < core->len; ++i) {
				if (v == core->buf[i]) return i;
			}
			return SizeType(-1);
		}

		template<typename Func>
		bool Exists(Func&& cond) const noexcept {
			for (SizeType i = 0; i < core->len; ++i) {
				if (cond(core->buf[i])) return true;
			}
			return false;
		}

		//struct Iter {
		//	T* ptr;
		//	bool operator!=(Iter const& other) noexcept { return ptr != other.ptr; }
		//	Iter& operator++() noexcept { ++ptr; return *this; }
		//	T& operator*() noexcept { return *ptr; }
		//};
		//Iter begin() noexcept { assert(core); return Iter{ core->buf }; }
		//Iter end() noexcept { assert(core); return Iter{ core->buf + core->len }; }
		//Iter begin() const noexcept { assert(core); return Iter{ core->buf }; }
		//Iter end() const noexcept { assert(core); return Iter{ core->buf + core->len }; }

	};

	// mem moveable tag
	template<typename T, typename SizeType>
	struct IsPod<TinyList<T, SizeType>, void> : std::true_type {};

	template<typename T> struct IsTinyList : std::false_type {};
	template<typename T, typename S> struct IsTinyList<TinyList<T, S>> : std::true_type {};
	template<typename T> constexpr bool IsTinyList_v = IsTinyList<std::remove_cvref_t<T>>::value;

	// tostring
	template<typename T>
	struct StringFuncs<T, std::enable_if_t<IsTinyList_v<T>>> {
		using S = typename T::S;
		static inline void Append(std::string& s, T const& in) {
			s.push_back('[');
			if (auto inLen = in.Len()) {
				for (S i = 0; i < inLen; ++i) {
					::xx::Append(s, in[i]);
					s.push_back(',');
				}
				s[s.size() - 1] = ']';
			} else {
				s.push_back(']');
			}
		}
	};

	// serde
	template<typename T>
	struct DataFuncs<T, std::enable_if_t< (IsTinyList_v<T>)>> {
		using U = typename T::ChildType;
		using S = typename T::S;
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			auto inLen = in.Len();
			d.WriteVarInteger<needReserve>((size_t)inLen);
			if (!inLen) return;
			if constexpr (sizeof(U) == 1 || std::is_floating_point_v<U>) {
				d.WriteFixedArray<needReserve>(in.Buf(), inLen);
			} else if constexpr (std::is_integral_v<U>) {
				if constexpr (needReserve) {
					auto cap = (size_t)inLen * (sizeof(U) + 2);
					if (d.cap < cap) {
						d.Reserve<false>(cap);
					}
				}
				for(S i = 0; i < inLen; ++i) {
					d.WriteVarInteger<false>(in[i]);
				}
			} else {
				for (S i = 0; i < inLen; ++i) {
					d.Write<needReserve>(in[i]);
				}
			}
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz = 0;
			if (int r = d.ReadVarInteger(siz)) return r;
			if (d.offset + siz > d.len) return __LINE__;
			out.Resize((S)siz);
			if (siz == 0) return 0;
			auto buf = out.Buf();
			if constexpr (sizeof(U) == 1 || std::is_floating_point_v<U>) {
				if (int r = d.ReadFixedArray(buf, siz)) return r;
			} else {
				for (size_t i = 0; i < siz; ++i) {
					if (int r = d.Read(buf[i])) return r;
				}
			}
			return 0;
		}
	};

}
