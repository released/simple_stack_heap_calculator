#include "parser/gcc_map_parser.h"

#include <cstdlib>
#include <regex>

#include "util/file_util.h"
#include "util/string_util.h"

namespace {

struct GccSection {
    std::string name;
    std::uint64_t address = 0;
    std::uint64_t sizeBytes = 0;
};

std::wstring NarrowToWide(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

bool TryParseHexToken(const std::string& text, std::uint64_t* out_value) {
    if (!out_value || text.size() < 3 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) {
        return false;
    }
    char* end = nullptr;
    unsigned long long value = std::strtoull(text.c_str(), &end, 16);
    if (!end || *end != '\0') {
        return false;
    }
    *out_value = static_cast<std::uint64_t>(value);
    return true;
}

bool IsWritableMemoryAttributes(const std::string& attrs) {
    return attrs.find('w') != std::string::npos || attrs.find('W') != std::string::npos;
}

bool IsCodeSection(const std::string& name) {
    return name == ".text" ||
           name == ".boot2" ||
           name == ".init" ||
           name == ".fini";
}

bool IsRoSection(const std::string& name) {
    return name == ".rodata" ||
           name.find(".rodata.") == 0 ||
           name == ".ARM.extab" ||
           name == ".ARM.exidx" ||
           name == ".init_array" ||
           name == ".fini_array" ||
           name == ".preinit_array" ||
           name.find(".binary_info") == 0;
}

bool IsRwSection(const std::string& name) {
    return name == ".data" || name == ".tdata";
}

bool IsZiSection(const std::string& name) {
    return name == ".bss" || name == ".tbss";
}

bool IsHeapSection(const std::string& name) {
    return name == ".heap" || stringutil::ContainsInsensitive(name, "heap");
}

bool IsStackSection(const std::string& name) {
    return stringutil::ContainsInsensitive(name, "stack");
}

bool TryFindSymbolValue(const std::vector<std::string>& lines,
                        const std::vector<std::string>& names,
                        std::uint64_t* out_value) {
    if (!out_value) {
        return false;
    }

    for (const std::string& line : lines) {
        for (const std::string& name : names) {
            if (!stringutil::ContainsInsensitive(line, name)) {
                continue;
            }

            const std::vector<std::string> tokens = stringutil::SplitWhitespace(line);
            for (const std::string& token : tokens) {
                std::uint64_t parsed = 0;
                if (TryParseHexToken(token, &parsed)) {
                    *out_value = parsed;
                    return true;
                }
            }
        }
    }

    return false;
}

void AddSymbol(std::vector<SymbolEntry>* out_symbols,
               const std::string& name,
               std::uint64_t address,
               SymbolType type) {
    if (!out_symbols) {
        return;
    }
    SymbolEntry symbol;
    symbol.address = address;
    symbol.type = type;
    symbol.symbolName = NarrowToWide(name);
    out_symbols->push_back(symbol);
}

}  // namespace

bool GccMapParser::SupportsFamily(CompilerFamily family) const {
    return family == CompilerFamily::Gcc;
}

MapParseData GccMapParser::Parse(const ParseOptions& options) const {
    MapParseData result;
    result.compilerDisplayName = L"GCC";
    result.logLines.push_back(L"Reading map file: " + options.mapPath);

    std::vector<std::string> lines;
    std::wstring readError;
    if (!fileutil::ReadAllLines(options.mapPath, &lines, &readError)) {
        result.errorMessage = L"Failed to read map file. " + readError;
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    bool hasMemoryConfig = false;
    bool hasLinkerMap = false;
    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "Memory Configuration")) {
            hasMemoryConfig = true;
        }
        if (stringutil::ContainsInsensitive(line, "Linker script and memory map")) {
            hasLinkerMap = true;
        }
    }
    if (!hasMemoryConfig || !hasLinkerMap) {
        result.errorMessage = L"Selected GCC tab, but this map file does not look like a GNU ld map.";
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    result.logLines.push_back(L"Compiler detected: GCC / GNU ld");

    bool inMemoryConfig = false;
    bool inLinkerMap = false;
    std::uint64_t writableRegionTotal = 0;

    static const std::regex kMemoryRegionPattern(
        R"(^\s*([^\s]+)\s+(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+)\s+([A-Za-z*]+)\s*$)",
        std::regex::ECMAScript);
    static const std::regex kTopLevelSectionPattern(
        R"(^(\.[^\s]+)\s+(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+)(?:\s+.*)?$)",
        std::regex::ECMAScript);

    std::vector<GccSection> sections;

    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "Memory Configuration")) {
            inMemoryConfig = true;
            inLinkerMap = false;
            continue;
        }
        if (stringutil::ContainsInsensitive(line, "Linker script and memory map")) {
            inMemoryConfig = false;
            inLinkerMap = true;
            continue;
        }

        if (inMemoryConfig) {
            if (stringutil::Trim(line).empty() || stringutil::ContainsInsensitive(line, "Name             Origin")) {
                continue;
            }

            std::smatch match;
            if (!std::regex_match(line, match, kMemoryRegionPattern)) {
                continue;
            }

            std::uint64_t origin = 0;
            std::uint64_t length = 0;
            if (!TryParseHexToken(match[2].str(), &origin) || !TryParseHexToken(match[3].str(), &length)) {
                continue;
            }

            MemoryRegion region;
            region.name = NarrowToWide(match[1].str());
            region.baseAddress = origin;
            region.sizeBytes = length;
            region.usedBytes = 0;
            result.memoryRegions.push_back(region);

            if (IsWritableMemoryAttributes(match[4].str())) {
                writableRegionTotal += length;
            }
            continue;
        }

        if (!inLinkerMap) {
            continue;
        }

        std::smatch match;
        if (!std::regex_match(line, match, kTopLevelSectionPattern)) {
            continue;
        }

        GccSection section;
        section.name = match[1].str();
        if (!TryParseHexToken(match[2].str(), &section.address) ||
            !TryParseHexToken(match[3].str(), &section.sizeBytes)) {
            continue;
        }
        sections.push_back(section);
    }

    std::uint64_t stackSectionBytes = 0;
    std::uint64_t heapSectionBytes = 0;
    std::uint64_t stackSectionBase = 0;
    std::uint64_t heapSectionBase = 0;

    for (const GccSection& section : sections) {
        for (MemoryRegion& region : result.memoryRegions) {
            const std::uint64_t regionEnd = region.baseAddress + region.sizeBytes;
            const std::uint64_t sectionEnd = section.address + section.sizeBytes;
            if (section.address >= region.baseAddress && sectionEnd <= regionEnd) {
                region.usedBytes += section.sizeBytes;
                break;
            }
        }

        if (IsCodeSection(section.name)) {
            result.programSize.codeBytes += section.sizeBytes;
            result.programSize.hasCode = true;
        } else if (IsRoSection(section.name)) {
            result.programSize.roDataBytes += section.sizeBytes;
            result.programSize.hasRoData = true;
        } else if (IsRwSection(section.name)) {
            result.programSize.rwDataBytes += section.sizeBytes;
            result.programSize.hasRwData = true;
        } else if (IsZiSection(section.name)) {
            result.programSize.ziDataBytes += section.sizeBytes;
            result.programSize.hasZiData = true;
        } else if (IsHeapSection(section.name)) {
            if (heapSectionBytes == 0) {
                heapSectionBase = section.address;
            }
            heapSectionBytes += section.sizeBytes;
        } else if (IsStackSection(section.name)) {
            if (stackSectionBytes == 0) {
                stackSectionBase = section.address;
            }
            stackSectionBytes += section.sizeBytes;
        }
    }

    if (heapSectionBytes > 0) {
        result.stackHeapInfo.heapBase = heapSectionBase;
        result.stackHeapInfo.heapLimit = heapSectionBase + heapSectionBytes;
        result.stackHeapInfo.heapSizeBytes = heapSectionBytes;
        result.stackHeapInfo.heapRecommendedBytes = heapSectionBytes;
        result.stackHeapInfo.heapIncludedInStaticRam = false;
        result.stackHeapInfo.source = L"Top-level GNU ld .heap section";
        result.stackHeapInfo.confidence = L"High";
    }

    if (stackSectionBytes > 0) {
        result.stackHeapInfo.stackBase = stackSectionBase;
        result.stackHeapInfo.stackLimit = stackSectionBase + stackSectionBytes;
        result.stackHeapInfo.stackSizeBytes = stackSectionBytes;
        result.stackHeapInfo.stackRecommendedBytes = stackSectionBytes;
        result.stackHeapInfo.stackIncludedInStaticRam = false;
        if (result.stackHeapInfo.source.empty()) {
            result.stackHeapInfo.source = L"Top-level GNU ld stack section";
        } else {
            result.stackHeapInfo.source += L"; stack section";
        }
        result.stackHeapInfo.confidence = L"High";
    }

    std::uint64_t stackBottom = result.stackHeapInfo.stackBase;
    std::uint64_t stackLimit = result.stackHeapInfo.stackBase;
    std::uint64_t stackTop = result.stackHeapInfo.stackLimit;
    if (TryFindSymbolValue(lines, {"__StackLimit", "__stack_limit"}, &stackLimit)) {
        stackBottom = stackLimit;
        AddSymbol(&result.symbols, "__StackLimit", stackLimit, SymbolType::Stack);
    }
    if (TryFindSymbolValue(lines, {"__StackBottom", "__stack_bottom"}, &stackBottom)) {
        AddSymbol(&result.symbols, "__StackBottom", stackBottom, SymbolType::Stack);
    }
    if (TryFindSymbolValue(lines, {"__StackTop", "__stack_top"}, &stackTop)) {
        AddSymbol(&result.symbols, "__StackTop", stackTop, SymbolType::Stack);
    }
    if (stackTop > stackBottom) {
        result.stackHeapInfo.stackBase = stackBottom;
        result.stackHeapInfo.stackLimit = stackTop;
        result.stackHeapInfo.stackSizeBytes = stackTop - stackBottom;
        result.stackHeapInfo.stackRecommendedBytes = result.stackHeapInfo.stackSizeBytes;
        result.stackHeapInfo.stackIncludedInStaticRam = false;
        if (stackLimit > 0) {
            result.stackHeapInfo.source = L"Symbols __StackLimit / __StackTop";
        } else {
            result.stackHeapInfo.source = L"Symbols __StackBottom / __StackTop";
        }
        result.stackHeapInfo.confidence = L"High";
    }

    std::uint64_t heapBase = result.stackHeapInfo.heapBase;
    std::uint64_t heapLimit = result.stackHeapInfo.heapLimit;
    if (TryFindSymbolValue(lines, {"__HeapBase", "__heap_base"}, &heapBase)) {
        AddSymbol(&result.symbols, "__HeapBase", heapBase, SymbolType::Heap);
    }
    if (TryFindSymbolValue(lines, {"__HeapLimit", "__heap_limit"}, &heapLimit)) {
        AddSymbol(&result.symbols, "__HeapLimit", heapLimit, SymbolType::Heap);
    }
    if (heapLimit > heapBase) {
        result.stackHeapInfo.heapBase = heapBase;
        result.stackHeapInfo.heapLimit = heapLimit;
        result.stackHeapInfo.heapSizeBytes = heapLimit - heapBase;
        result.stackHeapInfo.heapRecommendedBytes = result.stackHeapInfo.heapSizeBytes;
        result.stackHeapInfo.heapIncludedInStaticRam = false;
        if (result.stackHeapInfo.source.empty()) {
            result.stackHeapInfo.source = L"Symbols __HeapBase / __HeapLimit";
        } else {
            result.stackHeapInfo.source += L"; heap symbols";
        }
        result.stackHeapInfo.confidence = L"High";
    }

    if (writableRegionTotal > 0) {
        result.ramLimitBytes = writableRegionTotal;
        result.logLines.push_back(L"RAM limit uses the sum of writable memory regions from Memory Configuration.");
    }

    if (result.stackHeapInfo.stackRecommendedBytes == 0) {
        result.stackHeapInfo.stackRecommendedBytes = result.stackHeapInfo.stackSizeBytes;
    }
    if (result.stackHeapInfo.heapRecommendedBytes == 0) {
        result.stackHeapInfo.heapRecommendedBytes = result.stackHeapInfo.heapSizeBytes;
    }

    if (result.stackHeapInfo.stackSizeBytes == 0) {
        result.warnings.push_back(L"Stack section or stack symbols were not identified.");
    }
    if (result.stackHeapInfo.heapSizeBytes == 0) {
        result.warnings.push_back(L"Heap section or heap symbols were not identified.");
    }

    result.success = result.programSize.hasCode || result.programSize.hasRoData ||
                     result.programSize.hasRwData || result.programSize.hasZiData;
    if (!result.success) {
        result.errorMessage = L"GCC map parse found no recognized sections.";
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    result.logLines.push_back(L"GCC parsing finished.");
    return result;
}
