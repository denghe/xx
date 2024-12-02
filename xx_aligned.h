#pragma once
#include "xx_typetraits.h"

namespace xx {
	/**************************************************************************************/
	// for sse/avx
	// alignValue should be 2^n. usually 16 ~ 64

	template<typename T, size_t alignValue, bool includeLen>
	struct AlignedBuffer {
		static_assert(alignValue >= alignof(T) && alignValue& (alignValue - 1) == 0);

		struct Core_Data { T* data{}; };
		struct Core_DataLen { T* data{}; size_t len{}; };
		std::conditional_t<includeLen, Core_DataLen, Core_Data> core;
		std::unique_ptr<char[]> raw;

		XX_INLINE void Init(size_t len_) {
			if constexpr (includeLen) {
				core.len = len_;
			}
			raw = std::make_unique_for_overwrite<char[]>(len_ * sizeof(T) + alignValue);
			core.data = (T*)(((size_t)raw.get() + (alignValue - 1)) & ~(alignValue - 1));
		}

		XX_INLINE void Clear() requires includeLen {
			memset(core.data, 0, sizeof(T) * core.len);
		}

		XX_INLINE T& operator[](size_t idx) const noexcept {
			if constexpr (includeLen) {
				assert(idx < core.len);
			}
			return (T&)core.data[idx];
		}

		AlignedBuffer() = default;
		AlignedBuffer(AlignedBuffer const&) = delete;
		AlignedBuffer& operator=(AlignedBuffer const&) = delete;
		XX_INLINE AlignedBuffer(AlignedBuffer&& o) noexcept { operator=(std::move(o)); }
		XX_INLINE AlignedBuffer& operator=(AlignedBuffer&& o) noexcept {
			std::swap(core.data, o.core.data);
			if constexpr (includeLen) {
				std::swap(core.len, o.core.len);
			}
			std::swap(raw, o.raw);
			return *this;
		}

		operator T* () const {
			return (T*)core.data;
		}

		//XX_INLINE T* operator->() const {
		//	return (T*)core.data;
		//}
		// todo: more helper operator?
	};

}
