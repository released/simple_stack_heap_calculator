#include "parser/keil_map_parser.h"

#include <regex>

#include "util/file_util.h"
#include "util/string_util.h"

namespace {

struct ExecutionRegionState {
    std::wstring name;
    std::uint64_t baseAddress = 0;
    std::uint64_t usedBytes = 0;
    std::uint64_t sizeBytes = 0;
    std::uint64_t maxBytes = 0;
    bool maxKnown = false;
};

struct SectionRecord {
    std::uint64_t execAddress = 0;
    std::uint64_t loadAddress = 0;
    std::uint64_t sizeBytes = 0;
    std::string type;
    std::string attr;
    std::string sectionName;
    std::string objectName;
    bool isPad = false;
};

struct KeilSummary {
    bool totalsFound = false;
    int totalsPriority = -1;
    std::uint64_t codeBytes = 0;
    std::uint64_t incDataBytes = 0;
    std::uint64_t roDataBytes = 0;
    std::uint64_t rwDataBytes = 0;
    std::uint64_t ziDataBytes = 0;
    std::uint64_t debugBytes = 0;

    bool totalRoFound = false;
    bool totalRwFound = false;
    bool totalRomFound = false;
    std::uint64_t totalRoBytes = 0;
    std::uint64_t totalRwBytes = 0;
    std::uint64_t totalRomBytes = 0;
};

bool TryParseHexToken(const std::string& text, std::uint64_t* out_value) {
    if (!out_value || text.size() < 3 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) {
        return false;
    }
    char* end = nullptr;
    unsigned long long value = strtoull(text.c_str(), &end, 16);
    if (!end || *end != '\0') {
        return false;
    }
    *out_value = static_cast<std::uint64_t>(value);
    return true;
}

bool TryParseDecimalToken(const std::string& text, std::uint64_t* out_value) {
    if (!out_value) {
        return false;
    }
    char* end = nullptr;
    unsigned long long value = strtoull(text.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }
    *out_value = static_cast<std::uint64_t>(value);
    return true;
}

bool IsStackLabel(const std::string& text) {
    return stringutil::ContainsInsensitive(text, "STACK") || stringutil::ContainsInsensitive(text, "__INITIAL_SP");
}

bool IsHeapLabel(const std::string& text) {
    return stringutil::ContainsInsensitive(text, "HEAP") || stringutil::ContainsInsensitive(text, "__HEAP");
}

std::wstring NarrowToWide(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

void PushRegion(const ExecutionRegionState& state, std::vector<MemoryRegion>* regions) {
    if (!regions || state.name.empty()) {
        return;
    }

    MemoryRegion region;
    region.name = state.name;
    region.baseAddress = state.baseAddress;
    region.usedBytes = state.usedBytes;
    region.sizeBytes = state.maxKnown && state.maxBytes >= state.usedBytes ? state.maxBytes : state.sizeBytes;
    if (region.sizeBytes == 0) {
        region.sizeBytes = region.usedBytes;
    }
    regions->push_back(region);
}

bool TryParseExecutionRegionHeader(const std::string& line, ExecutionRegionState* out_state) {
    if (!out_state) {
        return false;
    }

    static const std::regex pattern(
        R"(^\s*Execution Region\s+(\S+)\s+\(Exec base:\s*(0x[0-9A-Fa-f]+), Load base:\s*(0x[0-9A-Fa-f]+), Size:\s*(0x[0-9A-Fa-f]+), Max:\s*(0x[0-9A-Fa-f]+),)",
        std::regex::ECMAScript);
    std::smatch match;
    if (!std::regex_search(line, match, pattern)) {
        return false;
    }

    std::uint64_t execBase = 0;
    std::uint64_t sizeBytes = 0;
    std::uint64_t maxBytes = 0;
    if (!TryParseHexToken(match[2].str(), &execBase) ||
        !TryParseHexToken(match[4].str(), &sizeBytes) ||
        !TryParseHexToken(match[5].str(), &maxBytes)) {
        return false;
    }

    out_state->name = NarrowToWide(match[1].str());
    out_state->baseAddress = execBase;
    out_state->usedBytes = sizeBytes;
    out_state->sizeBytes = sizeBytes;
    out_state->maxBytes = maxBytes;
    out_state->maxKnown = (maxBytes != 0xFFFFFFFFULL);
    return true;
}

bool TryParseSectionRecord(const std::string& line, SectionRecord* out_record) {
    if (!out_record) {
        return false;
    }

    std::vector<std::string> tokens = stringutil::SplitWhitespace(line);
    if (tokens.size() < 4) {
        return false;
    }
    if (!TryParseHexToken(tokens[0], &out_record->execAddress)) {
        return false;
    }

    if (tokens[1] != "-") {
        if (!TryParseHexToken(tokens[1], &out_record->loadAddress)) {
            return false;
        }
    }

    if (!TryParseHexToken(tokens[2], &out_record->sizeBytes)) {
        return false;
    }

    out_record->type = tokens[3];
    out_record->attr.clear();
    out_record->sectionName.clear();
    out_record->objectName.clear();
    out_record->isPad = stringutil::ContainsInsensitive(out_record->type, "PAD");

    if (out_record->isPad) {
        return true;
    }

    if (tokens.size() < 8) {
        return false;
    }

    out_record->attr = tokens[4];
    out_record->objectName = tokens.back();

    size_t sectionStart = 6;
    if (sectionStart < tokens.size() - 1 && tokens[sectionStart] == "*") {
        ++sectionStart;
    }
    if (sectionStart >= tokens.size() - 1) {
        return false;
    }

    std::string sectionName;
    for (size_t i = sectionStart; i + 1 < tokens.size(); ++i) {
        if (!sectionName.empty()) {
            sectionName += ' ';
        }
        sectionName += tokens[i];
    }
    out_record->sectionName = sectionName;
    return true;
}

void ParseObjectAndLibrary(const std::string& objectToken, std::wstring* out_object, std::wstring* out_library) {
    if (!out_object || !out_library) {
        return;
    }

    *out_object = NarrowToWide(objectToken);
    out_library->clear();

    size_t open = objectToken.find('(');
    size_t close = objectToken.find(')');
    if (open == std::string::npos || close == std::string::npos || close <= open) {
        return;
    }

    *out_library = NarrowToWide(objectToken.substr(0, open));
    *out_object = NarrowToWide(objectToken.substr(open + 1, close - open - 1));
}

bool LooksLikeRamRegion(const MemoryRegion& region) {
    return region.baseAddress >= 0x20000000ULL ||
           stringutil::ContainsInsensitive(region.name, L"ER_RW") ||
           stringutil::ContainsInsensitive(region.name, L"ER_ZI");
}

bool TryParseTotalsLine(const std::string& line, KeilSummary* out_summary) {
    if (!out_summary) {
        return false;
    }
    static const std::regex pattern(
        R"(^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(Grand Totals|ELF Image Totals|ROM Totals)\s*$)",
        std::regex::ECMAScript);
    std::smatch match;
    if (!std::regex_match(line, match, pattern)) {
        return false;
    }

    const std::string label = match[7].str();
    int priority = 0;
    if (label == "Grand Totals") {
        priority = 3;
    } else if (label == "ELF Image Totals") {
        priority = 2;
    } else if (label == "ROM Totals") {
        priority = 1;
    }
    if (priority < out_summary->totalsPriority) {
        return true;
    }

    std::uint64_t codeBytes = 0;
    std::uint64_t incDataBytes = 0;
    std::uint64_t roDataBytes = 0;
    std::uint64_t rwDataBytes = 0;
    std::uint64_t ziDataBytes = 0;
    std::uint64_t debugBytes = 0;
    const bool ok =
        TryParseDecimalToken(match[1].str(), &codeBytes) &&
        TryParseDecimalToken(match[2].str(), &incDataBytes) &&
        TryParseDecimalToken(match[3].str(), &roDataBytes) &&
        TryParseDecimalToken(match[4].str(), &rwDataBytes) &&
        TryParseDecimalToken(match[5].str(), &ziDataBytes) &&
        TryParseDecimalToken(match[6].str(), &debugBytes);
    if (!ok) {
        return false;
    }

    out_summary->totalsFound = true;
    out_summary->totalsPriority = priority;
    out_summary->codeBytes = codeBytes;
    out_summary->incDataBytes = incDataBytes;
    out_summary->roDataBytes = roDataBytes;
    out_summary->rwDataBytes = rwDataBytes;
    out_summary->ziDataBytes = ziDataBytes;
    out_summary->debugBytes = debugBytes;
    return true;
}

void TryParseNamedTotalLine(const std::string& line, KeilSummary* out_summary) {
    if (!out_summary) {
        return;
    }
    static const std::regex totalRoPattern(R"(^\s*Total RO\s+Size.*?(\d+)\s+\()", std::regex::ECMAScript);
    static const std::regex totalRwPattern(R"(^\s*Total RW\s+Size.*?(\d+)\s+\()", std::regex::ECMAScript);
    static const std::regex totalRomPattern(R"(^\s*Total ROM Size.*?(\d+)\s+\()", std::regex::ECMAScript);
    std::smatch match;
    if (std::regex_search(line, match, totalRoPattern) && TryParseDecimalToken(match[1].str(), &out_summary->totalRoBytes)) {
        out_summary->totalRoFound = true;
    }
    if (std::regex_search(line, match, totalRwPattern) && TryParseDecimalToken(match[1].str(), &out_summary->totalRwBytes)) {
        out_summary->totalRwFound = true;
    }
    if (std::regex_search(line, match, totalRomPattern) && TryParseDecimalToken(match[1].str(), &out_summary->totalRomBytes)) {
        out_summary->totalRomFound = true;
    }
}

std::wstring DetectCompilerDisplayName(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "Arm Compiler for Embedded 6")) {
            return L"KEIL v6";
        }
        if (stringutil::ContainsInsensitive(line, "ARM Linker, 5") ||
            stringutil::ContainsInsensitive(line, "Arm Compiler 5") ||
            stringutil::ContainsInsensitive(line, "RVCT")) {
            return L"KEIL v5";
        }
    }
    return L"KEIL";
}

}  // namespace

bool KeilMapParser::SupportsFamily(CompilerFamily family) const {
    return family == CompilerFamily::Keil;
}

MapParseData KeilMapParser::Parse(const ParseOptions& options) const {
    MapParseData result;
    result.compilerDisplayName = L"KEIL";
    result.logLines.push_back(L"Reading map file: " + options.mapPath);

    std::vector<std::string> lines;
    std::wstring readError;
    if (!fileutil::ReadAllLines(options.mapPath, &lines, &readError)) {
        result.errorMessage = L"Failed to read map file. " + readError;
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    bool looksLikeKeil = false;
    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "armlink") ||
            stringutil::ContainsInsensitive(line, "Arm Linker") ||
            stringutil::ContainsInsensitive(line, "Execution Region") ||
            stringutil::ContainsInsensitive(line, "Total RO  Size")) {
            looksLikeKeil = true;
            break;
        }
    }

    if (!looksLikeKeil) {
        result.errorMessage = L"Selected KEIL tab, but this map file does not look like a KEIL linker map.";
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    result.compilerDisplayName = DetectCompilerDisplayName(lines);
    result.logLines.push_back(L"Compiler detected: " + result.compilerDisplayName);

    ExecutionRegionState currentRegion;
    bool hasRegion = false;
    KeilSummary summary;

    std::uint64_t sectionCodeSum = 0;
    std::uint64_t sectionRoSum = 0;
    std::uint64_t sectionRwSum = 0;
    std::uint64_t sectionZiSum = 0;

    std::uint64_t removedHeapBytes = 0;
    std::uint64_t removedStackBytes = 0;
    static const std::regex removedSectionPattern(
        R"(^\s*Removing\s+.+?(\.bss\.[A-Za-z0-9_]+|\bSTACK\b|\bHEAP\b).*?\((\d+)\s+bytes\)\.)",
        std::regex::ECMAScript);

    for (const std::string& rawLine : lines) {
        TryParseTotalsLine(rawLine, &summary);
        TryParseNamedTotalLine(rawLine, &summary);

        std::smatch removedMatch;
        if (std::regex_search(rawLine, removedMatch, removedSectionPattern)) {
            std::uint64_t removedBytes = 0;
            if (TryParseDecimalToken(removedMatch[2].str(), &removedBytes)) {
                const std::string name = removedMatch[1].str();
                if (IsHeapLabel(name)) {
                    removedHeapBytes = removedBytes;
                } else if (IsStackLabel(name)) {
                    removedStackBytes = removedBytes;
                }
            }
        }

        ExecutionRegionState parsedRegion;
        if (TryParseExecutionRegionHeader(rawLine, &parsedRegion)) {
            if (hasRegion) {
                PushRegion(currentRegion, &result.memoryRegions);
            }
            currentRegion = parsedRegion;
            hasRegion = true;
            continue;
        }

        if (!hasRegion) {
            continue;
        }

        if (stringutil::ContainsInsensitive(rawLine, "Image component sizes") ||
            stringutil::ContainsInsensitive(rawLine, "Global Symbols")) {
            PushRegion(currentRegion, &result.memoryRegions);
            hasRegion = false;
            continue;
        }

        SectionRecord record;
        if (!TryParseSectionRecord(rawLine, &record) || record.isPad) {
            continue;
        }

        const bool isStack = IsStackLabel(record.sectionName);
        const bool isHeap = IsHeapLabel(record.sectionName);

        if (stringutil::ContainsInsensitive(record.type, "Code")) {
            sectionCodeSum += record.sizeBytes;
        } else if (stringutil::ContainsInsensitive(record.type, "Data") &&
                   stringutil::ContainsInsensitive(record.attr, "RO")) {
            sectionRoSum += record.sizeBytes;
        } else if (stringutil::ContainsInsensitive(record.type, "Data") &&
                   stringutil::ContainsInsensitive(record.attr, "RW")) {
            sectionRwSum += record.sizeBytes;
        } else if (stringutil::ContainsInsensitive(record.type, "Zero") &&
                   stringutil::ContainsInsensitive(record.attr, "RW")) {
            sectionZiSum += record.sizeBytes;
            if (isStack) {
                result.stackHeapInfo.stackBase = record.execAddress;
                result.stackHeapInfo.stackLimit = record.execAddress + record.sizeBytes;
                result.stackHeapInfo.stackSizeBytes = record.sizeBytes;
                result.stackHeapInfo.stackRecommendedBytes = record.sizeBytes;
                result.stackHeapInfo.stackIncludedInStaticRam = true;
                result.stackHeapInfo.source = L"Execution Region section " + NarrowToWide(record.sectionName);
                result.stackHeapInfo.confidence = L"High";
            } else if (isHeap) {
                result.stackHeapInfo.heapBase = record.execAddress;
                result.stackHeapInfo.heapLimit = record.execAddress + record.sizeBytes;
                result.stackHeapInfo.heapSizeBytes = record.sizeBytes;
                result.stackHeapInfo.heapRecommendedBytes = record.sizeBytes;
                result.stackHeapInfo.heapIncludedInStaticRam = true;
                if (result.stackHeapInfo.source.empty()) {
                    result.stackHeapInfo.source = L"Execution Region section " + NarrowToWide(record.sectionName);
                } else {
                    result.stackHeapInfo.source += L"; heap from section " + NarrowToWide(record.sectionName);
                }
                result.stackHeapInfo.confidence = L"High";
            }
        }
    }

    if (hasRegion) {
        PushRegion(currentRegion, &result.memoryRegions);
    }

    bool inGlobalSymbols = false;
    for (const std::string& rawLine : lines) {
        if (stringutil::ContainsInsensitive(rawLine, "Global Symbols")) {
            inGlobalSymbols = true;
            continue;
        }
        if (!inGlobalSymbols) {
            continue;
        }
        if (rawLine.find("==============================================================================") != std::string::npos) {
            continue;
        }

        std::vector<std::string> tokens = stringutil::SplitWhitespace(rawLine);
        if (tokens.size() < 2) {
            continue;
        }
        std::uint64_t address = 0;
        if (!TryParseHexToken(tokens[1], &address)) {
            continue;
        }

        const std::string& symbolName = tokens[0];
        const bool stackSymbol = IsStackLabel(symbolName);
        const bool heapSymbol = IsHeapLabel(symbolName);
        if (!stackSymbol && !heapSymbol) {
            continue;
        }

        SymbolEntry symbol;
        symbol.address = address;
        symbol.symbolName = NarrowToWide(symbolName);
        symbol.type = stackSymbol ? SymbolType::Stack : SymbolType::Heap;
        if (!tokens.empty()) {
            std::wstring objectName;
            std::wstring libraryName;
            ParseObjectAndLibrary(tokens.back(), &objectName, &libraryName);
            symbol.objectName = objectName;
            symbol.libraryName = libraryName;
        }
        result.symbols.push_back(symbol);

        if (stringutil::ContainsInsensitive(symbolName, "__initial_sp")) {
            result.stackHeapInfo.stackLimit = address;
        } else if (stringutil::ContainsInsensitive(symbolName, "Stack_Mem") ||
                   stringutil::ContainsInsensitive(symbolName, "STACK")) {
            if (result.stackHeapInfo.stackBase == 0) {
                result.stackHeapInfo.stackBase = address;
            }
        } else if (stringutil::ContainsInsensitive(symbolName, "__heap_base")) {
            result.stackHeapInfo.heapBase = address;
        } else if (stringutil::ContainsInsensitive(symbolName, "__heap_limit")) {
            result.stackHeapInfo.heapLimit = address;
        }
    }

    if (result.stackHeapInfo.stackSizeBytes == 0 &&
        result.stackHeapInfo.stackLimit > result.stackHeapInfo.stackBase) {
        result.stackHeapInfo.stackSizeBytes = result.stackHeapInfo.stackLimit - result.stackHeapInfo.stackBase;
        result.stackHeapInfo.stackRecommendedBytes = result.stackHeapInfo.stackSizeBytes;
        result.stackHeapInfo.stackIncludedInStaticRam = true;
        if (result.stackHeapInfo.confidence.empty()) {
            result.stackHeapInfo.confidence = L"Medium";
        }
        if (result.stackHeapInfo.source.empty()) {
            result.stackHeapInfo.source = L"Derived from stack symbols";
        }
    }

    if (result.stackHeapInfo.heapSizeBytes == 0 &&
        result.stackHeapInfo.heapLimit > result.stackHeapInfo.heapBase) {
        result.stackHeapInfo.heapSizeBytes = result.stackHeapInfo.heapLimit - result.stackHeapInfo.heapBase;
        result.stackHeapInfo.heapRecommendedBytes = result.stackHeapInfo.heapSizeBytes;
        result.stackHeapInfo.heapIncludedInStaticRam = true;
        if (result.stackHeapInfo.confidence.empty()) {
            result.stackHeapInfo.confidence = L"Medium";
        }
        if (result.stackHeapInfo.source.empty()) {
            result.stackHeapInfo.source = L"Derived from heap symbols";
        }
    }

    if (summary.totalsFound) {
        result.programSize.codeBytes = summary.codeBytes;
        result.programSize.roDataBytes = summary.roDataBytes;
        result.programSize.rwDataBytes = summary.rwDataBytes;
        result.programSize.ziDataBytes = summary.ziDataBytes;
        result.programSize.hasCode = true;
        result.programSize.hasRoData = true;
        result.programSize.hasRwData = true;
        result.programSize.hasZiData = true;
        result.logLines.push_back(L"Program sizes sourced from KEIL summary totals.");
    } else {
        result.programSize.codeBytes = sectionCodeSum;
        result.programSize.roDataBytes = sectionRoSum;
        result.programSize.rwDataBytes = sectionRwSum;
        result.programSize.ziDataBytes = sectionZiSum;
        result.programSize.hasCode = sectionCodeSum > 0;
        result.programSize.hasRoData = sectionRoSum > 0;
        result.programSize.hasRwData = sectionRwSum > 0;
        result.programSize.hasZiData = sectionZiSum > 0;
        result.warnings.push_back(L"Summary totals were not found. Falling back to section sums.");
    }

    if (result.stackHeapInfo.heapRecommendedBytes == 0 && removedHeapBytes > 0) {
        result.stackHeapInfo.heapRecommendedBytes = removedHeapBytes;
        result.stackHeapInfo.heapRecommendationEstimated = true;
        result.warnings.push_back(L"Removed HEAP section found in map. Heap recommendation uses removed section size.");
    }
    if (result.stackHeapInfo.stackRecommendedBytes == 0 && removedStackBytes > 0) {
        result.stackHeapInfo.stackRecommendedBytes = removedStackBytes;
        result.stackHeapInfo.stackRecommendationEstimated = true;
        result.warnings.push_back(L"Removed STACK section found in map. Stack recommendation uses removed section size.");
    }

    if (result.stackHeapInfo.heapRecommendedBytes == 0) {
        result.stackHeapInfo.heapRecommendedBytes = result.stackHeapInfo.heapSizeBytes;
    }
    if (result.stackHeapInfo.stackRecommendedBytes == 0) {
        result.stackHeapInfo.stackRecommendedBytes = result.stackHeapInfo.stackSizeBytes;
    }

    if (summary.totalRoFound &&
        (result.programSize.codeBytes + result.programSize.roDataBytes) != summary.totalRoBytes) {
        result.warnings.push_back(L"Program Size summary does not match Total RO Size exactly.");
    }
    if (summary.totalRwFound &&
        (result.programSize.rwDataBytes + result.programSize.ziDataBytes) != summary.totalRwBytes) {
        result.warnings.push_back(L"Program Size summary does not match Total RW Size exactly.");
    }
    if (summary.totalRomFound &&
        (result.programSize.codeBytes + result.programSize.roDataBytes + result.programSize.rwDataBytes) != summary.totalRomBytes) {
        result.warnings.push_back(L"Program Size summary does not match Total ROM Size exactly.");
    }

    if (summary.totalRoFound) {
        result.logLines.push_back(L"Validated Total RO Size from map summary.");
    }
    if (summary.totalRwFound) {
        result.logLines.push_back(L"Validated Total RW Size from map summary.");
    }
    if (summary.totalRomFound) {
        result.logLines.push_back(L"Validated Total ROM Size from map summary.");
    }

    std::uint64_t detectedRamLimit = 0;
    bool foundCapacity = false;
    for (const MemoryRegion& region : result.memoryRegions) {
        if (LooksLikeRamRegion(region) && region.sizeBytes > region.usedBytes) {
            detectedRamLimit += region.sizeBytes;
            foundCapacity = true;
        }
    }
    if (foundCapacity && detectedRamLimit > 0) {
        result.ramLimitBytes = detectedRamLimit;
        result.logLines.push_back(L"RAM limit parsed from execution region capacities.");
    } else {
        result.logLines.push_back(L"RAM limit not found in KEIL map. User override is recommended.");
    }

    if (result.stackHeapInfo.heapSizeBytes == 0 && result.stackHeapInfo.heapRecommendedBytes == 0) {
        result.warnings.push_back(L"Heap section or heap symbols were not found in active memory regions. Heap remains 0.");
    }
    if (result.stackHeapInfo.stackSizeBytes == 0 && result.stackHeapInfo.stackRecommendedBytes == 0) {
        result.warnings.push_back(L"Stack section or stack symbols were not found. Stack remains 0.");
    }

    result.success = result.programSize.hasCode || result.programSize.hasRoData ||
                     result.programSize.hasRwData || result.programSize.hasZiData;
    if (!result.success) {
        result.errorMessage = L"Failed to locate program sections in KEIL map.";
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    result.logLines.push_back(L"Raw section sums for cross-check: Code=" + NarrowToWide(std::to_string(sectionCodeSum)) +
                              L", RO=" + NarrowToWide(std::to_string(sectionRoSum)) +
                              L", RW=" + NarrowToWide(std::to_string(sectionRwSum)) +
                              L", ZI=" + NarrowToWide(std::to_string(sectionZiSum)));
    if (summary.totalsFound) {
        result.logLines.push_back(L"Final Program Size uses KEIL summary totals, not raw section sums.");
    }
    result.logLines.push_back(L"KEIL parsing finished.");
    return result;
}
