#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace stringutil {

std::wstring Trim(const std::wstring& text);
std::string Trim(const std::string& text);
std::wstring ToUpperCopy(const std::wstring& text);
std::string ToUpperCopy(const std::string& text);
bool ContainsInsensitive(const std::wstring& text, const std::wstring& needle);
bool ContainsInsensitive(const std::string& text, const std::string& needle);
std::vector<std::string> SplitWhitespace(const std::string& text);
std::wstring JoinLines(const std::vector<std::wstring>& lines);
bool TryParseInteger(const std::wstring& text, std::uint64_t* out_value);
bool TryParseHexOrDecimal(const std::wstring& text, std::uint64_t* out_value);

}
