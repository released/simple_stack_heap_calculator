#include "parser/placeholder_map_parser.h"

PlaceholderMapParser::PlaceholderMapParser(CompilerFamily family)
    : m_family(family) {
}

bool PlaceholderMapParser::SupportsFamily(CompilerFamily family) const {
    return family == m_family;
}

MapParseData PlaceholderMapParser::Parse(const ParseOptions&) const {
    MapParseData data;
    data.success = false;
    data.compilerDisplayName = CompilerFamilyToDisplayName(m_family);
    data.errorMessage = data.compilerDisplayName + L" parser is not implemented yet. Current milestone is KEIL v6 first.";
    data.logLines.push_back(L"Selected parser family: " + data.compilerDisplayName);
    data.logLines.push_back(L"Parser status: not implemented yet.");
    return data;
}
