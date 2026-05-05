#pragma once

#include <optional>
#include <string>

#include "core/compiler_family.h"
#include "model/analysis_types.h"

struct AnalysisRequest {
    CompilerFamily manualFamily = CompilerFamily::Keil;
    std::wstring mapPath;
    std::optional<std::uint64_t> ramLimitOverrideBytes;
    bool preferDetectedParser = true;
};

class AnalysisEngine {
public:
    AnalysisResult Run(const AnalysisRequest& request) const;
};
