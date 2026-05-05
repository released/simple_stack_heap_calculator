#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "model/map_format_types.h"

struct ProgramSize {
    std::uint64_t codeBytes = 0;
    std::uint64_t roDataBytes = 0;
    std::uint64_t rwDataBytes = 0;
    std::uint64_t ziDataBytes = 0;
    bool hasCode = false;
    bool hasRoData = false;
    bool hasRwData = false;
    bool hasZiData = false;
};

struct MemoryRegion {
    std::wstring name;
    std::uint64_t baseAddress = 0;
    std::uint64_t sizeBytes = 0;
    std::uint64_t usedBytes = 0;
};

enum class SymbolType {
    Unknown = 0,
    Code,
    RoData,
    RwData,
    ZiData,
    Stack,
    Heap,
};

struct SymbolEntry {
    std::uint64_t address = 0;
    std::uint64_t sizeBytes = 0;
    SymbolType type = SymbolType::Unknown;
    std::wstring objectName;
    std::wstring libraryName;
    std::wstring symbolName;
};

struct StackHeapInfo {
    std::uint64_t stackBase = 0;
    std::uint64_t stackLimit = 0;
    std::uint64_t stackSizeBytes = 0;
    std::uint64_t stackRecommendedBytes = 0;
    std::uint64_t heapBase = 0;
    std::uint64_t heapLimit = 0;
    std::uint64_t heapSizeBytes = 0;
    std::uint64_t heapRecommendedBytes = 0;
    bool stackIncludedInStaticRam = false;
    bool heapIncludedInStaticRam = false;
    bool stackRecommendationEstimated = false;
    bool heapRecommendationEstimated = false;
    std::wstring source;
    std::wstring confidence;
};

struct AnalysisReport {
    std::uint64_t flashUsedBytes = 0;
    std::uint64_t staticRamUsedBytes = 0;
    std::uint64_t estimatedRamRequiredBytes = 0;
    std::uint64_t ramRemainingBytes = 0;
    double ramUsagePercent = 0.0;
    std::wstring riskLevel;
    std::vector<std::wstring> warnings;
};

struct MapParseData {
    bool success = false;
    std::wstring compilerDisplayName;
    ProgramSize programSize;
    std::vector<MemoryRegion> memoryRegions;
    std::vector<SymbolEntry> symbols;
    StackHeapInfo stackHeapInfo;
    std::vector<std::wstring> warnings;
    std::vector<std::wstring> logLines;
    std::optional<std::uint64_t> ramLimitBytes;
    std::wstring errorMessage;
};

struct AnalysisResult {
    bool success = false;
    std::wstring compilerDisplayName;
    MapFormatDetectionResult detection;
    ProgramSize programSize;
    std::vector<MemoryRegion> memoryRegions;
    std::vector<SymbolEntry> symbols;
    StackHeapInfo stackHeapInfo;
    AnalysisReport report;
    std::vector<std::wstring> logLines;
    std::wstring narrative;
    bool usedAutomaticParserSelection = false;
    std::optional<std::uint64_t> parsedRamLimitBytes;
    std::optional<std::uint64_t> effectiveRamLimitBytes;
    std::wstring errorMessage;
};
