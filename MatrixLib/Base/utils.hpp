#pragma once

#include <string>
#include <cstdio>
#include <stdexcept>
#include <locale>
#include <codecvt>
#include <cwchar>

namespace utils {
inline std::wstring safe_wstr(const wchar_t* text, ptrdiff_t len) {
    if (!text || len <= 0 || len > 1000000) return {};
    return std::wstring(text, static_cast<size_t>(len));
}

template<typename... Args>
inline std::string format(const char* format, Args... args)
{
    // TODO: replace with std::format/fmt::format
    char buf[10240];
    if (std::snprintf(buf, sizeof(buf), format, args...) < 0)
    {
        throw std::runtime_error("snprintf() failed");
    }
    return std::string{buf};
}

template<typename... Args>
inline std::wstring format(const wchar_t* format, Args... args)
{
    // TODO: replace with std::format/fmt::format
    wchar_t buf[10240];
    // swprintf takes a maximum number of wchar_t elements, not bytes. On Windows
    // wchar_t is 2 bytes and sizeof() happened to be wrong-but-harmless; on Linux
    // it is 4 and _FORTIFY_SOURCE aborts with "buffer overflow detected".
    if (std::swprintf(buf, std::size(buf), format, args...) < 0)
    {
        throw std::runtime_error("swprintf() failed");
    }
    return std::wstring{buf};
}

inline std::string from_wstring(std::wstring_view wstr)
{
    // Game paths are ASCII. codecvt_utf8_utf16<wchar_t> is wrong on Linux, where wchar_t is 32-bit.
    std::string out;
    out.reserve(wstr.size());
    for (wchar_t ch : wstr) {
        out.push_back(ch < 128 ? static_cast<char>(ch) : '?');
    }
    return out;
}

inline std::wstring to_wstring(std::string_view str)
{
    std::wstring out;
    out.reserve(str.size());
    for (unsigned char ch : str) {
        out.push_back(static_cast<wchar_t>(ch));
    }
    return out;
}

// TODO: replace with string::starts_with from C++20
inline bool starts_with(const std::wstring& src, const std::wstring& sub)
{
    return (src.find(sub) == 0);
}

inline void to_lower(std::wstring& str, size_t offset = 0)
{
    for (size_t i = offset; i < str.length(); ++i)
    {
        str[i] = towlower(str[i]);
    }
}

inline std::wstring trim(const std::wstring& str)
{
    const auto first = str.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return {};
    const auto last = str.find_last_not_of(L" \t\r\n");
    return str.substr(first, last - first + 1);
}

inline std::wstring& replace(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    size_t tlen = str.length();
    if (tlen < 1 || tlen < from.length())
    {
        return str;
    }

    size_t offset = 0;
    while (true)
    {
        offset = str.find(from, offset);
        if (offset == std::wstring::npos)
        {
            break;
        }
        str.replace(offset, from.length(), to);
        offset += to.length();
    }

    return str;
}

} // namespace utils
