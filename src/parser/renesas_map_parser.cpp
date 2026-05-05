#include "parser/renesas_map_parser.h"

#include <cstdlib>
#include <regex>
#include <utility>

#include "util/file_util.h"
#include "util/string_util.h"

namespace {

struct RenesasSection {
    std::string name;
    std::uint64_t startAddress = 0;
    std::uint64_t endAddress = 0;
    std::uint64_t sizeBytes = 0;
};

struct RenesasTotals {
    bool ramDataFound = false;
    bool romDataFound = false;
    bool programFound = false;
    std::uint64_t ramDataBytes = 0;
    std::uint64_t romDataBytes = 0;
    std::uint64_t programBytes = 0;
};

std::wstring NarrowToWide(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

bool TryParseHex(const std::string& text, std::uint64_t* out_value) {
    if (!out_value || text.empty()) {
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

bool TryParseSectionValueLine(const std::string& line,
                              std::uint64_t* out_start,
                              std::uint64_t* out_end,
                              std::uint64_t* out_size) {
    if (!out_start || !out_end || !out_size) {
        return false;
    }

    const std::vector<std::string> tokens = stringutil::SplitWhitespace(line);
    if (tokens.size() < 4) {
        return false;
    }

    return TryParseHex(tokens[0], out_start) &&
           TryParseHex(tokens[1], out_end) &&
           TryParseHex(tokens[2], out_size);
}

bool TryParseTotalLine(const std::string& line, RenesasTotals* out_totals) {
    if (!out_totals) {
        return false;
    }

    static const std::regex kPattern(
        R"(^\s*(RAMDATA|ROMDATA|PROGRAM)\s+SECTION:\s+([0-9A-Fa-f]+)\s+Byte\(s\).*$)",
        std::regex::ECMAScript);
    std::smatch match;
    if (!std::regex_match(line, match, kPattern)) {
        return false;
    }

    std::uint64_t value = 0;
    if (!TryParseHex(match[2].str(), &value)) {
        return false;
    }

    const std::string kind = match[1].str();
    if (kind == "RAMDATA") {
        out_totals->ramDataFound = true;
        out_totals->ramDataBytes = value;
    } else if (kind == "ROMDATA") {
        out_totals->romDataFound = true;
        out_totals->romDataBytes = value;
    } else if (kind == "PROGRAM") {
        out_totals->programFound = true;
        out_totals->programBytes = value;
    }
    return true;
}

bool IsStackLabel(const std::string& text) {
    return stringutil::ContainsInsensitive(text, "STACK");
}

bool IsHeapLabel(const std::string& text) {
    return stringutil::ContainsInsensitive(text, "HEAP");
}

bool IsRl78FamilyMap(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "RL78") ||
            stringutil::ContainsInsensitive(line, "CC-RL")) {
            return true;
        }
    }
    return false;
}

bool IsRh850FamilyMap(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "RH850") ||
            stringutil::ContainsInsensitive(line, "CC-RH") ||
            stringutil::ContainsInsensitive(line, "v850")) {
            return true;
        }
    }
    return false;
}

bool IsSectionNameLine(const std::string& line) {
    const std::string trimmed = stringutil::Trim(line);
    if (trimmed.empty()) {
        return false;
    }
    if (trimmed == "SECTION                            START      END         SIZE   ALIGN") {
        return false;
    }
    if (trimmed.find("***") == 0) {
        return false;
    }
    if (trimmed.find("SECTION=") == 0 || trimmed.find("FILE=") == 0) {
        return false;
    }
    return true;
}

bool IsRl78RomCodeSection(const std::string& name) {
    return name == ".text" ||
           name == ".textf" ||
           name == ".RLIB" ||
           name == ".SLIB";
}

bool IsRl78RomRoSection(const std::string& name) {
    return name == ".vect" ||
           name == ".option_byte" ||
           name == ".security_id" ||
           name == ".monitor1" ||
           name == ".monitor2" ||
           name == ".const" ||
           name == ".constf";
}

bool IsRl78RomRwImageSection(const std::string& name) {
    return name == ".data" ||
           name == ".sdata" ||
           name == ".init_array";
}

bool IsRl78RamRwSection(const std::string& name) {
    return name == ".dataR" ||
           name == ".sdataR";
}

bool IsRl78ZiSection(const std::string& name) {
    return name == ".bss" ||
           name == ".sbss";
}

bool IsRh850CodeSection(const std::string& name) {
    return name == "RESET" || name == ".text";
}

bool IsRh850RoSection(const std::string& name) {
    return name == "EIINTTBL.const" ||
           name == ".const" ||
           name == ".INIT_DSEC.const" ||
           name == ".INIT_BSEC.const" ||
           name == "privateData.const";
}

bool IsRh850RamRwSection(const std::string& name) {
    return name == ".data.R";
}

bool IsRh850ZiSection(const std::string& name) {
    return name == ".bss" ||
           name == ".stack.bss" ||
           name == "privateData.bss";
}

bool IsRh850RomRwImageSection(const std::string& name) {
    return name == ".data" || name == "privateData.data";
}

std::vector<RenesasSection> ParseMappingList(const std::vector<std::string>& lines) {
    std::vector<RenesasSection> sections;
    bool inMappingList = false;
    std::string pendingSectionName;

    for (const std::string& line : lines) {
        if (stringutil::ContainsInsensitive(line, "*** Mapping List ***")) {
            inMappingList = true;
            pendingSectionName.clear();
            continue;
        }
        if (!inMappingList) {
            continue;
        }
        if (stringutil::ContainsInsensitive(line, "*** Total Section Size ***") ||
            stringutil::ContainsInsensitive(line, "*** Symbol List ***")) {
            break;
        }

        std::uint64_t startAddress = 0;
        std::uint64_t endAddress = 0;
        std::uint64_t sizeBytes = 0;
        if (!pendingSectionName.empty() &&
            TryParseSectionValueLine(line, &startAddress, &endAddress, &sizeBytes)) {
            RenesasSection section;
            section.name = pendingSectionName;
            section.startAddress = startAddress;
            section.endAddress = endAddress;
            section.sizeBytes = sizeBytes;
            sections.push_back(section);
            pendingSectionName.clear();
            continue;
        }

        if (IsSectionNameLine(line)) {
            pendingSectionName = stringutil::Trim(line);
        }
    }

    return sections;
}

RenesasTotals ParseTotals(const std::vector<std::string>& lines) {
    RenesasTotals totals;
    for (const std::string& line : lines) {
        TryParseTotalLine(line, &totals);
    }
    return totals;
}

bool TryFindSymbolAddress(const std::vector<std::string>& lines,
                          const std::vector<std::string>& names,
                          std::uint64_t* out_address) {
    if (!out_address) {
        return false;
    }

    for (size_t i = 0; i + 1 < lines.size(); ++i) {
        const std::string symbolLine = stringutil::Trim(lines[i]);
        if (symbolLine.empty()) {
            continue;
        }

        bool nameMatched = false;
        for (const std::string& name : names) {
            if (symbolLine == name) {
                nameMatched = true;
                break;
            }
        }
        if (!nameMatched) {
            continue;
        }

        const std::vector<std::string> tokens = stringutil::SplitWhitespace(lines[i + 1]);
        if (tokens.empty()) {
            continue;
        }
        if (TryParseHex(tokens[0], out_address)) {
            return true;
        }
    }

    return false;
}

void AppendSectionRegion(const RenesasSection& section, std::vector<MemoryRegion>* out_regions) {
    if (!out_regions) {
        return;
    }

    MemoryRegion region;
    region.name = NarrowToWide(section.name);
    region.baseAddress = section.startAddress;
    region.sizeBytes = section.sizeBytes;
    region.usedBytes = section.sizeBytes;
    out_regions->push_back(region);
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

void ParseRl78Sections(const std::vector<RenesasSection>& sections,
                       MapParseData* out_result,
                       std::uint64_t* out_romRwImageBytes) {
    if (!out_result || !out_romRwImageBytes) {
        return;
    }

    for (const RenesasSection& section : sections) {
        AppendSectionRegion(section, &out_result->memoryRegions);

        if (IsRl78RomCodeSection(section.name)) {
            out_result->programSize.codeBytes += section.sizeBytes;
            out_result->programSize.hasCode = true;
        } else if (IsRl78RomRoSection(section.name)) {
            out_result->programSize.roDataBytes += section.sizeBytes;
            out_result->programSize.hasRoData = true;
        } else if (IsRl78RomRwImageSection(section.name)) {
            *out_romRwImageBytes += section.sizeBytes;
        } else if (IsRl78RamRwSection(section.name)) {
            out_result->programSize.rwDataBytes += section.sizeBytes;
            out_result->programSize.hasRwData = true;
        } else if (IsRl78ZiSection(section.name)) {
            out_result->programSize.ziDataBytes += section.sizeBytes;
            out_result->programSize.hasZiData = true;
        }
    }
}

void ParseRh850Sections(const std::vector<RenesasSection>& sections,
                        MapParseData* out_result,
                        std::uint64_t* out_romRwImageBytes) {
    if (!out_result || !out_romRwImageBytes) {
        return;
    }

    for (const RenesasSection& section : sections) {
        AppendSectionRegion(section, &out_result->memoryRegions);

        if (IsRh850CodeSection(section.name)) {
            out_result->programSize.codeBytes += section.sizeBytes;
            out_result->programSize.hasCode = true;
        } else if (IsRh850RoSection(section.name)) {
            out_result->programSize.roDataBytes += section.sizeBytes;
            out_result->programSize.hasRoData = true;
        } else if (IsRh850RomRwImageSection(section.name)) {
            *out_romRwImageBytes += section.sizeBytes;
        } else if (IsRh850RamRwSection(section.name)) {
            out_result->programSize.rwDataBytes += section.sizeBytes;
            out_result->programSize.hasRwData = true;
        } else if (IsRh850ZiSection(section.name)) {
            out_result->programSize.ziDataBytes += section.sizeBytes;
            out_result->programSize.hasZiData = true;
            if (section.name == ".stack.bss") {
                out_result->stackHeapInfo.stackBase = section.startAddress;
                out_result->stackHeapInfo.stackLimit = section.startAddress + section.sizeBytes;
                out_result->stackHeapInfo.stackSizeBytes = section.sizeBytes;
                out_result->stackHeapInfo.stackRecommendedBytes = section.sizeBytes;
                out_result->stackHeapInfo.stackIncludedInStaticRam = true;
                out_result->stackHeapInfo.source = L"Section .stack.bss";
                out_result->stackHeapInfo.confidence = L"High";
            }
        }
    }
}

void FinalizeRwFallback(MapParseData* out_result, std::uint64_t romRwImageBytes) {
    if (!out_result) {
        return;
    }
    if (out_result->programSize.rwDataBytes == 0 && romRwImageBytes > 0) {
        out_result->programSize.rwDataBytes = romRwImageBytes;
        out_result->programSize.hasRwData = true;
        out_result->warnings.push_back(L"RW-data was derived from ROM-side initialized data sections because no runtime RW section was found.");
    }
}

void ApplyRl78Symbols(const std::vector<std::string>& lines, MapParseData* out_result) {
    if (!out_result) {
        return;
    }

    std::uint64_t ramStart = 0;
    std::uint64_t ramEnd = 0;
    std::uint64_t stackStart = 0;
    std::uint64_t stackEnd = 0;

    const bool hasRamStart = TryFindSymbolAddress(lines, {"__RAM_ADDR_START"}, &ramStart);
    const bool hasRamEnd = TryFindSymbolAddress(lines, {"__RAM_ADDR_END"}, &ramEnd);
    const bool hasStackStart = TryFindSymbolAddress(lines, {"__STACK_ADDR_START"}, &stackStart);
    const bool hasStackEnd = TryFindSymbolAddress(lines, {"__STACK_ADDR_END"}, &stackEnd);

    if (hasRamStart) {
        AddSymbol(&out_result->symbols, "__RAM_ADDR_START", ramStart, SymbolType::Unknown);
    }
    if (hasRamEnd) {
        AddSymbol(&out_result->symbols, "__RAM_ADDR_END", ramEnd, SymbolType::Unknown);
    }
    if (hasStackStart) {
        AddSymbol(&out_result->symbols, "__STACK_ADDR_START", stackStart, SymbolType::Stack);
    }
    if (hasStackEnd) {
        AddSymbol(&out_result->symbols, "__STACK_ADDR_END", stackEnd, SymbolType::Stack);
    }

    if (hasRamStart && hasRamEnd && ramEnd > ramStart) {
        out_result->ramLimitBytes = ramEnd - ramStart;
        out_result->logLines.push_back(L"RAM limit derived from __RAM_ADDR_START / __RAM_ADDR_END.");
    }

    if (hasStackStart && hasStackEnd && stackStart > stackEnd) {
        out_result->stackHeapInfo.stackBase = stackEnd;
        out_result->stackHeapInfo.stackLimit = stackStart;
        out_result->stackHeapInfo.stackSizeBytes = stackStart - stackEnd;
        out_result->stackHeapInfo.stackRecommendedBytes = out_result->stackHeapInfo.stackSizeBytes;
        out_result->stackHeapInfo.stackIncludedInStaticRam = false;
        out_result->stackHeapInfo.source = L"Symbols __STACK_ADDR_END / __STACK_ADDR_START";
        out_result->stackHeapInfo.confidence = L"Medium";
        out_result->stackHeapInfo.stackRecommendationEstimated = true;
    }
}

void ApplyRh850Symbols(const std::vector<std::string>& lines, MapParseData* out_result) {
    if (!out_result) {
        return;
    }

    std::uint64_t stackStart = 0;
    std::uint64_t stackEnd = 0;
    const bool hasStackStart =
        TryFindSymbolAddress(lines, {"__s.stack.bss", "__S_stack_bss"}, &stackStart);
    const bool hasStackEnd =
        TryFindSymbolAddress(lines, {"__e.stack.bss", "__E_stack_bss"}, &stackEnd);

    if (hasStackStart) {
        AddSymbol(&out_result->symbols, "__s.stack.bss", stackStart, SymbolType::Stack);
    }
    if (hasStackEnd) {
        AddSymbol(&out_result->symbols, "__e.stack.bss", stackEnd, SymbolType::Stack);
    }

    if (out_result->stackHeapInfo.stackSizeBytes == 0 &&
        hasStackStart &&
        hasStackEnd &&
        stackEnd > stackStart) {
        out_result->stackHeapInfo.stackBase = stackStart;
        out_result->stackHeapInfo.stackLimit = stackEnd;
        out_result->stackHeapInfo.stackSizeBytes = stackEnd - stackStart;
        out_result->stackHeapInfo.stackRecommendedBytes = out_result->stackHeapInfo.stackSizeBytes;
        out_result->stackHeapInfo.stackIncludedInStaticRam = true;
        out_result->stackHeapInfo.source = L"Symbols __s.stack.bss / __e.stack.bss";
        out_result->stackHeapInfo.confidence = L"High";
    }
}

}  // namespace

RenesasMapParser::RenesasMapParser(CompilerFamily family)
    : m_family(family) {
}

bool RenesasMapParser::SupportsFamily(CompilerFamily family) const {
    return family == m_family;
}

MapParseData RenesasMapParser::Parse(const ParseOptions& options) const {
    MapParseData result;
    result.compilerDisplayName = m_family == CompilerFamily::Rl78 ? L"RL78 (CC-RL)" : L"RH850 (CC-RH)";
    result.logLines.push_back(L"Reading map file: " + options.mapPath);

    std::vector<std::string> lines;
    std::wstring readError;
    if (!fileutil::ReadAllLines(options.mapPath, &lines, &readError)) {
        result.errorMessage = L"Failed to read map file. " + readError;
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    const bool isRl78Map = IsRl78FamilyMap(lines);
    const bool isRh850Map = IsRh850FamilyMap(lines);
    if (m_family == CompilerFamily::Rl78 && !isRl78Map) {
        result.errorMessage = L"Selected RL78 tab, but this map file does not look like a CC-RL / RL78 map.";
        result.logLines.push_back(result.errorMessage);
        return result;
    }
    if (m_family == CompilerFamily::Rh850 && !isRh850Map) {
        result.errorMessage = L"Selected RH850 tab, but this map file does not look like a CC-RH / RH850 map.";
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    result.logLines.push_back(L"Compiler detected: " + result.compilerDisplayName);

    const std::vector<RenesasSection> sections = ParseMappingList(lines);
    const RenesasTotals totals = ParseTotals(lines);

    std::uint64_t romRwImageBytes = 0;
    if (m_family == CompilerFamily::Rl78) {
        ParseRl78Sections(sections, &result, &romRwImageBytes);
        ApplyRl78Symbols(lines, &result);
    } else {
        ParseRh850Sections(sections, &result, &romRwImageBytes);
        ApplyRh850Symbols(lines, &result);
    }

    FinalizeRwFallback(&result, romRwImageBytes);

    if (result.stackHeapInfo.stackRecommendedBytes == 0) {
        result.stackHeapInfo.stackRecommendedBytes = result.stackHeapInfo.stackSizeBytes;
    }
    if (result.stackHeapInfo.heapRecommendedBytes == 0) {
        result.stackHeapInfo.heapRecommendedBytes = result.stackHeapInfo.heapSizeBytes;
    }

    if (totals.ramDataFound) {
        const std::uint64_t staticRam = result.programSize.rwDataBytes + result.programSize.ziDataBytes;
        result.logLines.push_back(L"RAMDATA summary total = " + NarrowToWide(std::to_string(totals.ramDataBytes)) + L" bytes.");
        if (staticRam != totals.ramDataBytes) {
            result.warnings.push_back(
                L"Parsed RW-data + ZI-data does not match the linker RAMDATA summary.");
        } else {
            result.logLines.push_back(L"Validated RAMDATA summary against parsed RW-data + ZI-data.");
        }
    }
    if (totals.romDataFound) {
        result.logLines.push_back(L"ROMDATA summary total = " + NarrowToWide(std::to_string(totals.romDataBytes)) + L" bytes.");
    }
    if (totals.programFound) {
        result.logLines.push_back(L"PROGRAM summary total = " + NarrowToWide(std::to_string(totals.programBytes)) + L" bytes.");
    }

    if (result.stackHeapInfo.stackSizeBytes == 0) {
        result.warnings.push_back(L"Stack section or symbols were not identified.");
    }
    if (result.stackHeapInfo.heapSizeBytes == 0) {
        result.warnings.push_back(L"Heap section was not identified in this map.");
    }

    result.success = result.programSize.hasCode || result.programSize.hasRoData ||
                     result.programSize.hasRwData || result.programSize.hasZiData;
    if (!result.success) {
        result.errorMessage = result.compilerDisplayName + L" map parse found no recognized sections.";
        result.logLines.push_back(result.errorMessage);
        return result;
    }

    result.logLines.push_back(result.compilerDisplayName + L" parsing finished.");
    return result;
}
