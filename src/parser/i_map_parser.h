#pragma once

#include <string>

#include "core/compiler_family.h"
#include "model/analysis_types.h"

struct ParseOptions {
    std::wstring mapPath;
};

class IMapParser {
public:
    virtual ~IMapParser() = default;

    virtual bool SupportsFamily(CompilerFamily family) const = 0;
    virtual MapParseData Parse(const ParseOptions& options) const = 0;
};
