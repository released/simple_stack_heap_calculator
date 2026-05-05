#include "parser/renesas_cc_rh_map_parser.h"

#include "parser/renesas_map_parser.h"

bool RenesasCcRhMapParser::SupportsFamily(CompilerFamily family) const {
    return family == CompilerFamily::Rh850;
}

MapParseData RenesasCcRhMapParser::Parse(const ParseOptions& options) const {
    return RenesasMapParser(CompilerFamily::Rh850).Parse(options);
}
