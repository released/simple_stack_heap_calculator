#pragma once

#include "parser/i_map_parser.h"

class PlaceholderMapParser : public IMapParser {
public:
    explicit PlaceholderMapParser(CompilerFamily family);

    bool SupportsFamily(CompilerFamily family) const override;
    MapParseData Parse(const ParseOptions& options) const override;

private:
    CompilerFamily m_family;
};
