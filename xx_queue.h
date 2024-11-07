#pragma once
#include "xx_data.h"
#include "xx_string.h"

namespace xx {

	// ring buffer FIFO queue, support random index visit, batch pop. performance better than std

	//...............FR...............					// Head == Tail
	//......Head+++++++++++Tail.......					// DataLen = Tail - Head
	//++++++Tail...........Head+++++++					// DataLen = BufLen - Head + Tail
	template <typename T, typename SizeType = ptrdiff_t>
	struct Queue {
		typedef T ChildType;
		using S = SizeType;
		T*			buf;
		SizeType	cap;
		SizeType	head{}, tail{};						// TH..............................

		explicit Queue(SizeType capacity = 8) noexcept {
			if (capacity < 8) {
				capacity = 8;
			}
			auto bufByteLen = Round2n(capacity * sizeof(T));
			buf = AlignedAlloc<T>((size_t)bufByteLen);				// buf can't be null
			assert(buf);
			cap = SizeType(bufByteLen / sizeof(T));
		}

		Queue(Queue&& o) noexcept : Queue() {
			std::swap(buf, o.buf);
			std::swap(cap, o.cap);
			std::swap(head, o.head);
			std::swap(tail, o.tail);
		}

		~Queue() noexcept {
			assert(buf);
			Clear();
			AlignedFree<T>(buf);
			buf = nullptr;
		}

		Queue(Queue const& o) = delete;
		Queue& operator=(Queue const& o) = delete;

		T& operator[](SizeType idx) const noexcept {
			return At(idx);
		}

		T& At(SizeType idx) const noexcept {
			assert(idx < Count());
			if (head + idx < cap) {
				return (T&)buf[head + idx];
			} else {
				return (T&)buf[head + idx - cap];
			}
		}

		SizeType Count() const noexcept {
			//......Head+++++++++++Tail.......
			//...............FR...............
			if (head <= tail) return tail - head;
			// ++++++Tail...........Head++++++
			else return tail + (cap - head);
		}

		bool Empty() const noexcept {
			return head == tail;
		}

		void Clear() noexcept {
			//........HT......................
			if (head == tail) return;

			if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
				//......Head+++++++++++Tail......
				if (head < tail) {
					for (auto i = head; i < tail; ++i) {
						buf[i].~T();
					}
				}
				// ++++++Tail...........Head++++++
				else {
					for (SizeType i = 0; i < tail; ++i) {
						buf[i].~T();
					}
					for (auto i = head; i < cap; ++i) {
						buf[i].~T();
					}
				}
			}

			//........HT......................
			head = tail = 0;
		}

		template<bool callByEmplace = false>
		void Reserve(SizeType capacity) noexcept {
			assert(capacity > 0);
			if (capacity <= cap) return;

			auto newBufByteLen = Round2n(capacity * sizeof(T));
			auto newBuf = AlignedAlloc<T>((size_t)newBufByteLen);
			assert(newBuf);
			auto newBufLen = SizeType(newBufByteLen / sizeof(T));

			// callByEmplace == true: ++++++++++++++TH++++++++++++++++
			auto dataLen = callByEmplace ? cap : Count();

			//......Head+++++++++++Tail.......
			if (head < tail) {
				if constexpr (xx::IsPod_v<T>) {
					memcpy((void*)newBuf, buf + head, dataLen * sizeof(T));
				} else {
					for (size_t i = 0; i < dataLen; ++i) {
						new (newBuf + i) T((T&&)buf[head + i]);
						buf[head + i].~T();
					}
				}
			}
			// ++++++Tail...........Head+++++++
			// ++++++++++++++TH++++++++++++++++
			else {
				//...Head++++++
				auto frontDataLen = cap - head;
				if constexpr (xx::IsPod_v<T>) {
					memcpy((void*)newBuf, buf + head, frontDataLen * sizeof(T));
				} else {
					for (size_t i = 0; i < frontDataLen; ++i) {
						new (newBuf + i) T((T&&)buf[head + i]);
						buf[head + i].~T();
					}
				}

				// ++++++Tail...
				if constexpr (xx::IsPod_v<T>) {
					memcpy((void*)(newBuf + frontDataLen), buf, tail * sizeof(T));
				} else {
					for (size_t i = 0; i < tail; ++i) {
						new (newBuf + frontDataLen + i) T((T&&)buf[i]);
						buf[i].~T();
					}
				}
			}

			// Head+++++++++++Tail.............
			head = 0;
			tail = dataLen;

			AlignedFree<T>(buf);
			buf = newBuf;
			cap = newBufLen;
		}

		template<typename...Args>
		T& Emplace(Args&&...args) noexcept {
			auto idx = tail;
			new (buf + tail++) T(std::forward<Args>(args)...);
			if (tail == cap) {			// cycle
				tail = 0;
			}
			if (tail == head) {			// no more space
				idx = cap - 1;
				Reserve<true>(cap * 2);
			}
			return buf[idx];
		}

		template<typename ...TS>
		void Push(TS&& ...vs) noexcept {
			(Emplace(std::forward<TS>(vs)), ...);
		}

		bool TryPop(T& outVal) noexcept {
			if (head == tail) return false;
			outVal = std::move(buf[head]);
			Pop();
			return true;
		}

		T& Top() const noexcept {
			assert(head != tail);
			return (T&)buf[head];
		}

		// ++head
		void Pop() noexcept {
			assert(head != tail);
			if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
				buf[head].~T();
			}
			++head;
			if (head == cap) {
				head = 0;
			}
		}

		SizeType PopMulti(SizeType count) noexcept {
			if (count <= 0) return 0;

			auto dataLen = Count();
			if (count >= dataLen) {
				Clear();
				return dataLen;
			}
			// count < dataLen

			//......Head+++++++++++Tail......
			if (head < tail) {
				if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
					//......Head+++++++++++count......
					for (auto i = head; i < head + count; ++i) buf[i].~T();
				}
				head += count;
			}
			// ++++++Tail...........Head++++++
			else {
				auto frontDataLen = cap - head;
				//...Head+++
				if (count < frontDataLen) {
					if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
						for (auto i = head; i < head + count; ++i) buf[i].~T();
					}
					head += count;
				} else {
					if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
						//...Head++++++
						for (auto i = head; i < cap; ++i) buf[i].~T();
					}

					// <-Head
					head = count - frontDataLen;

					if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
						// ++++++Head...
						for (SizeType i = 0; i < head; ++i) buf[i].~T();
					}
				}
			}
			return count;
		}

		// [ tail-1 ]
		T& Last() const noexcept {
			assert(head != tail);
			return (T&)buf[tail - 1 == (size_t)-1 ? cap - 1 : tail - 1];
		}

		void PopLast() noexcept {
			assert(head != tail);
			if constexpr (!(std::is_standard_layout_v<T> && std::is_trivial_v<T>)) {
				buf[tail].~T();
			}
			--tail;
			if (tail == (SizeType)-1) {
				tail = cap - 1;
			}
		}
	};

    // mem moveable tag
    template <typename T, typename SizeType>
    struct IsPod<Queue<T, SizeType>, void> : std::true_type {};

	template<typename T>
	using Queuei32 = Queue<T, int32_t>;

	template<typename T>
	struct IsXxQueue : std::false_type {};
	template<typename T, typename S>
	struct IsXxQueue<Queue<T, S>> : std::true_type {};
	template<typename T, typename S>
	struct IsXxQueue<Queue<T, S>&> : std::true_type {};
	template<typename T, typename S>
	struct IsXxQueue<Queue<T, S> const&> : std::true_type {};
	template<typename T>
	constexpr bool IsXxQueue_v = IsXxQueue<T>::value;

	// tostring
	template<typename T>
	struct StringFuncs<T, std::enable_if_t<IsXxQueue_v<T>>> {
		static inline void Append(std::string& s, T const& in) {
			s.push_back('[');
			if (auto inLen = in.Count()) {
				for (typename T::S i = 0; i < inLen; ++i) {
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
	struct DataFuncs<T, std::enable_if_t< (IsXxQueue_v<T>)>> {
		using U = typename T::ChildType;
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			auto siz = (size_t)in.Count();
			d.WriteVarInteger<needReserve>(siz);
			if (!siz) return;
			for (size_t i = 0; i < siz; ++i) {
				d.Write<needReserve>(in[(typename T::S)i]);
			}
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz = 0;
			if (int r = d.ReadVarInteger(siz)) return r;
			if (d.offset + siz > d.len) return __LINE__;
			out.Clear();
			if (siz == 0) return 0;
			out.Reserve((typename T::S)siz);
			for (size_t i = 0; i < siz; ++i) {
				auto&& o = out.Emplace();
				if (int r = d.Read(o)) return r;
			}
			return 0;
		}
	};

}
