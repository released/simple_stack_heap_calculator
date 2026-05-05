#include "parser/iar_map_parser.h"

#include <algorithm>
#include <cstdlib>
#include <regex>
#include <utility>

#include "util/file_util.h"
#include "util/string_util.h"

namespace {

std::wstring NarrowToWide(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

std::string NormalizeIarNumber(const std::string& text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (char ch : text) {
        if (ch != '\'' && ch != ',') {
            normalized.push_back(ch);
        }
    }
    return normalized;
}

bool TryParseIarHex(const std::string& text, std::uint64_t* out_value) {
    if (!out_value) {
        return false;
    }
    const std::string normalized = NormalizeIarNumber(stringutil::Trim(text));
    if (normalized.empty()) {
        return false;
    }
    char* end = nullptr;
    unsigned long long value = std::strtoull(normalized.c_str(), &end, 16);
    if (!end || *end != '\0') {
        return false;
    }
    *out_value = static_cast<std::uint64_t>(value);
    return true;
}

bool TryParseIarDecimal(const std::string& text, std::uint64_t* out_value) {
    if (!out_value) {
        return false;
    }
    const std::string normalized = NormalizeIarNumber(stringutil::Trim(text));
    if (normalized.empty()) {
        return false;
    }
    char* end = nullptr;
    unsigned long long value = std::strtoull(normalized.c_str(), &end, 10);
    if (!end || *end != '\0') {
        return false;
    }
    *out_value = static_cast<std::uint64_t>(value);
    return true;
}

bool TryParseFooterTotal(const std::string& line,
                         const std::string& label,
                         std::uint64_t* out_value) {
    if (!out_value || !stringutil::ContainsInsensitive(line, label)) {
        return false;
    }

    const std::vector<std::string> tokens = stringutil::SplitWhitespace(line);
    if (tokens.empty()) {
        return false;
    }
    return TryParseIarDecimal(tokens.front(), out_value);
}

bool IsStackSymbol(const std::string& name) {
    return stringutil::ContainsInsensitive(name, "stack");
}

bool IsHeapSymbol(const std::string& name) {
    return stringutil::ContainsInsensitive(name, "heap");
}

std::uint64_t MergeIntervalUnionBytes(std::vector<std::pair<std::uint64_t, std::uint64_t>> intervals) {
    if (intervals.empty()) {
        return 0;
    }

    std::sort(intervals.begin(), intervals.end());
    std::uint64_t total = 0;
    std::uint64_t currentStart = intervals.front().first;
    std::uint64_t currentEnd = intervals.front().second;

    for (size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i].first <= currentEnd + 1) {
            currentEnd = std::max(currentEnd, intervals[i].second);
            continue;
        }
        total += (currentEnd - currentStart) + 1;
        currentStart = intervals[i].first;
        currentEnd = intervals[i].second;
    }

    total += (currentEnd - currentStart) + 1;
    return total;
}

}  // namespace

bool IarMapParser::SupportsFamily(CompilerFamily family) const {
    return family == CompilerFamily::Iar;
}

MapParseData IarMapParser::Parse(const ParseOptions& options) const {
    MapParseData data;
    data.compilerDisplayName = L"IAR (ILINK)";
    data.logLines.push_back(L"Reading map file: " + options.mapPath);

    std::vector<std::string> lines;
    std::wstring readError;
    if (!fileutil::ReadAllLines(options.mapPath, &lines, &readError)) {
        data.errorMessage = L"Failed to read map file. " + readError;
        data.logLines.push_back(data.errorMessage);
        return data;
    }

    bool looksLikeIar = false;
    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "IAR ELF Linker") ||
            stringutil::ContainsInsensitive(line, "ILINK") ||
            stringutil::ContainsInsensitive(line, "*** PLACEMENT SUMMARY")) {
            looksLikeIar = true;
            break;
        }
    }
    if (!looksLikeIar) {
        data.errorMessage = L"Selected IAR tab, but this map file does not look like an IAR ILINK map.";
        data.logLines.push_back(data.errorMessage);
        return data;
    }

    data.logLines.push_back(L"Compiler detected: IAR (ILINK)");

    std::uint64_t readonlyCodeBytes = 0;
    std::uint64_t readwriteCodeBytes = 0;
    std::uint64_t readonlyDataBytes = 0;
    std::uint64_t readwriteDataBytes = 0;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> writableIntervals;

    std::uint64_t initedBytes = 0;
    std::uint64_t zeroBytes = 0;
    std::uint64_t uninitBytes = 0;
    std::uint64_t stackTotalBytes = 0;
    std::uint64_t stackMinAddress = 0;
    std::uint64_t stackMaxAddress = 0;
    bool stackAddressSeen = false;

    static const std::regex kRegionPattern(
        R"(^\s*\"([^\"]+)\":\s+place\s+in\s+\[from\s+(0x[0-9A-Fa-f']+)\s+to\s+(0x[0-9A-Fa-f']+)\]\s+\{\s*([^}]*)\}\s*;?\s*$)",
        std::regex::ECMAScript);
    static const std::regex kRegionNameOnlyPattern(
        R"(^\s*\"([^\"]+)\":\s*$)",
        std::regex::ECMAScript);
    static const std::regex kRegionContinuationPattern(
        R"(^\s*place\s+in\s+\[from\s+(0x[0-9A-Fa-f']+)\s+to\s+(0x[0-9A-Fa-f']+)\]\s+\{\s*([^}]*)\}\s*;?\s*$)",
        std::regex::ECMAScript);
    static const std::regex kSectionLinePattern(
        R"(^\s+(\S+)\s+([A-Za-z ]+?)\s+(0x[0-9A-Fa-f']+)\s+(0x[0-9A-Fa-f']+)\s+(.+?)\s*$)",
        std::regex::ECMAScript);
    static const std::regex kEntryLinePattern(
        R"(^\s*(\S+)\s+(0x[0-9A-Fa-f']+)\s+(0x[0-9A-Fa-f']+)\s+Data\s+\S+\s+.+$)",
        std::regex::ECMAScript);

    std::string pendingRegionName;
    for (const std::string& line : lines) {
        TryParseFooterTotal(line, "readonly  code memory", &readonlyCodeBytes);
        TryParseFooterTotal(line, "readwrite code memory", &readwriteCodeBytes);
        TryParseFooterTotal(line, "readonly  data memory", &readonlyDataBytes);
        TryParseFooterTotal(line, "readwrite data memory", &readwriteDataBytes);

        std::smatch regionMatch;
        if (std::regex_match(line, regionMatch, kRegionPattern)) {
            std::uint64_t startAddress = 0;
            std::uint64_t endAddress = 0;
            if (!TryParseIarHex(regionMatch[2].str(), &startAddress) ||
                !TryParseIarHex(regionMatch[3].str(), &endAddress) ||
                endAddress < startAddress) {
                continue;
            }

            MemoryRegion region;
            region.name = NarrowToWide(regionMatch[1].str());
            region.baseAddress = startAddress;
            region.sizeBytes = (endAddress - startAddress) + 1;
            region.usedBytes = 0;
            data.memoryRegions.push_back(region);
            if (stringutil::ContainsInsensitive(regionMatch[4].str(), "rw")) {
                writableIntervals.push_back({startAddress, endAddress});
            }
            continue;
        }

        std::smatch regionNameMatch;
        if (std::regex_match(line, regionNameMatch, kRegionNameOnlyPattern)) {
            pendingRegionName = regionNameMatch[1].str();
            continue;
        }

        std::smatch regionContinuationMatch;
        if (!pendingRegionName.empty() &&
            std::regex_match(line, regionContinuationMatch, kRegionContinuationPattern)) {
            std::uint64_t startAddress = 0;
            std::uint64_t endAddress = 0;
            if (!TryParseIarHex(regionContinuationMatch[1].str(), &startAddress) ||
                !TryParseIarHex(regionContinuationMatch[2].str(), &endAddress) ||
                endAddress < startAddress) {
                pendingRegionName.clear();
                continue;
            }

            MemoryRegion region;
            region.name = NarrowToWide(pendingRegionName);
            region.baseAddress = startAddress;
            region.sizeBytes = (endAddress - startAddress) + 1;
            region.usedBytes = 0;
            data.memoryRegions.push_back(region);
            if (stringutil::ContainsInsensitive(regionContinuationMatch[3].str(), "rw")) {
                writableIntervals.push_back({startAddress, endAddress});
            }
            pendingRegionName.clear();
            continue;
        }

        std::smatch sectionMatch;
        if (std::regex_match(line, sectionMatch, kSectionLinePattern)) {
            const std::string kind = stringutil::Trim(sectionMatch[2].str());
            std::uint64_t address = 0;
            std::uint64_t sizeBytes = 0;
            if (!TryParseIarHex(sectionMatch[3].str(), &address) ||
                !TryParseIarHex(sectionMatch[4].str(), &sizeBytes)) {
                continue;
            }

            for (MemoryRegion& region : data.memoryRegions) {
                const std::uint64_t regionEnd = region.baseAddress + region.sizeBytes;
                if (address >= region.baseAddress && address < regionEnd) {
                    region.usedBytes += sizeBytes;
                    break;
                }
            }

            if (stringutil::ContainsInsensitive(kind, "inited")) {
                initedBytes += sizeBytes;
            } else if (stringutil::ContainsInsensitive(kind, "zero")) {
                zeroBytes += sizeBytes;
            } else if (stringutil::ContainsInsensitive(kind, "uninit")) {
                uninitBytes += sizeBytes;
            }
            continue;
        }

        std::smatch entryMatch;
        if (std::regex_match(line, entryMatch, kEntryLinePattern)) {
            const std::string symbolName = entryMatch[1].str();
            std::uint64_t address = 0;
            std::uint64_t sizeBytes = 0;
            if (!TryParseIarHex(entryMatch[2].str(), &address) ||
                !TryParseIarHex(entryMatch[3].str(), &sizeBytes)) {
                continue;
            }

            if (IsStackSymbol(symbolName)) {
                SymbolEntry symbol;
                symbol.address = address;
                symbol.sizeBytes = sizeBytes;
                symbol.type = SymbolType::Stack;
                symbol.symbolName = NarrowToWide(symbolName);
                data.symbols.push_back(symbol);

                stackTotalBytes += sizeBytes;
                if (!stackAddressSeen) {
                    stackMinAddress = address;
                    stackMaxAddress = address + sizeBytes;
                    stackAddressSeen = true;
                } else {
                    stackMinAddress = std::min(stackMinAddress, address);
                    stackMaxAddress = std::max(stackMaxAddress, address + sizeBytes);
                }
            } else if (IsHeapSymbol(symbolName)) {
                SymbolEntry symbol;
                symbol.address = address;
                symbol.sizeBytes = sizeBytes;
                symbol.type = SymbolType::Heap;
                symbol.symbolName = NarrowToWide(symbolName);
                data.symbols.push_back(symbol);
            }
        }
    }

    for (size_t i = 0; i + 1 < lines.size(); ++i) {
        const std::string symbolName = stringutil::Trim(lines[i]);
        if (symbolName.empty() || (!IsStackSymbol(symbolName) && !IsHeapSymbol(symbolName))) {
            continue;
        }

        const std::vector<std::string> tokens = stringutil::SplitWhitespace(lines[i + 1]);
        if (tokens.size() < 2) {
            continue;
        }

        std::uint64_t address = 0;
        std::uint64_t sizeBytes = 0;
        if (!TryParseIarHex(tokens[0], &address) || !TryParseIarHex(tokens[1], &sizeBytes)) {
            continue;
        }

        if (IsStackSymbol(symbolName)) {
            SymbolEntry symbol;
            symbol.address = address;
            symbol.sizeBytes = sizeBytes;
            symbol.type = SymbolType::Stack;
            symbol.symbolName = NarrowToWide(symbolName);
            data.symbols.push_back(symbol);

            stackTotalBytes += sizeBytes;
            if (!stackAddressSeen) {
                stackMinAddress = address;
                stackMaxAddress = address + sizeBytes;
                stackAddressSeen = true;
            } else {
                stackMinAddress = std::min(stackMinAddress, address);
                stackMaxAddress = std::max(stackMaxAddress, address + sizeBytes);
            }
        } else if (IsHeapSymbol(symbolName)) {
            SymbolEntry symbol;
            symbol.address = address;
            symbol.sizeBytes = sizeBytes;
            symbol.type = SymbolType::Heap;
            symbol.symbolName = NarrowToWide(symbolName);
            data.symbols.push_back(symbol);
        }
    }

    data.programSize.codeBytes = readonlyCodeBytes;
    data.programSize.hasCode = readonlyCodeBytes > 0;
    data.programSize.roDataBytes = readonlyDataBytes;
    data.programSize.hasRoData = readonlyDataBytes > 0;
    data.programSize.rwDataBytes = initedBytes;
    data.programSize.hasRwData = initedBytes > 0;
    data.programSize.ziDataBytes = zeroBytes + uninitBytes;
    data.programSize.hasZiData = data.programSize.ziDataBytes > 0;

    data.logLines.push_back(L"IAR footer totals parsed.");
    data.logLines.push_back(L"Readonly code memory = " + NarrowToWide(std::to_string(readonlyCodeBytes)) + L" bytes.");
    data.logLines.push_back(L"Readwrite code memory = " + NarrowToWide(std::to_string(readwriteCodeBytes)) + L" bytes.");
    data.logLines.push_back(L"Readonly data memory = " + NarrowToWide(std::to_string(readonlyDataBytes)) + L" bytes.");
    data.logLines.push_back(L"Readwrite data memory = " + NarrowToWide(std::to_string(readwriteDataBytes)) + L" bytes.");
    data.logLines.push_back(L"Section-kind split: inited=" + NarrowToWide(std::to_string(initedBytes)) +
                            L", zero=" + NarrowToWide(std::to_string(zeroBytes)) +
                            L", uninit=" + NarrowToWide(std::to_string(uninitBytes)));

    if (readwriteCodeBytes > 0) {
        data.programSize.rwDataBytes += readwriteCodeBytes;
        data.logLines.push_back(L"RW-data includes readwrite code memory because RAMCODE consumes both flash image and RAM.");
    }

    if (stackTotalBytes > 0) {
        data.stackHeapInfo.stackBase = stackMinAddress;
        data.stackHeapInfo.stackLimit = stackMaxAddress;
        data.stackHeapInfo.stackSizeBytes = stackTotalBytes;
        data.stackHeapInfo.stackRecommendedBytes = stackTotalBytes;
        data.stackHeapInfo.stackIncludedInStaticRam = true;
        data.stackHeapInfo.source = L"ENTRY LIST stack symbols";
        data.stackHeapInfo.confidence = L"High";
    } else {
        data.warnings.push_back(L"Stack symbols were not identified in the IAR ENTRY LIST.");
    }

    if (data.stackHeapInfo.heapRecommendedBytes == 0) {
        data.warnings.push_back(L"Heap section or heap symbols were not identified in this IAR map.");
    }

    const std::uint64_t writableRegionTotal = MergeIntervalUnionBytes(writableIntervals);
    if (writableRegionTotal > 0) {
        data.ramLimitBytes = writableRegionTotal;
        data.logLines.push_back(L"RAM limit derived from writable PLACEMENT SUMMARY regions.");
    }

    const std::uint64_t splitRamBytes = data.programSize.rwDataBytes + data.programSize.ziDataBytes;
    if (readwriteDataBytes > 0 && splitRamBytes != readwriteDataBytes) {
        data.warnings.push_back(
            L"IAR readwrite data memory total does not exactly match the parser split of RW-data + ZI-data.");
    } else if (readwriteDataBytes > 0) {
        data.logLines.push_back(L"Validated readwrite data memory total against parsed RW-data + ZI-data.");
    }

    data.success = data.programSize.hasCode || data.programSize.hasRoData ||
                   data.programSize.hasRwData || data.programSize.hasZiData;
    if (!data.success) {
        data.errorMessage = L"IAR ILINK map parse found no recognized sections.";
        data.logLines.push_back(data.errorMessage);
        return data;
    }

    data.logLines.push_back(L"IAR parsing finished.");
    return data;
}
