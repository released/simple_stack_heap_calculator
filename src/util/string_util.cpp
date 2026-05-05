#include "util/string_util.h"

#include <algorithm>
#include <cwctype>
#include <sstream>

std::wstring stringutil::Trim(const std::wstring& text) {
    size_t start = 0;
    while (start < text.size() && iswspace(text[start])) {
        ++start;
    }

    size_t end = text.size();
    while (end > start && iswspace(text[end - 1])) {
        --end;
    }

    return text.substr(start, end - start);
}

std::string stringutil::Trim(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    size_t end = text.size();
    while (end > start && isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

std::wstring stringutil::ToUpperCopy(const std::wstring& text) {
    std::wstring value = text;
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) { return static_cast<wchar_t>(towupper(ch)); });
    return value;
}

std::string stringutil::ToUpperCopy(const std::string& text) {
    std::string value = text;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(toupper(ch)); });
    return value;
}

bool stringutil::ContainsInsensitive(const std::wstring& text, const std::wstring& needle) {
    return ToUpperCopy(text).find(ToUpperCopy(needle)) != std::wstring::npos;
}

bool stringutil::ContainsInsensitive(const std::string& text, const std::string& needle) {
    return ToUpperCopy(text).find(ToUpperCopy(needle)) != std::string::npos;
}

std::vector<std::string> stringutil::SplitWhitespace(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::wstring stringutil::JoinLines(const std::vector<std::wstring>& lines) {
    std::wstring result;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            result += L"\r\n";
        }
        result += lines[i];
    }
    return result;
}

bool stringutil::TryParseInteger(const std::wstring& text, std::uint64_t* out_value) {
    if (!out_value) {
        return false;
    }

    std::wstring trimmed = Trim(text);
    if (trimmed.empty()) {
        return false;
    }

    wchar_t* end = nullptr;
    unsigned long long value = wcstoull(trimmed.c_str(), &end, 10);
    if (!end || *end != L'\0') {
        return false;
    }

    *out_value = static_cast<std::uint64_t>(value);
    return true;
}

bool stringutil::TryParseHexOrDecimal(const std::wstring& text, std::uint64_t* out_value) {
    if (!out_value) {
        return false;
    }

    std::wstring trimmed = Trim(text);
    if (trimmed.empty()) {
        return false;
    }

    int base = 10;
    const wchar_t* start = trimmed.c_str();
    if (trimmed.size() > 2 && trimmed[0] == L'0' && (trimmed[1] == L'x' || trimmed[1] == L'X')) {
        base = 16;
    }

    wchar_t* end = nullptr;
    unsigned long long value = wcstoull(start, &end, base);
    if (!end || *end != L'\0') {
        return false;
    }

    *out_value = static_cast<std::uint64_t>(value);
    return true;
}
