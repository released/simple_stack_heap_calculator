#include "core/analysis_engine.h"

#include <memory>
#include <vector>

#include "detector/map_format_detector.h"
#include "parser/parser_factory.h"
#include "util/format_util.h"
#include "util/string_util.h"

namespace {

std::wstring BuildNarrative(const AnalysisResult& result) {
    std::vector<std::wstring> lines;
    lines.push_back(L"Detected Map Format: " + MapFormatToDisplayName(result.detection.detectedFormat) +
                    L" (" + std::to_wstring(result.detection.confidenceScore) + L"%)");
    lines.push_back(L"Parser Selection: " +
                    std::wstring(result.usedAutomaticParserSelection ? L"Automatic" : L"Manual fallback") +
                    L" -> " + result.compilerDisplayName);
    if (!result.detection.matchedFingerprints.empty()) {
        lines.push_back(L"Matched Fingerprints:");
        for (const std::wstring& fingerprint : result.detection.matchedFingerprints) {
            lines.push_back(L"  - " + fingerprint);
        }
        lines.push_back(L"");
    }
    lines.push_back(L"Compiler: " + result.compilerDisplayName);
    lines.push_back(L"");
    lines.push_back(L"Program Size");
    lines.push_back(L"  Code      = " + formatutil::BytesWithHex(result.programSize.codeBytes));
    lines.push_back(L"  RO-data   = " + formatutil::BytesWithHex(result.programSize.roDataBytes));
    lines.push_back(L"  RW-data   = " + formatutil::BytesWithHex(result.programSize.rwDataBytes));
    lines.push_back(L"  ZI-data   = " + formatutil::BytesWithHex(result.programSize.ziDataBytes));
    lines.push_back(L"  Total Flash Estimate = Code + RO-data + RW-data = " + formatutil::BytesWithHex(result.report.flashUsedBytes));
    lines.push_back(L"  Total RAM Static     = RW-data + ZI-data = " + formatutil::BytesWithHex(result.report.staticRamUsedBytes));
    lines.push_back(L"");
    lines.push_back(L"Stack / Heap");
    lines.push_back(L"  Stack Reserved = " + formatutil::BytesWithHex(result.stackHeapInfo.stackSizeBytes));
    lines.push_back(L"  Heap Reserved  = " + formatutil::BytesWithHex(result.stackHeapInfo.heapSizeBytes));
    lines.push_back(L"  Stack reserved / estimated = " + formatutil::BytesWithHex(result.stackHeapInfo.stackRecommendedBytes));
    lines.push_back(L"  Heap reserved / estimated  = " + formatutil::BytesWithHex(result.stackHeapInfo.heapRecommendedBytes));
    if (!result.stackHeapInfo.source.empty()) {
        lines.push_back(L"  Source         = " + result.stackHeapInfo.source);
    }
    if (!result.stackHeapInfo.confidence.empty()) {
        lines.push_back(L"  Confidence     = " + result.stackHeapInfo.confidence);
    }
    if (result.stackHeapInfo.stackSizeBytes > 0) {
        lines.push_back(L"  Stack range    = " + formatutil::Hex(result.stackHeapInfo.stackBase) + L" -> " + formatutil::Hex(result.stackHeapInfo.stackLimit));
    }
    if (result.stackHeapInfo.heapSizeBytes > 0) {
        lines.push_back(L"  Heap range     = " + formatutil::Hex(result.stackHeapInfo.heapBase) + L" -> " + formatutil::Hex(result.stackHeapInfo.heapLimit));
    }
    lines.push_back(L"");
    lines.push_back(L"RAM Estimate");
    lines.push_back(L"  Estimated RAM Required = " + formatutil::BytesWithHex(result.report.estimatedRamRequiredBytes));
    if (result.effectiveRamLimitBytes.has_value()) {
        lines.push_back(L"  RAM Limit             = " + formatutil::BytesWithHex(*result.effectiveRamLimitBytes));
        lines.push_back(L"  RAM Remaining         = RAM Limit - Estimated RAM Required = " + formatutil::BytesWithHex(result.report.ramRemainingBytes));
        lines.push_back(L"  RAM Usage             = " + formatutil::Percent(result.report.ramUsagePercent));
    } else {
        lines.push_back(L"  RAM Limit             = N/A");
        lines.push_back(L"  RAM Remaining         = N/A");
        lines.push_back(L"  RAM Usage             = N/A");
    }
    lines.push_back(L"  Risk Level            = " + result.report.riskLevel);
    lines.push_back(L"");
    lines.push_back(L"Memory Regions");
    if (result.memoryRegions.empty()) {
        lines.push_back(L"  No memory regions parsed.");
    } else {
        for (const MemoryRegion& region : result.memoryRegions) {
            std::wstring line = L"  " + region.name + L": base=" + formatutil::Hex(region.baseAddress) +
                                L", used=" + formatutil::BytesWithHex(region.usedBytes) +
                                L", size=" + formatutil::BytesWithHex(region.sizeBytes);
            lines.push_back(line);
        }
    }
    if (!result.report.warnings.empty()) {
        lines.push_back(L"");
        lines.push_back(L"Warnings");
        for (const std::wstring& warning : result.report.warnings) {
            lines.push_back(L"  - " + warning);
        }
    }
    return stringutil::JoinLines(lines);
}

}  // namespace

AnalysisResult AnalysisEngine::Run(const AnalysisRequest& request) const {
    AnalysisResult result;
    MapFormatDetector detector;
    result.detection = detector.DetectFromFile(request.mapPath);

    std::vector<std::wstring> preflightLog;
    preflightLog.push_back(L"Map detection started.");
    preflightLog.push_back(L"Detected format: " + MapFormatToDisplayName(result.detection.detectedFormat));
    preflightLog.push_back(L"Detection confidence: " + std::to_wstring(result.detection.confidenceScore) + L"%");
    for (const std::wstring& fingerprint : result.detection.matchedFingerprints) {
        preflightLog.push_back(L"Matched fingerprint: " + fingerprint);
    }
    for (const std::wstring& warning : result.detection.warnings) {
        preflightLog.push_back(L"Detection warning: " + warning);
    }

    CompilerFamily selectedFamily = request.manualFamily;
    if (request.preferDetectedParser && result.detection.confidenceScore >= 60) {
        const std::optional<CompilerFamily> detectedFamily = TryMapFormatToCompilerFamily(result.detection.detectedFormat);
        if (detectedFamily.has_value()) {
            selectedFamily = *detectedFamily;
            result.usedAutomaticParserSelection = true;
            preflightLog.push_back(L"Parser selection: automatic -> " + CompilerFamilyToDisplayName(selectedFamily));
        } else {
            preflightLog.push_back(L"Parser selection: no parser matches detected format, using manual fallback.");
        }
    } else {
        preflightLog.push_back(L"Parser selection: manual fallback -> " + CompilerFamilyToDisplayName(selectedFamily));
    }

    std::unique_ptr<IMapParser> parser = ParserFactory::Create(selectedFamily);
    if (!parser) {
        result.logLines = preflightLog;
        result.errorMessage = L"Parser factory failed to create parser.";
        return result;
    }

    MapParseData parsed = parser->Parse(ParseOptions{request.mapPath});
    result.logLines = preflightLog;
    result.logLines.insert(result.logLines.end(), parsed.logLines.begin(), parsed.logLines.end());
    if (!parsed.success) {
        result.compilerDisplayName = parsed.compilerDisplayName.empty() ? CompilerFamilyToDisplayName(selectedFamily) : parsed.compilerDisplayName;
        result.errorMessage = parsed.errorMessage.empty() ? L"Parser failed." : parsed.errorMessage;
        return result;
    }

    result.success = true;
    result.compilerDisplayName = parsed.compilerDisplayName;
    result.programSize = parsed.programSize;
    result.memoryRegions = parsed.memoryRegions;
    result.symbols = parsed.symbols;
    result.stackHeapInfo = parsed.stackHeapInfo;
    result.parsedRamLimitBytes = parsed.ramLimitBytes;

    result.report.flashUsedBytes = result.programSize.codeBytes + result.programSize.roDataBytes + result.programSize.rwDataBytes;
    result.report.staticRamUsedBytes = result.programSize.rwDataBytes + result.programSize.ziDataBytes;
    result.report.estimatedRamRequiredBytes = result.report.staticRamUsedBytes;
    if (!result.stackHeapInfo.stackIncludedInStaticRam) {
        result.report.estimatedRamRequiredBytes += result.stackHeapInfo.stackSizeBytes;
    } else if (result.stackHeapInfo.stackSizeBytes > 0) {
        result.logLines.push_back(L"Stack is already included in static RAM total. Estimated RAM does not add it twice.");
    }
    if (!result.stackHeapInfo.heapIncludedInStaticRam) {
        result.report.estimatedRamRequiredBytes += result.stackHeapInfo.heapSizeBytes;
    } else if (result.stackHeapInfo.heapSizeBytes > 0) {
        result.logLines.push_back(L"Heap is already included in static RAM total. Estimated RAM does not add it twice.");
    }

    if (request.ramLimitOverrideBytes.has_value()) {
        result.effectiveRamLimitBytes = request.ramLimitOverrideBytes;
    } else if (parsed.ramLimitBytes.has_value()) {
        result.effectiveRamLimitBytes = parsed.ramLimitBytes;
    }

    result.report.riskLevel = L"Unknown";
    result.report.warnings = parsed.warnings;
    result.report.warnings.insert(result.report.warnings.end(),
                                  result.detection.warnings.begin(),
                                  result.detection.warnings.end());

    if (result.effectiveRamLimitBytes.has_value()) {
        const std::uint64_t limit = *result.effectiveRamLimitBytes;
        if (limit >= result.report.estimatedRamRequiredBytes) {
            result.report.ramRemainingBytes = limit - result.report.estimatedRamRequiredBytes;
        } else {
            result.report.ramRemainingBytes = 0;
            result.report.warnings.push_back(L"Estimated RAM required exceeds RAM limit.");
        }
        if (limit > 0) {
            result.report.ramUsagePercent = static_cast<double>(result.report.estimatedRamRequiredBytes) * 100.0 /
                                            static_cast<double>(limit);
            if (result.report.ramUsagePercent >= 95.0) {
                result.report.riskLevel = L"High";
            } else if (result.report.ramUsagePercent >= 85.0) {
                result.report.riskLevel = L"Medium";
            } else {
                result.report.riskLevel = L"Low";
            }
        }
    } else {
        result.report.warnings.push_back(L"RAM limit is unknown. Fill the RAM Limit field or extend parser support for region capacity.");
    }

    if (result.stackHeapInfo.heapRecommendationEstimated) {
        result.report.warnings.push_back(L"Heap recommendation is estimated from a removed or inferred heap section.");
    }
    if (result.stackHeapInfo.stackRecommendationEstimated) {
        result.report.warnings.push_back(L"Stack recommendation is estimated from a removed or inferred stack section.");
    }

    result.narrative = BuildNarrative(result);
    return result;
}
