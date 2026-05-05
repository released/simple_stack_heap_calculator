#pragma once

#include <memory>

#include "parser/i_map_parser.h"

class ParserFactory {
public:
    static std::unique_ptr<IMapParser> Create(CompilerFamily family);
};
