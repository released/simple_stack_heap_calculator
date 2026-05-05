#pragma once

#include "parser/i_map_parser.h"

class IarMapParser : public IMapParser {
public:
    bool SupportsFamily(CompilerFamily family) const override;
    MapParseData Parse(const ParseOptions& options) const override;
};
