#pragma once
#include "xx_includes.h"

namespace xx {

    /************************************************************************************/
    // template is same checkers

    template< template<typename...>typename T>
    struct Template_t {};

    template<typename T>
    struct TemplateExt_t {
        static constexpr bool isTemplate{ false };
    };

    template< template<typename...> typename T, typename...Args>
    struct TemplateExt_t<T<Args...>> {
        static constexpr bool isTemplate{ true };
        using Type = Template_t<T>;
    };

    template<typename T, typename U>
    constexpr bool TemplateIsSame() {
        if constexpr (TemplateExt_t<T>::isTemplate != TemplateExt_t<U>::isTemplate) return false;
        else return std::is_same_v<typename TemplateExt_t<T>::Type, typename TemplateExt_t<U>::Type>;
    }

    template<typename T, typename U>
    constexpr bool TemplateIsSame_v = TemplateIsSame<T, U>();

    struct AnyType {};  // helper type

    template<typename T> constexpr bool IsStdOptional_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::optional<AnyType>>;
    template<typename T> constexpr bool IsStdPair_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::pair<AnyType, AnyType>>;
    template<typename T> constexpr bool IsStdTuple_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::tuple<AnyType>>;
    template<typename T> constexpr bool IsStdVariant_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::variant<AnyType>>;
    template<typename T> constexpr bool IsStdUniquePtr_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::unique_ptr<AnyType>>;
    template<typename T> constexpr bool IsStdVector_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::vector<AnyType>>;
    template<typename T> constexpr bool IsStdList_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::list<AnyType>>;
    template<typename T> constexpr bool IsStdSet_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::set<AnyType>>;
    template<typename T> constexpr bool IsStdUnorderedSet_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::unordered_set<AnyType>>;
    template<typename T> constexpr bool IsStdMap_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::map<AnyType, AnyType>>;
    template<typename T> constexpr bool IsStdUnorderedMap_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::unordered_map<AnyType, AnyType>>;
    template<typename T> constexpr bool IsStdQueue_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::queue<AnyType>>;
    template<typename T> constexpr bool IsStdDeque_v = TemplateIsSame_v<std::remove_cvref_t<T>, std::deque<AnyType>>;
    // ...

    template<typename T> constexpr bool IsStdSetLike_v = IsStdSet_v<T> || IsStdUnorderedSet_v<T>;
    template<typename T> constexpr bool IsStdMapLike_v = IsStdMap_v<T> || IsStdUnorderedMap_v<T>;
    template<typename T> constexpr bool IsStdQueueLike_v = IsStdQueue_v<T> || IsStdDeque_v<T>;
    // ...

    template<typename> struct IsStdArray : std::false_type {};
    template<typename T, size_t S> struct IsStdArray<std::array<T, S>> : std::true_type {};
    template<typename T> constexpr bool IsStdArray_v = IsStdArray<std::remove_cvref_t<T>>::value;

    /************************************************************************************/
    // std::is_pod like, flag container can memcpy or memmove

    template<typename T, typename ENABLED = void>
    struct IsPod : std::false_type {};
    template<typename T>
    struct IsPod<T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>>> : std::true_type {};
    template<typename T> constexpr bool IsPod_v = IsPod<T>::value;

    /************************************************************************************/
    // check T is literal "xxxxx" strings

    template<typename T, typename = void> struct IsLiteral : std::false_type {};
    template<size_t L> struct IsLiteral<char[L], void> : std::true_type {
        static const size_t len = L;
    };
    template<size_t L> struct IsLiteral<char const [L], void> : std::true_type {
        static const size_t len = L;
    };
    template<size_t L> struct IsLiteral<char const (&)[L], void> : std::true_type {
        static const size_t len = L;
    };
    template<typename T> constexpr bool IsLiteral_v = IsLiteral<T>::value;
    template<typename T> constexpr size_t LiteralLen = IsLiteral<T>::len;

    /************************************************************************************/
    // check T has member funcs: data() size()

    template<typename, typename = void> struct IsContainer : std::false_type {};
    template<typename T> struct IsContainer<T, std::void_t<decltype(std::declval<T>().data()), decltype(std::declval<T>().size())>>
        : std::true_type {};
    template<typename T>
    constexpr bool IsContainer_v = IsContainer<T>::value;

    /************************************************************************************/
    // check T has member func: operator()

    template<typename T, typename = void> struct IsCallable : std::is_function<T> {};
    template<typename T> struct IsCallable<T, typename std::enable_if<std::is_same<decltype(void(&T::operator())), void>::value>::type> : std::true_type {};
    template<typename T> constexpr bool IsCallable_v = IsCallable<T>::value;

    /************************************************************************************/
	// check T is std::function

	template<typename> struct IsFunction : std::false_type {};
	template<typename T> struct IsFunction<std::function<T>> : std::true_type {
		using FT = T;
	};
	template<typename T> constexpr bool IsFunction_v = IsFunction<std::remove_cvref_t<T>>::value;

    /************************************************************************************/
    // check tuple contains T

    template<typename T, typename Tuple> struct HasType;
    template<typename T> struct HasType<T, std::tuple<>> : std::false_type {};
    template<typename T, typename U, typename... Ts> struct HasType<T, std::tuple<U, Ts...>> : HasType<T, std::tuple<Ts...>> {};
    template<typename T, typename... Ts> struct HasType<T, std::tuple<T, Ts...>> : std::true_type {};
    template<typename T, typename Tuple> using TupleContainsType = typename HasType<T, Tuple>::type;
    template<typename T, typename Tuple> constexpr bool TupleContainsType_v = TupleContainsType<T, Tuple>::value;

    /************************************************************************************/
    // return T in tuple's index

    template<class T, class Tuple> struct TupleTypeIndex;
    template<class T, class...TS> struct TupleTypeIndex<T, std::tuple<T, TS...>> {
        static const size_t value = 0;
    };
    template<class T, class U, class... TS> struct TupleTypeIndex<T, std::tuple<U, TS...>> {
        static const size_t value = 1 + TupleTypeIndex<T, std::tuple<TS...>>::value;
    };
    template<typename T, typename Tuple> constexpr size_t TupleTypeIndex_v = TupleTypeIndex<T, Tuple>::value;

    /************************************************************************************/
    // foreach tuple elements

    template <typename Tuple, typename F, std::size_t ...Indices>
    void ForEachCore(Tuple&& tuple, F&& f, std::index_sequence<Indices...>) {
    	using swallow = int[];
    	(void)swallow { 1, (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})... };
    }

    template <typename Tuple, typename F>
    void ForEach(Tuple&& tuple, F&& f) {
    	constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
        ForEachCore(std::forward<Tuple>(tuple), std::forward<F>(f), std::make_index_sequence<N>{});
    }

    /************************************************************************************/
    // foreach tuple types
    /*
    	xx::ForEachType<Tup>([&]<typename T>() {
			// ...
		});
    */

    template<typename T, typename F>
    static inline constexpr void ForEachType(F&& func) {
        auto h = []<typename TT, typename FF, size_t... I>(FF && func, std::index_sequence<I...>) {
            (func.template operator()<std::tuple_element_t<I, TT>>(), ...);
        };
        h.template operator()<T> (
            std::forward<F>(func),
            std::make_index_sequence<std::tuple_size_v<T>>{}
        );
    }

    /************************************************************************************/
    // simple tuple memory like array<T, siz>

    template<typename ...TS> struct SimpleTuple;
    template<typename T> struct SimpleTuple<T> {
        T value;
    };
    template<typename T, typename ...TS> struct SimpleTuple<T, TS...> {
        T value;
        SimpleTuple<TS...> others;
    };

    template<typename T, typename...TS>
    auto&& Get(xx::SimpleTuple<TS...>& t) {
        if constexpr (std::is_same_v< decltype(t.value), T>) {
            return t.value;
        } else {
            return Get<T>(t.others);
        }
    }

    /************************************************************************************/
    // ref args[index]

    template <int I, typename...Args>
    decltype(auto) GetAt(Args&&...args) {
        return std::get<I>(std::forward_as_tuple(std::forward<Args>(args)...));
    }

    /************************************************************************************/
    // static comparer func type define

    template<typename A, typename B>
    using ABComparer = bool (*) (A const& a, B const& b);


    /************************************************************************************/
    // mapping operator(): return T, container, args, mutable

    template<typename T, class = void>
    struct FuncTraits;

    template<typename Rtv, typename...Args>
    struct FuncTraits<Rtv(*)(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = void;
        static const bool isMutable = true;
    };

    template<typename Rtv, typename...Args>
    struct FuncTraits<Rtv(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = void;
        static const bool isMutable = true;
    };

    template<typename Rtv, typename CT, typename... Args>
    struct FuncTraits<Rtv(CT::*)(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = CT;
        static const bool isMutable = true;
    };

    template<typename Rtv, typename CT, typename... Args>
    struct FuncTraits<Rtv(CT::*)(Args ...) const> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = CT;
        static const bool isMutable = false;
    };

    template<typename T>
    struct FuncTraits<T, std::void_t<decltype(&T::operator())> >
        : public FuncTraits<decltype(&T::operator())> {};

    template<typename T> using FuncR_t = typename FuncTraits<T>::R;
    template<typename T> using FuncA_t = typename FuncTraits<T>::A;
    template<typename T> using FuncA2_t = typename FuncTraits<T>::A2;
    template<typename T> using FuncC_t = typename FuncTraits<T>::C;
    template<typename T> constexpr bool isMutable_v = FuncTraits<T>::isMutable;


    /************************************************************************************/
    // check T is baseof Template. usage: XX_IsTemplateOf(BT, T)::value

    struct IsTemplateOf {
        template <template <class> class TM, class T> static std::true_type  checkfunc(TM<T>);
        template <template <class> class TM>          static std::false_type checkfunc(...);
        template <template <int>   class TM, int N>   static std::true_type  checkfunc(TM<N>);
        template <template <int>   class TM>          static std::false_type checkfunc(...);
    };
#define XX_IsTemplateOf(TM, ...) decltype(::xx::IsTemplateOf::checkfunc<TM>(std::declval<__VA_ARGS__>()))

    /************************************************************************************/
    // ctor call helper

    template<typename U>
    struct TCtor {
        U* p;
        template<typename...Args>
        U& operator()(Args&&...args) {
            return *new (p) U(std::forward<Args>(args)...);
        }
    };

    /************************************************************************************/
    // literal string's template container

    template<size_t n>
    struct Str {
        char v[n];
        constexpr Str(const char(&s)[n]) {
            std::copy_n(s, n, v);
        }
        constexpr std::string_view ToSV() const {
            return { v, n - 1 };
        }
    };

    /***********************************************************************************/
    // find max size in multi types

    template<typename T, typename... Args>
    struct MaxSizeof {
        static const size_t value = sizeof(T) > MaxSizeof<Args...>::value
            ? sizeof(T)
            : MaxSizeof<Args...>::value;
    };
    template<typename T>
    struct MaxSizeof<T> {
        static const size_t value = sizeof(T);
    };

    template<typename T, typename... Args>
    constexpr size_t MaxSizeof_v = MaxSizeof<T, Args...>::value;

    /************************************************************************************/
    // helper types

    enum class ForeachResult {
        Continue,
        RemoveAndContinue,
        Break,
        RemoveAndBreak
    };

    template<typename T>
    struct FromTo {
        T from{}, to{};
    };
    template<typename T> constexpr bool IsFromTo_v = TemplateIsSame_v<std::remove_cvref_t<T>, FromTo<AnyType>>;

    template<typename T>
    struct CurrentMax {
        T current{}, max{};
    };
    template<typename T> constexpr bool IsCurrentMax_v = TemplateIsSame_v<std::remove_cvref_t<T>, CurrentMax<AnyType>>;

    template<typename T, typename S>
    struct BufLenRef {
        T* buf;
        S* len;
    };
    template<typename T> constexpr bool IsBufLenRef_v = TemplateIsSame_v<std::remove_cvref_t<T>, BufLenRef<AnyType, AnyType>>;

    /************************************************************************************/
    // check T is []{} likely (lambda)( depend on compiler impl )
    // unsafe

    namespace Detail {
        template<Str s, typename T>
        inline static constexpr bool LambdaChecker() {
            if constexpr (sizeof(s.v) > 8) {
                for (std::size_t i = 0; i < sizeof(s.v) - 8; i++) {
                    if ((s.v[i + 0] == '(' || s.v[i + 0] == '<')
                        && s.v[i + 1] == 'l'
                        && s.v[i + 2] == 'a'
                        && s.v[i + 3] == 'm'
                        && s.v[i + 4] == 'b'
                        && s.v[i + 5] == 'd'
                        && s.v[i + 6] == 'a'
                        && (s.v[i + 7] == ' ' || s.v[i + 7] == '(' || s.v[i + 7] == '_')
                        ) return true;
                }
            }
            return false;
        }
    }

    template<typename...T>
    constexpr bool IsLambda() {
        return ::xx::Detail::LambdaChecker<
#ifdef _MSC_VER
            __FUNCSIG__
#else
            __PRETTY_FUNCTION__
#endif
            , T...>();
    }
    template<typename ...T>
    constexpr bool IsLambda_v = IsLambda<T...>();

}
