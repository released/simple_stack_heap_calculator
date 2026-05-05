#include "detector/map_format_detector.h"

#include <algorithm>
#include <map>

#include "util/file_util.h"
#include "util/string_util.h"

namespace {

struct WeightedFingerprint {
    MapFormat format = MapFormat::Unknown;
    int weight = 0;
    std::wstring description;
    std::vector<std::string> requiredTokens;
};

struct ScoreState {
    int score = 0;
    std::vector<std::wstring> matchedFingerprints;
};

bool MatchesFingerprint(const std::vector<std::string>& upperLines,
                        const WeightedFingerprint& fingerprint) {
    for (const std::string& line : upperLines) {
        bool allMatched = true;
        for (const std::string& token : fingerprint.requiredTokens) {
            if (line.find(token) == std::string::npos) {
                allMatched = false;
                break;
            }
        }
        if (allMatched) {
            return true;
        }
    }
    return false;
}

std::vector<WeightedFingerprint> BuildFingerprints() {
    return {
        {MapFormat::KeilArmccArmclang, 8, L"Program Size: Code= summary", {"PROGRAM SIZE:", "CODE="}},
        {MapFormat::KeilArmccArmclang, 6, L"Memory Map of the image header", {"MEMORY MAP OF THE IMAGE"}},
        {MapFormat::KeilArmccArmclang, 6, L"Execution Region layout", {"EXECUTION REGION"}},
        {MapFormat::KeilArmccArmclang, 5, L"Load Region layout", {"LOAD REGION"}},
        {MapFormat::KeilArmccArmclang, 4, L"Image Symbol Table section", {"IMAGE SYMBOL TABLE"}},

        {MapFormat::GccGnuLd, 8, L"GNU ld Memory Configuration table", {"MEMORY CONFIGURATION"}},
        {MapFormat::GccGnuLd, 8, L"GNU ld linker script memory map", {"LINKER SCRIPT AND MEMORY MAP"}},
        {MapFormat::GccGnuLd, 6, L"Discarded input sections list", {"DISCARDED INPUT SECTIONS"}},
        {MapFormat::GccGnuLd, 5, L"GNU ld LOADADDR / ASSERT style symbols", {"LOADADDR"}},
        {MapFormat::GccGnuLd, 4, L"GNU ld ASSERT region overflow check", {"ASSERT (", "REGION"}},

        {MapFormat::IarIlink, 8, L"IAR ILINK banner", {"ILINK"}},
        {MapFormat::IarIlink, 6, L"IAR placement summary", {"PLACEMENT SUMMARY"}},
        {MapFormat::IarIlink, 5, L"IAR module summary", {"MODULE SUMMARY"}},
        {MapFormat::IarIlink, 5, L"IAR section list", {"SECTION LIST"}},
        {MapFormat::IarIlink, 4, L"IAR toolchain banner", {"IAR"}},

        {MapFormat::RenesasCcRl, 8, L"CC-RL toolchain banner", {"CC-RL"}},
        {MapFormat::RenesasCcRl, 7, L"RL78 architecture marker", {"RL78"}},
        {MapFormat::RenesasCcRl, 6, L"RL78 stack address symbols", {"__STACK_ADDR_START"}},
        {MapFormat::RenesasCcRl, 5, L"RL78 RAM address symbols", {"__RAM_ADDR_START"}},
        {MapFormat::RenesasCcRl, 4, L"Renesas rlink mapping list", {"*** MAPPING LIST ***"}},

        {MapFormat::RenesasCcRh, 8, L"CC-RH toolchain banner", {"CC-RH"}},
        {MapFormat::RenesasCcRh, 7, L"RH850 architecture marker", {"RH850"}},
        {MapFormat::RenesasCcRh, 6, L"RH850 stack section symbols", {"__S.STACK.BSS"}},
        {MapFormat::RenesasCcRh, 5, L"RH850 V850 library marker", {"V850"}},
        {MapFormat::RenesasCcRh, 4, L"Renesas rlink mapping list", {"*** MAPPING LIST ***"}},

        {MapFormat::Ghs, 8, L"Green Hills banner", {"GREEN HILLS"}},
        {MapFormat::Ghs, 7, L"MULTI environment marker", {"MULTI"}},
        {MapFormat::Ghs, 5, L"GHS linker section summary pattern", {"LINKER", "SECTION"}},
        {MapFormat::Ghs, 4, L"GHS memory map phrasing", {"MEMORY MAP"}},
    };
}

int CalculateConfidenceScore(int bestScore, int secondBestScore) {
    if (bestScore <= 0) {
        return 0;
    }

    int confidence = bestScore * 5;
    const int margin = bestScore - secondBestScore;
    confidence += margin * 4;
    if (bestScore < 10) {
        confidence -= 20;
    }
    if (margin <= 2) {
        confidence -= 20;
    }
    if (confidence < 0) {
        confidence = 0;
    }
    if (confidence > 100) {
        confidence = 100;
    }
    return confidence;
}

}  // namespace

MapFormatDetectionResult MapFormatDetector::DetectFromFile(const std::wstring& mapPath) const {
    std::vector<std::string> lines;
    std::wstring readError;
    if (!fileutil::ReadAllLines(mapPath, &lines, &readError)) {
        MapFormatDetectionResult result;
        result.detectedFormat = MapFormat::Unknown;
        result.warnings.push_back(L"Detection could not read the map file: " + readError);
        return result;
    }
    return DetectFromLines(lines);
}

MapFormatDetectionResult MapFormatDetector::DetectFromLines(const std::vector<std::string>& lines) const {
    MapFormatDetectionResult result;
    const std::vector<WeightedFingerprint> fingerprints = BuildFingerprints();

    std::vector<std::string> upperLines;
    upperLines.reserve(lines.size());
    for (const std::string& line : lines) {
        upperLines.push_back(stringutil::ToUpperCopy(line));
    }

    std::map<MapFormat, ScoreState> scores;
    for (const WeightedFingerprint& fingerprint : fingerprints) {
        if (!MatchesFingerprint(upperLines, fingerprint)) {
            continue;
        }
        ScoreState& state = scores[fingerprint.format];
        state.score += fingerprint.weight;
        state.matchedFingerprints.push_back(fingerprint.description);
    }

    MapFormat bestFormat = MapFormat::Unknown;
    int bestScore = 0;
    int secondBestScore = 0;
    for (const auto& entry : scores) {
        const int score = entry.second.score;
        if (score > bestScore) {
            secondBestScore = bestScore;
            bestScore = score;
            bestFormat = entry.first;
        } else if (score > secondBestScore) {
            secondBestScore = score;
        }
    }

    if (bestScore < 8) {
        result.detectedFormat = MapFormat::Unknown;
        result.confidenceScore = CalculateConfidenceScore(bestScore, secondBestScore);
        result.warnings.push_back(L"No strong map-format fingerprints were found.");
        return result;
    }

    result.detectedFormat = bestFormat;
    result.confidenceScore = CalculateConfidenceScore(bestScore, secondBestScore);
    result.matchedFingerprints = scores[bestFormat].matchedFingerprints;

    if (result.confidenceScore < 60) {
        result.warnings.push_back(L"Detection confidence is low. Manual parser selection should be treated as the fallback.");
    }
    if (bestScore - secondBestScore <= 2 && secondBestScore > 0) {
        result.warnings.push_back(L"Multiple map formats produced similar fingerprint scores.");
    }
    if (result.matchedFingerprints.empty()) {
        result.warnings.push_back(L"The winning format had no recorded fingerprint descriptions.");
    }

    return result;
}

std::optional<CompilerFamily> TryMapFormatToCompilerFamily(MapFormat format) {
    switch (format) {
    case MapFormat::KeilArmccArmclang:
        return CompilerFamily::Keil;
    case MapFormat::GccGnuLd:
        return CompilerFamily::Gcc;
    case MapFormat::IarIlink:
        return CompilerFamily::Iar;
    case MapFormat::RenesasCcRl:
        return CompilerFamily::Rl78;
    case MapFormat::RenesasCcRh:
        return CompilerFamily::Rh850;
    case MapFormat::Ghs:
        return CompilerFamily::Ghs;
    default:
        return std::nullopt;
    }
}
