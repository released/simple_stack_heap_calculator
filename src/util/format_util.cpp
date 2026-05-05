#include "util/format_util.h"

#include <iomanip>
#include <sstream>

std::wstring formatutil::Hex(std::uint64_t value) {
    std::wstringstream stream;
    stream << L"0x" << std::uppercase << std::hex << value;
    return stream.str();
}

std::wstring formatutil::HexOrNa(const std::optional<std::uint64_t>& value) {
    if (!value.has_value()) {
        return L"N/A";
    }
    return Hex(*value);
}

std::wstring formatutil::BytesWithHex(std::uint64_t value) {
    std::wstringstream stream;
    stream << Hex(value) << L" (" << std::dec << value << L" bytes)";
    return stream.str();
}

std::wstring formatutil::Percent(double value) {
    std::wstringstream stream;
    stream << std::fixed << std::setprecision(1) << value << L"%";
    return stream.str();
}

std::uint64_t formatutil::RoundUpToUnit(std::uint64_t value, std::uint64_t unitBytes) {
    if (unitBytes == 0) {
        return value;
    }
    std::uint64_t remainder = value % unitBytes;
    if (remainder == 0) {
        return value;
    }
    return value + (unitBytes - remainder);
}

std::uint64_t formatutil::AddPercentRoundedUp(std::uint64_t baseValue, int percent, std::uint64_t unitBytes) {
    if (percent <= 0) {
        return RoundUpToUnit(baseValue, unitBytes);
    }

    const std::uint64_t rawValue = baseValue + ((baseValue * static_cast<std::uint64_t>(percent)) + 99ULL) / 100ULL;
    return RoundUpToUnit(rawValue, unitBytes);
}
