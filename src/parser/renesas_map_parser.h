#pragma once

#include "parser/i_map_parser.h"

class RenesasMapParser : public IMapParser {
public:
    explicit RenesasMapParser(CompilerFamily family);

    bool SupportsFamily(CompilerFamily family) const override;
    MapParseData Parse(const ParseOptions& options) const override;

private:
    CompilerFamily m_family;
};
