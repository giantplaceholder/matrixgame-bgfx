#pragma once

// GCC 12 libstdc++ has no std::format. This covers the call shapes used by MatrixGame.

#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace mgfmt {

inline std::string timestamp(const char* fmt) {
    using clock = std::chrono::system_clock;
    const auto now = clock::to_time_t(clock::now());
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    char buf[64] = {};
    std::strftime(buf, sizeof(buf), fmt, &tm);
    return buf;
}

inline std::wstring timestamp_w(const wchar_t* fmt) {
    using clock = std::chrono::system_clock;
    const auto now = clock::to_time_t(clock::now());
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    wchar_t buf[64] = {};
    std::wcsftime(buf, sizeof(buf) / sizeof(buf[0]), fmt, &tm);
    return buf;
}

template <typename T>
std::string stringify(const T& value, bool hex = false) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        return value;
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
        return std::string(value);
    } else if constexpr (std::is_convertible_v<T, std::string_view> && !std::is_arithmetic_v<std::decay_t<T>>) {
        return std::string(std::string_view(value));
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::chrono::system_clock::time_point>) {
        return timestamp("%Y-%m-%d %H:%M:%S");
    } else {
        std::ostringstream out;
        if (hex) {
            out << std::hex << std::uppercase << value;
        } else {
            out << value;
        }
        return out.str();
    }
}

inline std::string format_rest(std::string_view pattern) {
    return std::string(pattern);
}

template <typename T, typename... Rest>
std::string format_rest(std::string_view pattern, const T& value, const Rest&... rest) {
    const auto open = pattern.find('{');
    if (open == std::string_view::npos) {
        return std::string(pattern);
    }
    const auto close = pattern.find('}', open);
    const bool hex = close != std::string_view::npos && pattern.substr(open, close - open).find(":X") != std::string_view::npos;
    std::string out(pattern.substr(0, open));
    out += stringify(value, hex);
    if (close == std::string_view::npos) {
        return out;
    }
    out += format_rest(pattern.substr(close + 1), rest...);
    return out;
}

template <typename... Args>
std::string format(std::string_view pattern, const Args&... args) {
    if (pattern.size() >= 2 && pattern[0] == '{' && pattern[1] == ':') {
        return timestamp("%Y-%m-%d %H:%M:%S");
    }
    return format_rest(pattern, args...);
}

template <typename... Args>
std::wstring format(std::wstring_view pattern, const Args&... args) {
    if (pattern.find(L"{:") == 0) {
        return timestamp_w(L"%Y-%m-%d-%H-%M-%S");
    }
    std::wstring out;
    out.reserve(pattern.size() + 32);
    std::size_t cursor = 0;
    auto take = [&](auto&& value, bool hex) {
        using V = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<V, std::chrono::system_clock::time_point>) {
            const auto stamp = timestamp("%Y-%m-%d-%H-%M-%S");
            out.append(stamp.begin(), stamp.end());
        } else {
            std::ostringstream narrow;
            if (hex) {
                narrow << std::hex << std::uppercase;
            }
            narrow << value;
            const auto text = narrow.str();
            out.append(text.begin(), text.end());
        }
    };
    auto append_one = [&](auto&& value) {
        const auto open = pattern.find(L'{', cursor);
        if (open == std::wstring_view::npos) {
            out.append(pattern.substr(cursor));
            cursor = pattern.size();
            return;
        }
        out.append(pattern.substr(cursor, open - cursor));
        const auto close = pattern.find(L'}', open);
        const bool hex = close != std::wstring_view::npos && pattern.substr(open, close - open).find(L":X") != std::wstring_view::npos;
        if constexpr (std::is_convertible_v<std::decay_t<decltype(value)>, std::wstring>) {
            out += std::wstring(value);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::wstring_view>) {
            out += std::wstring(value);
        } else {
            take(value, hex);
        }
        cursor = close == std::wstring_view::npos ? pattern.size() : close + 1;
    };
    (append_one(args), ...);
    if (cursor < pattern.size()) {
        out.append(pattern.substr(cursor));
    }
    return out;
}

}  // namespace mgfmt
