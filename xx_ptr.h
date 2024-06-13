#pragma once
#include "xx_typetraits.h"
#include "xx_mem.h"

// std::shared_ptr / weak_ptr likely but thin, no atomic( fast 4 times ), a little unsafe for easy use

namespace xx {

    /***********************************************************************************************/

    typedef void(*PtrDeleter)(void*);
    template<typename T>
    struct PtrHeader {
        uint32_t sharedCount;
        uint32_t weakCount;
        PtrDeleter deleter{};
        T data;
        XX_INLINE void Init() {
            sharedCount = 1;
            weakCount = 0;
        }
    };

    template<typename HT>
    XX_INLINE HT* GetPtrHeader(void* p) {
        return container_of(p, HT, data);
    }

    template<typename T, typename ENABLED = void>
    struct PtrHeaderSwitcher {
        using type = PtrHeader<T>;
    };

    template<typename T, typename ENABLED = void>
    using PtrHeader_t = typename PtrHeaderSwitcher<T>::type;

    template<typename T, typename U>
    constexpr bool PtrAlignCheck_v = alignof(U) <= sizeof(void*) && alignof(T) <= sizeof(void*) || alignof(U) == alignof(T);

    template<typename T>
    struct Weak;

    /***********************************************************************************************/

    template<typename T, bool weakSupport>
    struct Shared_ {
        using HeaderType = PtrHeader_t<T>;
        using ElementType = T;
        template<typename U>
        using S = Shared_<U, weakSupport>;
        T* pointer{};

        operator bool() const {
            return pointer != nullptr;
        }

        bool Empty() const {
            return pointer == nullptr;
        }

        bool HasValue() const {
            return pointer != nullptr;
        }

        operator T *const &() const {
            return pointer;
        }

        operator T *&() {
            return pointer;
        }

        T *const &operator->() const {
            return pointer;
        }

        T const &Value() const {
            return *pointer;
        }

        T &Value() {
            return *pointer;
        }

        template<typename ...Args>
        decltype(auto) operator()(Args&&...args) {
            return (*pointer)(std::forward<Args>(args)...);
        }

        auto& operator[](size_t idx) {
            return pointer->operator[](idx);
        }

        auto const& operator[](size_t idx) const {
            return pointer->operator[](idx);
        }

        Shared_() = default;

        template<std::derived_from<T> U>
        Shared_(U* ptr) : pointer(ptr) {
            static_assert(PtrAlignCheck_v<T, U>);
            if (ptr) {
                ++GetPtrHeader<HeaderType>(ptr)->sharedCount;
            }
        }

        Shared_(T* ptr) : pointer(ptr) {
            if (ptr) {
                ++GetPtrHeader<HeaderType>(ptr)->sharedCount;
            }
        }

        template<std::derived_from<T> U>
        Shared_(S<U> const& o) : Shared_(o.pointer) {
            static_assert(PtrAlignCheck_v<T, U>);
        }

        Shared_(Shared_ const& o) : Shared_(o.pointer) {}

        template<std::derived_from<T> U>
        Shared_(S<U>&& o) {
            static_assert(PtrAlignCheck_v<T, U>);
            pointer = o.pointer;
            o.pointer = {};
        }

        Shared_(Shared_&& o) {
            pointer = o.pointer;
            o.pointer = {};
        }

        template<std::derived_from<T> U>
        Shared_ &operator=(U* ptr) {
            static_assert(PtrAlignCheck_v<T, U>);
            Reset(ptr);
            return *this;
        }

        Shared_ &operator=(T* ptr) {
            Reset(ptr);
            return *this;
        }

        template<std::derived_from<T> U>
        Shared_ &operator=(S<U> const& o) {
            static_assert(PtrAlignCheck_v<T, U>);
            Reset(o.pointer);
            return *this;
        }

        Shared_ &operator=(Shared_ const& o) {
            Reset(o.pointer);
            return *this;
        }

        template<std::derived_from<T> U>
        Shared_ &operator=(S<U> &&o) {
            static_assert(PtrAlignCheck_v<T, U>);
            Reset();
            std::swap(pointer, (*(Shared_ *) &o).pointer);
            return *this;
        }

        Shared_ &operator=(Shared_ &&o) {
            std::swap(pointer, o.pointer);
            return *this;
        }

        template<typename U>
        bool operator==(S<U> const& o) const {
            return pointer == o.pointer;
        }

        template<typename U>
        bool operator!=(S<U> const& o) const {
            return pointer != o.pointer;
        }

        template<std::derived_from<T> U>
        S<U> As() const {
            static_assert(PtrAlignCheck_v<T, U>);
            if constexpr (std::is_same_v<U, T>) {
                return *this;
            } else if constexpr (std::is_base_of_v<U, T>) {
                return pointer;
            } else {
                return dynamic_cast<U *>(pointer);
            }
        }

        // unsafe
        template<std::derived_from<T> U>
        S<U> &Cast() const {
            static_assert(PtrAlignCheck_v<T, U>);
            return *(S<U> *) this;
        }

        // unsafe
        XX_INLINE HeaderType* GetHeader() const {
            return (HeaderType*)GetPtrHeader<HeaderType>(pointer);
        }

        uint32_t GetSharedCount() const {
            if (!pointer) return 0;
            return GetHeader()->sharedCount;
        }

        uint32_t GetWeakCount() const {
            if (!pointer) return 0;
            return GetHeader()->weakCount;
        }

        void Reset() {
            if (pointer) {
                auto h = GetHeader();
                assert(h->sharedCount);
                // think about field weak point to self
                if (h->sharedCount == 1) {
                    h->deleter(pointer);
                    pointer = {};
                    if constexpr (weakSupport) {
                        if (h->weakCount == 0) {
                            AlignedFree<HeaderType>(h);
                        } else {
                            h->sharedCount = 0;
                        }
                    } else {
                        AlignedFree<HeaderType>(h);
                    }
                } else {
                    --h->sharedCount;
                    pointer = {};
                }
            }
        }

        template<std::derived_from<T> U>
        void Reset(U* ptr) {
            static_assert(PtrAlignCheck_v<T, U>);
            if (pointer == ptr) return;
            Reset();
            if (ptr) {
                pointer = ptr;
                ++GetPtrHeader<HeaderType>(ptr)->sharedCount;
            }
        }

        ~Shared_() {
            Reset();
        }

        template<std::derived_from<T> U = T, typename...Args>
        S<U>& Emplace(Args &&...args) {
            static_assert(PtrAlignCheck_v<T, U>);
            Reset();
            auto h = AlignedAlloc<typename S<U>::HeaderType>();
            h->Init();
            pointer = (T*)&h->data;
            h->deleter = [](void* o) { std::destroy_at((U*)o); };
            std::construct_at(&h->data, std::forward<Args>(args)...);
            return (S<U>&)*this;
        }

        Weak<T> ToWeak() const requires(weakSupport);
    };

    template<typename T>
    using Shared = Shared_<T, true>;

    template<typename T>
    using Ref = Shared_<T, false>;

    /************************************************************************************/
    // std::weak_ptr like

    template<typename T>
    struct Weak {
        using HeaderType = PtrHeader_t<T>;
        using ElementType = T;
        HeaderType* h{};

        uint32_t GetSharedCount() const {
            if (!h) return 0;
            return h->sharedCount;
        }

        uint32_t GetWeakCount() const {
            if (!h) return 0;
            return h->weakCount;
        }

        operator bool() const {
            return h && h->sharedCount;
        }

        // unsafe
        T *pointer() const {
            return &h->data;
        }

        // unsafe
        T *GetPointer() const {
            return pointer();
        }

        // unsafe
        void SetH(void* h_) {
            h = (HeaderType*)h_;
        }

        template<typename ...Args>
        decltype(auto) operator()(Args&&...args) {
            return (*pointer())(std::forward<Args>(args)...);
        }

        void Reset() {
            if (h) {
                if (h->weakCount == 1 && h->sharedCount == 0) {
                    AlignedFree<HeaderType>(h);
                } else {
                    --h->weakCount;
                }
                h = {};
            }
        }

        template<std::derived_from<T> U>
        void Reset(Shared<U> const& s) {
            static_assert(PtrAlignCheck_v<T, U>);
            Reset();
            if (s.pointer) {
                h = s.GetHeader();
                ++h->weakCount;
            }
        }

        Shared<T> Lock() const {
            if (h && h->sharedCount) {
                auto p = &h->data;
                return (Shared<T>&)p;
            }
            return {};
        }

        // unsafe: need ensure "alive"
        ElementType *operator->() const {
            return &h->data;
        }

        // unsafe: need ensure "alive"
        ElementType const &Value() const {
            return h->data;
        }

        // unsafe: need ensure "alive"
        ElementType &Value() {
            return h->data;
        }

        // unsafe: need ensure "alive"
        operator ElementType *() const {
            return &h->data;
        }

        template<std::derived_from<T> U>
        Weak &operator=(Shared<U> const &o) {
            Reset(o);
            return *this;
        }

        template<std::derived_from<T> U>
        Weak(Shared<U> const &o) {
            Reset(o);
        }

        ~Weak() {
            Reset();
        }

        Weak() = default;

        Weak(Weak const &o) : h(o.h) {
            if (o.h) {
                ++o.h->weakCount;
            }
        }

        template<std::derived_from<T> U>
        Weak(Weak<U> const &o) : h(o.h) {
            static_assert(PtrAlignCheck_v<T, U>);
            if (o.h) {
                ++o.h->weakCount;
            }
        }

        Weak(Weak &&o) : h(o.h) {
            o.h = {};
        }

        template<std::derived_from<T> U>
        Weak(Weak<U> &&o) : h(o.h) {
            static_assert(PtrAlignCheck_v<T, U>);
            o.h = {};
        }

        Weak &operator=(Weak const &o) {
            if (&o != this) {
                Reset(o.Lock());
            }
            return *this;
        }

        template<std::derived_from<T> U>
        Weak &operator=(Weak<U> const &o) {
            if ((void *) &o != (void *) this) {
                Reset(((Weak *) (&o))->Lock());
            }
            return *this;
        }

        Weak &operator=(Weak &&o) {
            std::swap(h, o.h);
            return *this;
        }

        template<std::derived_from<T> U>
        bool operator==(Weak<U> const &o) const {
            return h == o.h;
        }

        template<std::derived_from<T> U>
        bool operator!=(Weak<U> const &o) const {
            return h != o.h;
        }
    };

    template<typename T, bool weakSupport>
    Weak<T> Shared_<T, weakSupport>::ToWeak() const requires(weakSupport) {
        if (pointer) {
            auto h = GetHeader();
            return (Weak<T>&)h;
        }
        return {};
    }

    /************************************************************************************/

    // memmove support flags
    template<typename T, bool b> struct IsPod<Shared_<T, b>, void> : std::true_type {};
    template<typename T> struct IsPod<Shared<T>, void> : std::true_type {};
    template<typename T> struct IsPod<Weak<T>, void> : std::true_type {};
    template<typename T> struct IsPod<Ref<T>, void> : std::true_type {};

    template<typename T> struct IsShared : std::false_type {};
    template<typename T> struct IsShared<Shared<T>> : std::true_type {};
    template<typename T> struct IsShared<Shared<T> &> : std::true_type {};
    template<typename T> struct IsShared<Shared<T> const &> : std::true_type {};
    template<typename T> constexpr bool IsShared_v = IsShared<T>::value;

    template<typename T> struct IsWeak : std::false_type {};
    template<typename T> struct IsWeak<Weak<T>> : std::true_type {};
    template<typename T> struct IsWeak<Weak<T> &> : std::true_type {};
    template<typename T> struct IsWeak<Weak<T> const &> : std::true_type {};
    template<typename T> constexpr bool IsWeak_v = IsWeak<T>::value;

    template<typename T> struct IsRef : std::false_type {};
    template<typename T> struct IsRef<Ref<T>> : std::true_type {};
    template<typename T> struct IsRef<Ref<T>&> : std::true_type {};
    template<typename T> struct IsRef<Ref<T> const&> : std::true_type {};
    template<typename T> constexpr bool IsRef_v = IsRef<T>::value;


    template<typename T, typename...Args>
    Shared<T> MakeShared(Args &&...args) {
        Shared<T> rtv;
        rtv.Emplace(std::forward<Args>(args)...);
        return rtv;
    }

    template<typename T, typename ...Args>
    Shared<T> TryMakeShared(Args &&...args) {
        try {
            return Make<T>(std::forward<Args>(args)...);
        }
        catch (...) {
            return Shared<T>();
        }
    }

    template<typename T, typename ...Args>
    Shared<T> &MakeSharedTo(Shared<T> &v, Args &&...args) {
        v = Make<T>(std::forward<Args>(args)...);
        return v;
    }

    template<typename T, typename ...Args>
    Shared<T> &TryMakeSharedTo(Shared<T> &v, Args &&...args) {
        v = TryMake<T>(std::forward<Args>(args)...);
        return v;
    }

    template<typename T, typename U>
    Shared<T> As(Shared<U> const &v) {
        return v.template As<T>();
    }

    template<typename T, typename U>
    bool Is(Shared<U> const &v) {
        return !v.template As<T>().Empty();
    }

    // unsafe
    template<typename T>
    Shared<T> SharedFromThis(T * thiz) {
        assert(thiz);
        return *(Shared<T> *) &thiz;
    }

    // unsafe
    template<typename T>
    Weak<T> WeakFromThis(T * thiz) {
        assert(thiz);
        return (*(Shared<T>*)&thiz).ToWeak();
    }

    template<typename T, typename...Args>
    Ref<T> MakeRef(Args &&...args) {
        Ref<T> rtv;
        rtv.Emplace(std::forward<Args>(args)...);
        return rtv;
    }

    // unsafe
    template<typename T>
    Ref<T>& RefFromThis(T* thiz) {
        assert(thiz);
        return (Ref<T>&)thiz;
    }

    template<typename U, typename T = U>
    Weak<T> ToWeak(Shared<U> const& s) {
        static_assert(std::is_base_of_v<T, U>);
        static_assert(PtrAlignCheck_v<T, U>);
        if (s) {
            auto h = s.GetHeader();
            return (Weak<T>&)h;
        }
        return {};
    }
}

// let Shared Weak Ref support std hash container
namespace std {
    template<typename T>
    struct hash<xx::Shared<T>> {
        size_t operator()(xx::Shared<T> const &v) const {
            return (size_t) v.pointer;
        }
    };

    template<typename T>
    struct hash<xx::Weak<T>> {
        size_t operator()(xx::Weak<T> const &v) const {
            return (size_t) v.h;
        }
    };

    template<typename T>
    struct hash<xx::Ref<T>> {
        size_t operator()(xx::Ref<T> const &v) const {
            return (size_t) v.pointer;
        }
    };
}
