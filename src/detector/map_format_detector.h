#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/compiler_family.h"
#include "model/map_format_types.h"

class MapFormatDetector {
public:
    MapFormatDetectionResult DetectFromFile(const std::wstring& mapPath) const;
    MapFormatDetectionResult DetectFromLines(const std::vector<std::string>& lines) const;
};

std::optional<CompilerFamily> TryMapFormatToCompilerFamily(MapFormat format);
