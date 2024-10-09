#pragma once
#include "xx_data_shared.h"
#include "xx_ptr.h"

namespace xx {

    /*  for copy
struct XXXXXXXXXXX {
    static constexpr uint16_t cTypeId{ 111111111 };
	static constexpr uint16_t cParentTypeId{ xx::SerdeBase::cTypeId };
    void WriteTo(xx::Data& d) const override;
    int32_t ReadFrom(xx::Data_r& dr) override;
};
    */
    struct SerdeBase {
        static constexpr uint16_t cTypeId{};
        static constexpr uint16_t cParentTypeId{};
        SerdeBase() = default;
        virtual ~SerdeBase() = default;
        virtual void WriteTo(Data& d) const {};
        virtual int32_t ReadFrom(Data_r& dr) { return __LINE__; }
        uint16_t typeId;  // fill by MakeShared, after constructor
    };

    struct SerdeInfo;
    struct DataEx_r : Data_r {
        using Data_r::Data_r;
        SerdeInfo* si{};                            // need fill before use
        xx::Listi32<xx::Shared<SerdeBase>> ptrs;    // need clear after read finish
    };

    struct UdPtr {
        size_t* p;
        UdPtr(size_t* p) : p(p) {}
        UdPtr(UdPtr const&) = default;
        UdPtr& operator=(UdPtr const&) = default;
        UdPtr(UdPtr&& u) noexcept {
            p = std::exchange(u.p, nullptr);
        }
        UdPtr& operator=(UdPtr&& u) noexcept {
            p = std::exchange(u.p, nullptr);
            return *this;
        }
        ~UdPtr() {
            *p = 0;
        }
    };
    template<>
    struct IsPod<UdPtr, void> : std::true_type {};

    struct DataEx : Data {
        using Data::Data;
        SerdeInfo* si{};                            // need fill before use
        xx::Listi32<UdPtr> ptrs;                    // need clear after write finish
    };

    struct SerdeInfo {
        typedef Shared<SerdeBase> (*Func) ();
        inline static Shared<SerdeBase> null;

        std::array<Func, 65536> fs{};
        std::array<uint16_t, 65536> pids{};
        void Init() {
            memset(fs.data(), 0, sizeof(Func) * fs.size());
            memset(pids.data(), 0, sizeof(uint16_t) * pids.size());
        }

		XX_INLINE bool IsBaseOf(uint16_t baseTypeId, uint16_t typeId) noexcept {
			for (; typeId != baseTypeId; typeId = pids[typeId]) {
				if (!typeId || typeId == pids[typeId]) return false;
			}
			return true;
		}

		template<typename BT>
		XX_INLINE bool IsBaseOf(uint16_t typeId) noexcept {
			static_assert(std::is_base_of_v<SerdeBase, BT>);
			return IsBaseOf(BT::cTypeId, typeId);
		}

		template<typename BT, typename T>
		XX_INLINE bool IsBaseOf() noexcept {
			static_assert(std::is_base_of_v<SerdeBase, T>);
			static_assert(std::is_base_of_v<SerdeBase, BT>);
			return IsBaseOf(BT::cTypeId, T::cTypeId);
		}

		template<typename T>
        XX_INLINE void Register() {
			static_assert(std::is_base_of_v<SerdeBase, T>);
			pids[T::cTypeId] = T::cParentTypeId;
			fs[T::cTypeId] = []() -> Shared<SerdeBase> { return ::xx::MakeShared<T>(); };
		}

        template<typename T = SerdeBase>
        XX_INLINE Shared<T> MakeShared(uint16_t const& typeId) {
            static_assert(std::is_base_of_v<SerdeBase, T>);
            if (!typeId || !fs[typeId] || !IsBaseOf<T>(typeId)) return nullptr;
			return (Shared<T>&)fs[typeId]();
		}

        XX_INLINE DataEx MakeDataEx() {
            DataEx d;
            d.si = this;
            return d;
        }

        template<typename M>
        XX_INLINE DataShared MakeDataShared(M&& msg) {
            auto d = MakeDataEx();
            d.Write(std::forward<M>(msg));
            return std::move(d);
        }

        template<typename M>
        XX_INLINE DataShared MakeDataShared() {
            return MakeDataShared(::xx::MakeShared<M>());
        }

        template<typename D>
        XX_INLINE DataEx_r MakeDataEx_r(D&& d) {
            DataEx_r dr(std::forward<D>(d));
            dr.si = this;
            return dr;
        }

        template<typename M>
        XX_INLINE Shared<M> MakeMessage(DataShared const& ds) {
            auto dr = MakeDataEx_r(ds);
            xx::Shared<M> msg;
            if (dr.Read(msg)) return {};
            return msg;
        }
    };

    inline XX_INLINE uint16_t ReadVarUint16(void* buf, size_t offset, size_t len) {
        auto u = ((uint8_t*)buf)[offset] & 0x7Fu;
        if ((((uint8_t*)buf)[offset] & 0x80u) == 0) return u;
        if (offset + 1 >= len) return 0;
        return u | ((((uint8_t*)buf)[offset + 1] & 0x7Fu) << 7);
    }

    inline XX_INLINE uint16_t ReadTypeId(DataShared const& ds) {
        auto buf = ds.GetBuf();
        auto len = ds.GetLen();
        if (len < 2 || buf[0] != 1) return 0;
        return ReadVarUint16(buf, 1, len);
    }

    template<typename T>
    struct DataFuncs<T, std::enable_if_t<xx::IsShared_v<T> && std::is_base_of_v<SerdeBase, typename T::ElementType>>> {
        using U = typename T::ElementType;
        //static_assert(TypeId_v<U> > 0);

        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            if (!in) {
                d.WriteFixed<needReserve>((uint8_t)0);              // index( nullptr ) same as WriteVar(size_t 0)
            } else {
                auto h = in.GetHeader();
                if (h->ud == 0) {                   // first time
                    auto& ptrs = ((DataEx&)d).ptrs;
                    ptrs.Emplace(&h->ud);           // for restore
                    h->ud = (size_t)ptrs.len;       // store index
                    d.WriteVarInteger<needReserve>(h->ud);          // index
                    d.WriteVarInteger<needReserve>(in->typeId);     // type id
                    in->WriteTo(d);                                 // content
                } else {                            // exists
                    d.WriteVarInteger<needReserve>(h->ud);          // index
                }
            }
        }

        static inline int Read(Data_r& dr, T& out) {
            size_t idx;
            if (int r = dr.Read(idx)) return r;                 // index
            if (!idx) {                             // nullptr
                out.Reset();
                return 0;
            }
            --idx;
            auto& ptrs = ((DataEx_r&)dr).ptrs;
            auto& si = *((DataEx_r&)dr).si;
            auto len = (size_t)ptrs.len;
            if (idx == len) {                       // first time
                uint16_t typeId;
                if (int r = dr.Read(typeId)) return r;          // type id
                if (!typeId) return __LINE__;
                if (!si.fs[typeId]) return __LINE__;
                if (!si.IsBaseOf<U>(typeId)) return __LINE__;

                if (!out || out->typeId != typeId) {
                    out = std::move(si.MakeShared<U>(typeId));
                }
                assert(out);
                ptrs.Emplace(out);
                return out->ReadFrom(dr);
            } else {
                if (idx > len) return __LINE__;
                auto& o = ptrs[(int32_t)idx];
                if (!si.IsBaseOf<U>(o->typeId)) return __LINE__;
                out = (Shared<U>&)o;
                return 0;
            }
        }
    };
    
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<xx::IsWeak_v<T> && std::is_base_of_v<SerdeBase, typename T::ElementType>>> {
        using U = typename T::ElementType;
        //static_assert(TypeId_v<U> > 0);

        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            if (!in) {
                d.WriteFixed<needReserve>((uint8_t)0);              // index( nullptr ) same as WriteVar(size_t 0)
            } else {
                auto p = &in.h->data;
                d.Write<needReserve>((Shared<U>&)p);
            }
        }

        static inline int Read(Data_r& dr, T& out) {
            Shared<U> o;
            if (int r = dr.Read(o)) return r;
            out = o;
            return 0;
        }
    };

}
