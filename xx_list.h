#pragma once
#include "xx_data.h"
#include "xx_string.h"

namespace xx {

	// std::vector similar, resize no fill zero, move only, Shared/Weak faster memcpy
	template<typename T, typename SizeType = ptrdiff_t>
	struct List {
		typedef T ChildType;
		using S = SizeType;
		T* buf{};
		SizeType cap{}, len{};

		List() = default;
		List(List const& o) = delete;
		List& operator=(List const& o) = delete;
		List(List&& o) noexcept
			: buf(o.buf)
			, cap(o.cap)
			, len(o.len) {
			o.buf = nullptr;
			o.cap = 0;
			o.len = 0;
		}
		List& operator=(List&& o) noexcept {
			std::swap(buf, o.buf);
			std::swap(len, o.len);
			std::swap(cap, o.cap);
			return *this;
		}
		~List() noexcept {
			Clear(true);
		}

        XX_INLINE bool Empty() const {
            return !len;
        }

		XX_INLINE SizeType Count() const {
            return len;
        }

		XX_INLINE T* Buf() const {
			return buf;
		}

		XX_INLINE SizeType Len() const {
			return len;
		}

		XX_INLINE List Clone() const {
			List rtv;
			rtv.Reserve(len);
			rtv.AddRange(*this);
			return rtv;
		}

		// func == [](auto& a, auto& b) { return a->xxx < b->xxx; }
		template<typename F>
		XX_INLINE void StdSort(F&& func) {
			std::sort(buf, buf + len, std::forward<F>(func));
		}

		XX_INLINE void Reserve(SizeType cap_) noexcept {
			if (auto newBuf = ReserveBegin(cap_)) {
				ReserveEnd(newBuf);
			}
		}

		T* ReserveBegin(SizeType cap_) noexcept {
			assert(cap_ > 0);
			if (cap_ <= cap) return {};
			if (!cap) {
				buf = (T*)new MyAlignedStorage<T>[cap_];
				cap = cap_;
				return {};
			}
			do {
				cap *= 2;
			} while (cap < cap_);
			return (T*) new MyAlignedStorage<T>[cap];
		}
		void ReserveEnd(T* newBuf) noexcept {
			if constexpr (IsPod_v<T>) {
				::memcpy((void*)newBuf, (void*)buf, len * sizeof(T));
			}
			else {
				for (SizeType i = 0; i < len; ++i) {
					new (&newBuf[i]) T((T&&)buf[i]);
					buf[i].~T();
				}
			}
			delete[](MyAlignedStorage<T>*)buf;
			buf = newBuf;
		}

		template<bool fillVal = false, int val = 0>
		void Resize(SizeType len_) noexcept {
			if (len_ == len) return;
			else if (len_ < len) {
				for (SizeType i = len_; i < len; ++i) {
					buf[i].~T();
				}
			}
			else {	// len_ > len
				Reserve(len_);
				if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
					for (SizeType i = len; i < len_; ++i) {
						new (buf + i) T();
					}
				} else if constexpr(fillVal) {
					memset(buf + len, val, (len_ - len) * sizeof(T));
				}
			}
			len = len_;
		}

		XX_INLINE T& operator[](SizeType idx) const noexcept {
			assert(idx >= 0 && idx < len);
			return (T&)buf[idx];
		}

		XX_INLINE T& At(SizeType idx) const noexcept {
			xx_assert(idx >= 0 && idx < len);
			return (T&)buf[idx];
		}

		XX_INLINE T& Top() const noexcept {
			assert(len > 0);
			return (T&)buf[len - 1];
		}

		XX_INLINE void Pop() noexcept {
			assert(len > 0);
			--len;
			buf[len].~T();
		}

		XX_INLINE bool TryPop(T& output) noexcept {
			if (!len) return false;
			output = (T&&)buf[--len];
			buf[len].~T();
			return true;
		}

		void Clear(bool freeBuf = false) noexcept {
			if (!cap) return;
			if (len) {
				for (SizeType i = 0; i < len; ++i) {
					buf[i].~T();
				}
				len = 0;
			}
			if (freeBuf) {
				delete[](MyAlignedStorage<T>*)buf;
				buf = {};
				cap = 0;
			}
		}

		XX_INLINE void Remove(T const& v) noexcept {
			for (SizeType i = 0; i < len; ++i) {
				if (v == buf[i]) {
					RemoveAt(i);
					return;
				}
			}
		}

		XX_INLINE void RemoveAt(SizeType idx) noexcept {
			assert(idx >= 0 && idx < len);
			--len;
			if constexpr (IsPod_v<T>) {
				buf[idx].~T();
				::memmove(buf + idx, buf + idx + 1, (len - idx) * sizeof(T));
			}
			else {
				for (SizeType i = idx; i < len; ++i) {
					buf[i] = (T&&)buf[i + 1];
				}
				buf[len].~T();
			}
		}

		XX_INLINE void SwapRemoveAt(SizeType idx) noexcept {
			assert(idx >= 0 && idx < len);
			buf[idx].~T();
			--len;
			if (len != idx) {
				if constexpr (IsPod_v<T>) {
                    ::memcpy(&buf[idx], &buf[len], sizeof(T) );
				} else {
					new (&buf[idx]) T((T&&)buf[len]);
				}
			}
		}

        XX_INLINE void PopBack() {
            assert(len);
            --len;
            if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
                buf[len].~T();
            }
        }

        XX_INLINE T& Back() const {
            assert(len);
            return (T&)buf[len - 1];
        }

		template<typename...Args>
		XX_INLINE T& Emplace(Args&&...args) noexcept {
			if (auto newBuf = ReserveBegin(len + 1)) {
				new (&newBuf[len]) T(std::forward<Args>(args)...);
				ReserveEnd(newBuf);
				return newBuf[len++];
			} else {
				return *new (&buf[len++]) T(std::forward<Args>(args)...);
			}
		}

		template<typename ...TS>
		XX_INLINE void Add(TS&&...vs) noexcept {
			(Emplace(std::forward<TS>(vs)), ...);
		}

		void AddRange(T const* items, SizeType count) noexcept {
			if (auto newBuf = ReserveBegin(len + count)) {
				if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
					::memcpy(newBuf + len, items, count * sizeof(T));
				} else {
					for (SizeType i = 0; i < count; ++i) {
						new (&newBuf[len + i]) T(items[i]);
					}
				}
				ReserveEnd(newBuf);
			} else {
				if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
					::memcpy(buf + len, items, count * sizeof(T));
				} else {
					for (SizeType i = 0; i < count; ++i) {
						new (&buf[len + i]) T(items[i]);
					}
				}
			}
			len += count;
		}

		template<typename L>
		XX_INLINE void AddRange(L const& list) noexcept {
			return AddRange(list.buf, list.len);
		}

		XX_INLINE SizeType Find(T const& v) const noexcept {
			for (SizeType i = 0; i < len; ++i) {
				if (v == buf[i]) return i;
			}
			return SizeType(-1);
		}

        template<typename Func>
		XX_INLINE bool Exists(Func&& cond) const noexcept {
			for (SizeType i = 0; i < len; ++i) {
				if (cond(buf[i])) return true;
			}
			return false;
		}

		// simple support "for( auto&& c : list )" syntax
		struct Iter {
			T* ptr;
			bool operator!=(Iter const& other) noexcept { return ptr != other.ptr; }
			Iter& operator++() noexcept { ++ptr; return *this; }
			T& operator*() noexcept { return *ptr; }
		};
		Iter begin() noexcept { return Iter{ buf }; }
		Iter end() noexcept { return Iter{ buf + len }; }
		Iter begin() const noexcept { return Iter{ buf }; }
		Iter end() const noexcept { return Iter{ buf + len }; }
	};

	// mem moveable tag
	template<typename T, typename SizeType>
	struct IsPod<List<T, SizeType>, void> : std::true_type {};

	template<typename T>
	using Listi32 = List<T, int32_t>;

	template<typename T> struct IsList : std::false_type {};
	template<typename T, typename S> struct IsList<List<T, S>> : std::true_type {};
	template<typename T> constexpr bool IsList_v = IsList<std::remove_cvref_t<T>>::value;

	// tostring
	template<typename T>
	struct StringFuncs<T, std::enable_if_t<IsList_v<T>>> {
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
	struct DataFuncs<T, std::enable_if_t< (IsList_v<T>)>> {
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
				for (S i = 0; i < inLen; ++i) {
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
