#include "parser/renesas_cc_rl_map_parser.h"

#include "parser/renesas_map_parser.h"

bool RenesasCcRlMapParser::SupportsFamily(CompilerFamily family) const {
    return family == CompilerFamily::Rl78;
}

MapParseData RenesasCcRlMapParser::Parse(const ParseOptions& options) const {
    return RenesasMapParser(CompilerFamily::Rl78).Parse(options);
}
