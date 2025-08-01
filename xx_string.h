﻿#pragma once
#include "xx_data.h"

namespace xx {
    // 各种 string 辅助( 主要针对基础数据类型或简单自定义结构 )

    /************************************************************************************/
    // StringFuncs 适配模板 for easy to append value to std::string
    /************************************************************************************/

    template<typename T, typename ENABLED = void>
    struct StringFuncs {
        static inline void Append(std::string& s, T const& in) {
            assert(false);
        }
        static inline void AppendCore(std::string& s, T const& in) {
            assert(false);
        }
    };

    /************************************************************************************/
    // Append / ToString
    /************************************************************************************/

    namespace Core {
        template<typename T>
        void Append(std::string &s, T const &v) {
            ::xx::StringFuncs<T>::Append(s, v);
        }
    }

    template<typename ...Args>
    void Append(std::string& s, Args const& ... args) {
        (::xx::Core::Append(s, args), ...);
    }

    template<std::size_t...Idxs, typename...TS>
    void AppendFormatCore(std::index_sequence<Idxs...>, std::string& s, size_t const& i, TS const&...vs) {
        (((i == Idxs) ? (Append(s, vs), 0) : 0), ...);
    }

    // 格式化追加, {0} {1}... 这种. 针对重复出现的参数, 是从已经追加出来的字串区域复制, 故追加自己并不会导致内容翻倍
    template<typename...TS>
    size_t AppendFormat(std::string& s, char const* format, TS const&...vs) {
        std::array<std::pair<size_t, size_t>, sizeof...(vs)> cache{};
        size_t offset = 0;
        while (auto c = format[offset]) {
            if (c == '{') {
                c = format[++offset];
                if (c == '{') {
                    Append(s, '{');
                }
                else {
                    size_t i = 0;
                LabLoop:
                    c = format[offset];
                    if (c) {
                        if (c == '}') {
                            if (i >= sizeof...(vs)) return i;   // error
                            if (cache[i].second) {
                                s.append(s.data() + cache[i].first, cache[i].second);
                            }
                            else {
                                cache[i].first = s.size();
                                AppendFormatCore(std::make_index_sequence<sizeof...(TS)>(), s, i, vs...);
                                cache[i].second = s.size() - cache[i].first;
                            }
                            goto LabEnd;
                        }
                        else {
                            i = i * 10 + (c - '0');
                        }
                        ++offset;
                    }
                    goto LabLoop;
                LabEnd:;
                }
            }
            else {
                Append(s, c);
            }
            ++offset;
        }
        return 0;
    }

    template<typename ...Args>
    std::string ToString(Args const& ... args) {
        std::string s;
        Append(s, args...);
        return s;
    }

    template<typename ...Args>
    std::string ToStringFormat(char const* format, Args const& ... args) {
        std::string s;
        AppendFormat(s, format, args...);
        return s;
    }

    // double to 1.234k  5M 7.8T  1e123 ...
    template<typename CharType = char>
    inline int ToStringEN(double d, CharType* o) {
        if (d < 1) {
            o[0] = '0';
            return 1;
        }
        int len{};
        auto e = (int)std::log10(d);
        if (e < 3) {
            len = e + 1;
            auto n = (int)d;
            while (n >= 10) {
                auto a = n / 10;
                auto b = n - a * 10;
                o[e--] = (char)(b + 48);
                n = a;
            }
            o[0] = (char)(n + 48);
        } else {
            auto idx = e / 3;
            d /= std::pow(10, idx * 3);
            e = e - idx * 3;
            len = e + 1;
            auto n = (int)d;
            auto bak = n;
            while (n >= 10) {
                auto a = n / 10;
                auto b = n - a * 10;
                o[e--] = (char)(b + 48);
                n = a;
            }
            o[0] = (char)(n + 48);
            if (d > bak) {
                auto first = (int)((d - bak) * 10);
                if (first > 0) {
                    o[len++] = '.';
                    o[len++] = (char)(first + 48);
                }
            }
            if (idx < 10) {
                o[len++] = " KMGTPEZYB"[idx];
            } else {
                o[len++] = 'e';
                idx *= 3;
                len += (int)std::log10(idx) + 1;
                e = len - 1;
                n = idx;
                while (n >= 10) {
                    auto a = n / 10;
                    auto b = n - a * 10;
                    o[e--] = (char)(b + 48);
                    n = a;
                }
                o[e] = (char)(n + 48);
            }
        }
        return len;
    }


    // ucs4 to utf8. write to out. return len
    inline size_t Char32ToUtf8(char32_t c32, char* out) {
        auto& c = (uint32_t&)c32;
        auto& o = (uint8_t*&)out;
        if (c < 0x7F) {
            o[0] = c;
            return 1;
        }
        else if (c < 0x7FF) {
            o[0] = 0b1100'0000 | (c >> 6);
            o[1] = 0b1000'0000 | (c & 0b0011'1111);
            return 2;
        }
        else if (c < 0x10000) {
            o[0] = 0b1110'0000 | (c >> 12);
            o[1] = 0b1000'0000 | ((c >> 6) & 0b0011'1111);
            o[2] = 0b1000'0000 | (c & 0b0011'1111);
            return 3;
        }
        else if (c < 0x110000) {
            o[0] = 0b1111'0000 | (c >> 18);
            o[1] = 0b1000'0000 | ((c >> 12) & 0b0011'1111);
            o[2] = 0b1000'0000 | ((c >> 6) & 0b0011'1111);
            o[3] = 0b1000'0000 | (c & 0b0011'1111);
            return 4;
        }
        else if (c < 0x1110000) {
            o[0] = 0b1111'1000 | (c >> 24);
            o[1] = 0b1000'0000 | ((c >> 18) & 0b0011'1111);
            o[2] = 0b1000'0000 | ((c >> 12) & 0b0011'1111);
            o[3] = 0b1000'0000 | ((c >> 6) & 0b0011'1111);
            o[4] = 0b1000'0000 | (c & 0b0011'1111);
            return 4;
        }
        xx_assert(false);   // out of char32_t handled range
        return {};
    }



    inline void StringU8ToU32(std::u32string& out, std::string_view const& sv) {
        out.reserve(out.size() + sv.size());
        char32_t wc{};
        for (int i = 0; i < sv.size(); ) {
            char c = sv[i];
            if ((c & 0x80) == 0) {
                wc = c;
                ++i;
            } else if ((c & 0xE0) == 0xC0) {
                wc = (sv[i] & 0x1F) << 6;
                wc |= (sv[i + 1] & 0x3F);
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                wc = (sv[i] & 0xF) << 12;
                wc |= (sv[i + 1] & 0x3F) << 6;
                wc |= (sv[i + 2] & 0x3F);
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                wc = (sv[i] & 0x7) << 18;
                wc |= (sv[i + 1] & 0x3F) << 12;
                wc |= (sv[i + 2] & 0x3F) << 6;
                wc |= (sv[i + 3] & 0x3F);
                i += 4;
            } else if ((c & 0xFC) == 0xF8) {
                wc = (sv[i] & 0x3) << 24;
                wc |= (sv[i] & 0x3F) << 18;
                wc |= (sv[i] & 0x3F) << 12;
                wc |= (sv[i] & 0x3F) << 6;
                wc |= (sv[i] & 0x3F);
                i += 5;
            } else if ((c & 0xFE) == 0xFC) {
                wc = (sv[i] & 0x1) << 30;
                wc |= (sv[i] & 0x3F) << 24;
                wc |= (sv[i] & 0x3F) << 18;
                wc |= (sv[i] & 0x3F) << 12;
                wc |= (sv[i] & 0x3F) << 6;
                wc |= (sv[i] & 0x3F);
                i += 6;
            }
            out += wc;
        }
    }

    inline std::u32string StringU8ToU32(std::string_view const& sv) {
        std::u32string out;
        StringU8ToU32(out, sv);
        return out;
    }

    inline void StringU32ToU8(std::string& out, std::u32string_view const& sv) {
        for (int i = 0; i < sv.size(); ++i) {
            char32_t wc = sv[i];
            if (0 <= wc && wc <= 0x7f) {
                out += (char)wc;
            } else if (0x80 <= wc && wc <= 0x7ff) {
                out += (0xc0 | (wc >> 6));
                out += (0x80 | (wc & 0x3f));
            } else if (0x800 <= wc && wc <= 0xffff) {
                out += (0xe0 | (wc >> 12));
                out += (0x80 | ((wc >> 6) & 0x3f));
                out += (0x80 | (wc & 0x3f));
            } else if (0x10000 <= wc && wc <= 0x1fffff) {
                out += (0xf0 | (wc >> 18));
                out += (0x80 | ((wc >> 12) & 0x3f));
                out += (0x80 | ((wc >> 6) & 0x3f));
                out += (0x80 | (wc & 0x3f));
            } else if (0x200000 <= wc && wc <= 0x3ffffff) {
                out += (0xf8 | (wc >> 24));
                out += (0x80 | ((wc >> 18) & 0x3f));
                out += (0x80 | ((wc >> 12) & 0x3f));
                out += (0x80 | ((wc >> 6) & 0x3f));
                out += (0x80 | (wc & 0x3f));
            } else if (0x4000000 <= wc && wc <= 0x7fffffff) {
                out += (0xfc | (wc >> 30));
                out += (0x80 | ((wc >> 24) & 0x3f));
                out += (0x80 | ((wc >> 18) & 0x3f));
                out += (0x80 | ((wc >> 12) & 0x3f));
                out += (0x80 | ((wc >> 6) & 0x3f));
                out += (0x80 | (wc & 0x3f));
            }
        }
    }

    inline std::string StringU32ToU8(std::u32string_view const& sv) {
        std::string out;
        StringU32ToU8(out, sv);
        return out;
    }

    template<typename T>
    std::string const& U8AsString(T const& u8s) {
        return (std::string const&)u8s;
    }

    template<typename T>
    std::string U8AsString(T&& u8s) {
        return (std::string&&)u8s;
    }


    // __asdf__qwer_   to  AsdfQwer
    template<bool firstCharUpperCase = true>
    std::string ToHump(std::string_view s) {
        std::string r;
        if (!s.size()) return r;
        s = s.substr(s.find_first_not_of('_'));
        if (!s.size()) return r;

        auto e = s.size();
        r.reserve(e);
        if constexpr (firstCharUpperCase) {
            r.push_back(std::toupper(s[0]));
        } else {
            r.push_back(s[0]);
        }
        for (size_t i = 1; i < e; ++i) {
            if (s[i] != '_') {
                r.push_back(s[i]);
            } else {
                do {
                    ++i;
                    if (i >= e) return r;
                } while (s[i] == '_');
                r.push_back(std::toupper(s[i]));
            }
        }
        return r;
    }


    /************************************************************************************/
    // StringFuncs 继续适配各种常见数据类型
    /************************************************************************************/

    // 适配 char* \0 结尾 字串
    template<>
    struct StringFuncs<char*, void> {
        static inline void Append(std::string& s, char* in) {
            s.append(in ? in: "null");
        }
    };

    // 适配 char const* \0 结尾 字串
    template<>
    struct StringFuncs<char const*, void> {
        static inline void Append(std::string& s, char const* in) {
            s.append(in ? in: "null");
        }
    };

    // 适配 literal char[len] string
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsLiteral_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append(in);
        }
    };

    // 适配 std::string_view
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::string_view, T> || std::is_base_of_v<std::u8string_view, T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append((std::string_view&)in);
        }
    };

    // 适配 std::u32string_view
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::u32string_view, T>>> {
        static inline void Append(std::string& s, T const& in) {
            StringU32ToU8(s, in);
        }
    };

    // 适配 std::string   //( 前后加引号 )
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::string, T>>> {
        static inline void Append(std::string& s, T const& in) {
            //s.push_back('\"');
            s.append(in);
            //s.push_back('\"');
        }
    };

    // 适配 std::u32string
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::u32string, T>>> {
        static inline void Append(std::string& s, T const& in) {
            //s.push_back('\"');
            StringU32ToU8(s, in);
            //s.push_back('\"');
        }
    };

    // 适配 type_info     typeid(T)
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::type_info, T>>> {
        static inline void Append(std::string& s, T const& in) {
            //s.push_back('\"');
            s.append(in.name());
            //s.push_back('\"');
        }
    };

    // 用来方便的插入空格
    struct CharRepeater {
        char item;
        size_t len;
    };
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<CharRepeater, T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append(in.len, in.item);
        }
    };


    // 适配所有数字( char32_t 会转为 utf8 )
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
                s.append(in ? "true" : "false");
            }
            else if constexpr (std::is_same_v<char, std::decay_t<T>> || std::is_same_v<char8_t, std::decay_t<T>>) {
                s.push_back((char)in);
            }
            else if constexpr (std::is_floating_point_v<T>) {
                std::array<char, 40> buf;
                std::string_view sv;
#ifndef _MSC_VER
                if constexpr (sizeof(T) == 4) {
                    snprintf(buf.data(), buf.size(), "%.7f", in);
                } else {
                    static_assert(sizeof(T) == 8);
                    snprintf(buf.data(), buf.size(), "%.16lf", in);
                }
                sv = buf.data();
                if (sv.find('.') != sv.npos) {
                    if (auto siz = sv.find_last_not_of('0'); siz != sv.npos) {
                        if (sv[siz] == '.') --siz;
                        sv = std::string_view(sv.data(), siz + 1);
                    }
                }
#else
                auto [ptr, _] = std::to_chars(buf.data(), buf.data() + buf.size(), in, std::chars_format::general, sizeof(T) == 4 ? 7 : 16);
                sv = std::string_view(buf.data(), ptr - buf.data());
#endif
                s.append(sv);
            }
            else {
                if constexpr (std::is_same_v<char32_t, std::decay_t<T>>) {
                    char buf[8];
                    auto siz = Char32ToUtf8(in, buf);
                    s.append(std::string_view(buf, siz));
                }
                else {
                    //s.append(std::to_string(in));
                    std::array<char, 40> buf;
                    auto [ptr, _] = std::to_chars(buf.data(), buf.data() + buf.size(), in);
                    s.append(std::string_view(buf.data(), ptr - buf.data()));
                }
            }
        }
    };

    // 适配 enum( 根据原始数据类型调上面的适配 )
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_enum_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append(std::to_string((std::underlying_type_t<T>)in));
        }
    };

    // 适配 TimePoint
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsStdTimepoint_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            AppendTimePoint_Local(s, in);
        }
    };

    // 适配 std::optional
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsStdOptional_v<T>>> {
        static inline void Append(std::string &s, T const &in) {
            if (in.has_value()) {
                ::xx::Append(s, in.value());
            } else {
                s.append("null");
            }
        }
    };

    // 适配 std::????set list std::vector std::array
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsStdSetLike_v<T> || IsStdArray_v<T> || IsStdList_v<T> || IsStdVector_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.push_back('[');
            if (!in.empty()) {
                for(auto&& o : in) {
                    ::xx::Append(s, o);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };

    // 适配 std::????map
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsStdMapLike_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.push_back('[');
            if (!in.empty()) {
                for (auto &kv : in) {
                    ::xx::Append(s, kv.first);
                    s.push_back(',');
                    ::xx::Append(s, kv.second);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };

    // 适配 std::pair
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsStdPair_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.push_back('[');
            ::xx::Append(s, in.first);
            s.push_back(',');
            ::xx::Append(s, in.second);
            s.push_back(']');
        }
    };
	
    // 适配 std::tuple
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsStdTuple_v<T>>> {
        static inline void Append(std::string &s, T const &in) {
            s.push_back('[');
            std::apply([&](auto const &... args) {
                (::xx::Append(s, args, ','), ...);
                if constexpr(sizeof...(args) > 0) {
                    s.resize(s.size() - 1);
                }
            }, in);
            s.push_back(']');
        }
    };

    // 适配 std::variant
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsStdVariant_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            std::visit([&](auto const& v) {
                ::xx::Append(s, v);
            }, in);
        }
    };

    // 适配 Span, Data_r, Data_rw
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<Span, T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.push_back('[');
            if (auto inLen = in.len) {
                for (size_t i = 0; i < inLen; ++i) {
                    ::xx::Append(s, (uint8_t)in[i]);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };

    // 适配 FromTo
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsFromTo_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, '[', in.from, ", ", in.to, ']');
        }
    };

    // 适配 CurrentMax
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsCurrentMax_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            ::xx::Append(s, '[', in.current, ", ", in.max, ']');
        }
    };


    /************************************************************************************/
    // utils
    /************************************************************************************/

    template<typename S>
    inline size_t StrLen(S const& s) {
        if constexpr (std::is_pointer_v<S>) {
            if (!s) return 0;
            using C = std::remove_cvref_t<std::remove_pointer_t<S>>;
            return std::basic_string_view<C>(s).size();
        }
        else  if constexpr (IsLiteral_v<S>) {
            return LiteralLen<S> - 1;
        }
        else {
            return s.size();
        }
    }

    constexpr std::string_view base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"sv;

    inline std::string Base64Encode(std::string_view const& in) {
        std::string out;
        int val = 0, valb = -6;
        for (uint8_t c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(base64chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(base64chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    inline std::string Base64Decode(std::string_view const& in) {
        std::string out;
        std::array<int, 256> T;
        T.fill(-1);
        for (int i = 0; i < 64; i++) T[base64chars[i]] = i;
        int val = 0, valb = -8;
        for (uint8_t c : in) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    constexpr std::string_view intToStringChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"sv;

    // can't support negative interger now
    template<bool sClear = true, size_t fixedSize = 0, char fixedFill = '0', int toBase = 10, typename N, typename T>
    size_t IntToStringTo(T& s, N i) {
        constexpr int bufSiz{ sizeof(N) * 4 };
        std::array<char, bufSiz> buf;

        int idx{ bufSiz - 1 };
        while (i >= (N)toBase) {
            auto a = i / (N)toBase;
            auto b = i - a * (N)toBase;
            buf[idx--] = intToStringChars[b];
            i = a;
        }
        buf[idx] = intToStringChars[i];

        size_t numLen = bufSiz - idx;
        size_t len = numLen;
        if constexpr (fixedSize) {
            if (fixedSize > numLen) {
                len = fixedSize;
            }
        }

        if constexpr (sClear) {
            s.clear();
            s.reserve(len);
        } else {
            s.reserve(s.size() + len);
        }

        if constexpr (fixedSize) {
            if (fixedSize > numLen) {
                s.append(fixedSize - numLen, fixedFill);
            }
        }
        if constexpr (sizeof(typename T::value_type) == 1) {
            s.append((typename T::value_type*)&buf[0] + idx, numLen);
        } else {
            for (; idx < bufSiz; ++idx) {
                s.push_back((typename T::value_type)buf[idx]);
            }
        }

        return (size_t)len;
    }

    template<typename R = std::string, size_t fixedSize = 0, char fixedFill = '0', int toBase = 10, typename N>
    R IntToString(N i) {
        R s;
        ToStringTo<false, fixedSize, fixedFill, toBase>(s, i);
        return s;
    }


    struct ExpressionCalculator {
        int operator()(char const* exp_) {
            exp = exp_;
            return Expression();
        }
        template<typename S>
        int operator()(S const& exp_) {
            exp = exp_.data();
            return Expression();
        }
    protected:
        const char* exp{};
        char Peek() {
            return *exp;
        }
        char Get() {
            return *exp++;
        }
        int Number() {
            int result = Get() - '0';
            while (Peek() >= '0' && Peek() <= '9') {
                result = 10 * result + Get() - '0';
            }
            return result;
        }
        int Factor() {
            if (Peek() >= '0' && Peek() <= '9') return Number();
            else if (Peek() == '(') {
                Get(); // '('
                int result = Expression();
                Get(); // ')'
                return result;
            } else if (Peek() == '-') {
                Get();
                return -Factor();
            }
            return 0; // error
        }
        int Term() {
            int result = Factor();
            while (Peek() == '*' || Peek() == '/') {
                if (Get() == '*') {
                    result *= Factor();
                } else {
                    result /= Factor();
                }
            }
            return result;
        }
        int Expression() {
            int result = Term();
            while (Peek() == '+' || Peek() == '-') {
                if (Get() == '+') {
                    result += Term();
                } else {
                    result -= Term();
                }
            }
            return result;
        }
    };



    inline constexpr std::string_view TrimRight(std::string_view const& s) {
        auto idx = s.find_last_not_of(" \t\n\r\f\v");
        if (idx == std::string_view::npos) return { s.data(), 0 };
        return { s.data(), idx + 1 };
    }

    inline constexpr std::string_view TrimLeft(std::string_view const& s) {
        auto idx = s.find_first_not_of(" \t\n\r\f\v");
        if (idx == std::string_view::npos) return { s.data(), 0 };
        return { s.data() + idx, s.size() - idx };
    }

    inline constexpr std::string_view Trim(std::string_view const& s) {
        return TrimLeft(TrimRight(s));
    }

    template<size_t numDelimiters>
    inline constexpr std::string_view SplitOnce(std::string_view& sv, char const(&delimiters)[numDelimiters]) {
        static_assert(numDelimiters >= 2);
        auto data = sv.data();
        auto siz = sv.size();
        for (size_t i = 0; i != siz; ++i) {
            bool found;
            if constexpr (numDelimiters == 2) {
                found = sv[i] == delimiters[0];
            }
            else {
                found = std::string_view(delimiters).find(sv[i]) != std::string_view::npos;
            }
            if (found) {
                sv = std::string_view(data + i + 1, siz - i - 1);
                return {data, i};
            }
        }
        sv = std::string_view(data + siz, 0);
        return {data, siz};
    }

    template<typename T>
    inline constexpr bool SvToNumber(std::string_view const& input, T& out) {
        auto&& r = std::from_chars(input.data(), input.data() + input.size(), out);
        return r.ec != std::errc::invalid_argument && r.ec != std::errc::result_out_of_range;
    }

    template<typename T>
    inline constexpr T SvToNumber(std::string_view const& input, T const& defaultValue) {
        T out;
        auto&& r = std::from_chars(input.data(), input.data() + input.size(), out);
        return r.ec != std::errc::invalid_argument && r.ec != std::errc::result_out_of_range ? out : defaultValue;
    }

    template<typename T>
    inline constexpr std::optional<T> SvToNumber(std::string_view const& input) {
        T out;
        auto&& r = std::from_chars(input.data(), input.data() + input.size(), out);
        return r.ec != std::errc::invalid_argument && r.ec != std::errc::result_out_of_range ? out : std::optional<T>{};
    }

    template<typename T>
    inline std::string_view ToStringView(T const& v, char* buf, size_t len) {
        static_assert(std::is_integral_v<T>);
        if (auto [ptr, ec] = std::to_chars(buf, buf + len, v); ec == std::errc()) {
            return { buf, size_t(ptr - buf) };
        }
        else return {};
    }

    template<typename T, size_t len>
    inline std::string_view ToStringView(T const& v, char(&buf)[len]) {
        return ToStringView<T>(v, buf, len);
    }

    // 转换 s 数据类型 为 T 填充 dst
    template<typename T>
    inline void Convert(char const* s, T& dst) {
        if (!s) {
            dst = T();
        }
        else if constexpr (std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) <= 4) {
            dst = (T)strtoul(s, nullptr, 0);
        }
        else if constexpr (std::is_integral_v<T> && !std::is_unsigned_v<T> && sizeof(T) <= 4) {
            dst = (T)atoi(s);
        }
        else if constexpr (std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) == 8) {
            dst = strtoull(s, nullptr, 0);
        }
        else if constexpr (std::is_integral_v<T> && !std::is_unsigned_v<T> && sizeof(T) == 8) {
            dst = atoll(s);
        }
        else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4) {
            dst = strtof(s, nullptr);
        }
        else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8) {
            dst = atof(s);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            dst = s[0] == '1' || s[0] == 't' || s[0] == 'T' || s[0] == 'y' || s[0] == 'Y';
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            dst = s;
        }
        // todo: more
    }


    inline int FromHex(uint8_t c) {
        if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
        else if (c >= 'a' && c <= 'z') return c - 'a' + 10;
        else if (c >= '0' && c <= '9') return c - '0';
        else return 0;
    }
    inline uint8_t FromHex(char const* c) {
        return ((uint8_t)FromHex(c[0]) << 4) | (uint8_t)FromHex(c[1]);
    }

    inline void ToHex(uint8_t c, uint8_t& h1, uint8_t& h2) {
        auto a = c / 16;
        auto b = c % 16;
        h1 = (uint8_t)(a + ((a <= 9) ? '0' : ('a' - 10)));
        h2 = (uint8_t)(b + ((b <= 9) ? '0' : ('a' - 10)));
    }

    inline void ToHex(std::string& s) {
        auto len = s.size();
        s.resize(len * 2);
        auto b = (uint8_t*)s.data();
        for (auto i = (ptrdiff_t)len - 1; i >= 0; --i) {
            xx::ToHex(b[i], b[i * 2], b[i * 2 + 1]);
        }
    }



    // 用 s 滚动异或 buf 内容. 注意传入 buf 需要字节对齐, 小尾适用
    inline void XorContent(uint64_t s, char* buf, size_t len) {
        auto p = (char*)&s;
        auto left = len % sizeof(s);                                                                        // 余数 ( 这里假定 buf 一定会按 4/8 字节对齐, 小尾 )
        size_t i = 0;
        for (; i < len - left; i += sizeof(s)) {                                                            // 把字节对齐的部分肏了
            *(uint64_t*)&buf[i] ^= s;
        }
        for (auto j = i; i < len; ++i) {                                                                    // 余下部分单字节肏
            buf[i] ^= p[i - j];
        }
    }

    // 用 s 滚动异或 b 内容
    inline void XorContent(char const* s, size_t slen, char* b, size_t blen) {
        auto e = b + blen;
        for (size_t i = 0; b < e; *b++ ^= s[i++]) {
            if (i == slen) i = 0;
        }
    }




    // 移除文件路径部分只剩文件名
    inline int RemovePath(std::string& s) {
        auto b = s.data();
        auto e = (int)s.size() - 1;
        for (int i = e; i >= 0; --i) {
            if (b[i] == '/' || b[i] == '\\') {
                memmove(b, b + i + 1, e - i);
                s.resize(e - i);
                return i + 1;
            }
        }
        return 0;
    }
	
    // 获取 1, 2 级文件扩展名
    inline std::pair<std::string_view, std::string_view> GetFileNameExts(std::string const& fn) {
        std::pair<std::string_view, std::string_view> rtv;
        auto dotPos = fn.rfind('.');
        auto extLen = fn.size() - dotPos;
        rtv.first = std::string_view(fn.data() + dotPos, extLen);
        if (dotPos) {
            dotPos = fn.rfind('.', dotPos - 1);
            if(dotPos != std::string::npos) {
                extLen = fn.size() - dotPos - extLen;
                rtv.second = std::string_view(fn.data() + dotPos, extLen);
            }
        }
        return rtv;
    }


    // 将 string 里数字部分转为 n 字节定长（前面补0）后返回( 方便排序 ). 不支持小数
    inline std::string InnerNumberToFixed(std::string_view const& s, int n = 16) {
        std::string t, d;
        bool handleDigit = false;
        for (auto&& c : s) {
            if (c >= '0' && c <= '9') {
                if (!handleDigit) {
                    handleDigit = true;
                }
                d.append(1, c);
            }
            else {
                if (handleDigit) {
                    handleDigit = false;
                    t.append(n - d.size(), '0');
                    t.append(d);
                    d.clear();
                }
                else {
                    t.append(1, c);
                }
            }
        }
        if (handleDigit) {
            handleDigit = false;
            t.append(n - d.size(), '0');
            t.append(d);
            d.clear();
        }
        return t;
    }



    /************************************************************************************/
    // 各种 Cout
    /************************************************************************************/

    // 替代 std::cout. 支持实现了 StringFuncs 模板适配的类型
    template<typename...Args>
    inline void Cout(Args const& ...args) {
        std::string s;
        Append(s, args...);
        for (auto&& c : s) {
            if (!c) c = '^';
        }
        // std::cout << s;
        printf("%s", s.c_str());
    }

    // 在 Cout 基础上添加了换行
    template<typename...Args>
    inline void CoutN(Args const& ...args) {
        Cout(args...);
        //std::cout << std::endl;
        puts("");
    }

    // 在 CoutN 基础上于头部添加了时间
    template<typename...Args>
    inline void CoutTN(Args const& ...args) {
        CoutN("[", std::chrono::system_clock::now(), "] ", args...);
    }

    // 立刻输出
    inline void CoutFlush() {
        //std::cout.flush();
        fflush(stdout);
    }

    // 带 format 格式化的 Cout
    template<typename...Args>
    inline void CoutFormat(char const* const& format, Args const& ...args) {
        std::string s;
        AppendFormat(s, format, args...);
        for (auto&& c : s) {
            if (!c) c = '^';
        }
        //std::cout << s;
        printf("%s", s.c_str());
    }

    // 在 CoutFormat 基础上添加了换行
    template<typename...Args>
    inline void CoutNFormat(char const* const& format, Args const& ...args) {
        CoutFormat(format, args...);
        //std::cout << std::endl;
        puts("");
    }

    // 在 CoutNFormat 基础上于头部添加了时间
    template<typename...Args>
    inline void CoutTNFormat(char const* const& format, Args const& ...args) {
        CoutNFormat("[", std::chrono::system_clock::now(), "] ", args...);
    }
}
