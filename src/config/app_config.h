#pragma once

#include <string>

#include "core/compiler_family.h"

struct AppConfigData {
    std::wstring mapPath;
    std::wstring ramLimitText;
};

class AppConfig {
public:
    bool Load(AppConfigData* out_data, std::wstring* out_error) const;
    bool Save(const AppConfigData& data, std::wstring* out_error) const;
    std::wstring GetConfigPath() const;

private:
    bool EnsureFileExists(std::wstring* out_error) const;
};
