#include "parser/ghs_map_parser.h"

bool GhsMapParser::SupportsFamily(CompilerFamily family) const {
    return family == CompilerFamily::Ghs;
}

MapParseData GhsMapParser::Parse(const ParseOptions& options) const {
    MapParseData data;
    data.success = false;
    data.compilerDisplayName = L"GHS";
    data.logLines.push_back(L"Reading map file: " + options.mapPath);
    data.logLines.push_back(L"GHS map detected, but the parser is currently detection-first and not fully implemented.");
    data.errorMessage = L"GHS parser is not fully implemented yet. Detection is available, but parsing support is still incomplete.";
    return data;
}
