#pragma once

#include <string>
#include <vector>

enum class MapFormat {
    KeilArmccArmclang = 0,
    GccGnuLd,
    IarIlink,
    RenesasCcRl,
    RenesasCcRh,
    Ghs,
    Unknown,
};

inline std::wstring MapFormatToDisplayName(MapFormat format) {
    switch (format) {
    case MapFormat::KeilArmccArmclang:
        return L"KEIL_ARMCC_ARMCLANG";
    case MapFormat::GccGnuLd:
        return L"GCC_GNU_LD";
    case MapFormat::IarIlink:
        return L"IAR_ILINK";
    case MapFormat::RenesasCcRl:
        return L"RENESAS_CC_RL";
    case MapFormat::RenesasCcRh:
        return L"RENESAS_CC_RH";
    case MapFormat::Ghs:
        return L"GHS";
    default:
        return L"UNKNOWN";
    }
}

struct MapFormatDetectionResult {
    MapFormat detectedFormat = MapFormat::Unknown;
    int confidenceScore = 0;
    std::vector<std::wstring> matchedFingerprints;
    std::vector<std::wstring> warnings;
};
