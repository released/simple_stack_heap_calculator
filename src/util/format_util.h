#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace formatutil {

std::wstring Hex(std::uint64_t value);
std::wstring HexOrNa(const std::optional<std::uint64_t>& value);
std::wstring BytesWithHex(std::uint64_t value);
std::wstring Percent(double value);
std::uint64_t RoundUpToUnit(std::uint64_t value, std::uint64_t unitBytes);
std::uint64_t AddPercentRoundedUp(std::uint64_t baseValue, int percent, std::uint64_t unitBytes);

}
