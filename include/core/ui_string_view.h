﻿#pragma once

// C++
#include <cstdint>
#include <cstring>
#include <cwchar>
// Util
#include "../util/ui_endian.h"
#include "../util/ui_unimacro.h"


namespace LongUI {
    // detail namespace
    namespace detail {
        // string length
        inline auto strlen(const char ptr[]) noexcept->size_t { return std::strlen(ptr); }
        // string length
        inline auto strlen(const wchar_t ptr[]) noexcept->size_t { return std::wcslen(ptr); }
        // string length
        auto strlen(const char16_t ptr[]) noexcept->size_t;
        // string length
        auto strlen(const char32_t ptr[]) noexcept->size_t;
        // name to rgb32
        auto name_rgb32(const char* a, const char* b, char c) noexcept->uint32_t;
        // name to rgba32
        auto color_rgba32(const char* a, const char* b, char c) noexcept->uint32_t;
    }
    // string view
    template<typename T> struct PodStringView {
        // from c-tyle string
        static auto FromCStyle(const T str[]) noexcept -> PodStringView { return{ str, str + detail::strlen(str) }; };
        // size
        auto size() const noexcept -> uint32_t { return static_cast<uint32_t>(end() - begin()); }
        // begin
        auto begin() const noexcept ->const T*{ return first; };
        // end
        auto end() const noexcept ->const T*{ return second; };
        // is true?
        operator bool() noexcept { return *begin() == 't'; }
        // to RGBA32 in byte order
        auto ColorRGBA32() const noexcept->uint32_t {
            return detail::color_rgba32(
                reinterpret_cast<const char*>(first) + helper::ascii_offset<sizeof(T)>::value,
                reinterpret_cast<const char*>(second) + helper::ascii_offset<sizeof(T)>::value,
                sizeof(T)
            );}
        // to RGBA32 in byte order
        auto NamedRGB32() const noexcept->uint32_t { 
            return detail::name_rgb32(
                reinterpret_cast<const char*>(first) + helper::ascii_offset<sizeof(T)>::value,
                reinterpret_cast<const char*>(second) + helper::ascii_offset<sizeof(T)>::value,
                sizeof(T)
            );}
        // get char then move
        //auto Char32() noexcept -> char32_t;
        // to rgba32
        //auto NamedRGBA32() const noexcept -> uint32_t { (NamedRGB32() << 8) | 0xff; }
        // split then move
        auto Split(T ch) noexcept->PodStringView;
        // to float
        operator float() const noexcept;
        // to double
        operator double() const noexcept;
        // to int32_t
        operator int32_t() const noexcept;
        // to float
        auto ToFloat() const noexcept -> float { return static_cast<float>(*this); }
        // to int32_t
        auto ToInt32() const noexcept -> int32_t { return static_cast<int32_t>(*this); }
        // to double
        auto ToDouble() const noexcept -> double { return static_cast<double>(*this); }
        // 1st
        const T*        first;
        // 2nd
        const T*        second;
    };
    // _sv
    inline PodStringView<char> operator ""_sv(const char* str, size_t len) noexcept {
        return{ str , str + len };
    }
    // _sv
    inline PodStringView<wchar_t> operator ""_sv(const wchar_t* str, size_t len) noexcept {
        return{ str , str + len };
    }
    // _sv
    inline PodStringView<char16_t> operator ""_sv(const char16_t* str, size_t len) noexcept {
        return{ str , str + len };
    }
    // _sv
    inline PodStringView<char32_t> operator ""_sv(const char32_t* str, size_t len) noexcept {
        return{ str , str + len };
    }
}
// HELPER MACRO
#define UI_DECLARE_METHOD_FOR_CHAR_TYPE(T) \
    template<> PodStringView<T>::operator float() const noexcept;\
    template<> PodStringView<T>::operator double() const noexcept;\
    template<> PodStringView<T>::operator int32_t() const noexcept;\
    template<> PodStringView<T> PodStringView<T>::Split(T ch) noexcept;

namespace LongUI {
    // char
    UI_DECLARE_METHOD_FOR_CHAR_TYPE(char);
    // wchar
    UI_DECLARE_METHOD_FOR_CHAR_TYPE(wchar_t);
    // char16_t
    UI_DECLARE_METHOD_FOR_CHAR_TYPE(char16_t);
    // char32_t
    UI_DECLARE_METHOD_FOR_CHAR_TYPE(char32_t);
}
#undef UI_DECLARE_METHOD_FOR_CHAR_TYPE