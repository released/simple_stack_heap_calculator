#include "config/app_config.h"

#include <Windows.h>

#include <filesystem>
#include <vector>

namespace {

constexpr wchar_t kIniFileName[] = L"SimpleStackHeapCalculator.ini";
constexpr wchar_t kSectionUi[] = L"UI";
constexpr wchar_t kKeyMapPath[] = L"MapPath";
constexpr wchar_t kKeyRamLimit[] = L"RamLimit";

std::wstring GetExecutableDirectory() {
    std::vector<wchar_t> buffer(MAX_PATH, L'\0');
    DWORD copied = 0;
    while (true) {
        copied = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (copied == 0) {
            return std::filesystem::current_path().wstring();
        }
        if (copied < buffer.size() - 1) {
            break;
        }
        buffer.resize(buffer.size() * 2, L'\0');
    }

    std::filesystem::path exePath(std::wstring(buffer.data(), copied));
    return exePath.parent_path().wstring();
}

std::wstring ReadIniString(const wchar_t* section, const wchar_t* key, const std::wstring& path) {
    std::vector<wchar_t> buffer(512, L'\0');
    while (true) {
        DWORD copied = ::GetPrivateProfileStringW(section, key, L"", buffer.data(),
                                                  static_cast<DWORD>(buffer.size()), path.c_str());
        if (copied < buffer.size() - 1) {
            return std::wstring(buffer.data(), copied);
        }
        buffer.resize(buffer.size() * 2, L'\0');
    }
}

bool WriteIniString(const wchar_t* section, const wchar_t* key, const wchar_t* value,
                    const std::wstring& path, std::wstring* out_error) {
    if (::WritePrivateProfileStringW(section, key, value, path.c_str()) != FALSE) {
        return true;
    }

    if (out_error) {
        *out_error = L"Unable to write config file: " + path;
    }
    return false;
}

}  // namespace

std::wstring AppConfig::GetConfigPath() const {
    return (std::filesystem::path(GetExecutableDirectory()) / kIniFileName).wstring();
}

bool AppConfig::EnsureFileExists(std::wstring* out_error) const {
    const std::wstring configPath = GetConfigPath();
    if (std::filesystem::exists(configPath)) {
        return true;
    }

    AppConfigData defaults;
    return Save(defaults, out_error);
}

bool AppConfig::Load(AppConfigData* out_data, std::wstring* out_error) const {
    if (!out_data) {
        if (out_error) {
            *out_error = L"Output config buffer is null.";
        }
        return false;
    }

    if (!EnsureFileExists(out_error)) {
        return false;
    }

    const std::wstring configPath = GetConfigPath();
    out_data->mapPath = ReadIniString(kSectionUi, kKeyMapPath, configPath);
    out_data->ramLimitText = ReadIniString(kSectionUi, kKeyRamLimit, configPath);
    return true;
}

bool AppConfig::Save(const AppConfigData& data, std::wstring* out_error) const {
    const std::wstring configPath = GetConfigPath();
    const std::filesystem::path configDir = std::filesystem::path(configPath).parent_path();

    std::error_code ec;
    std::filesystem::create_directories(configDir, ec);
    if (ec) {
        if (out_error) {
            *out_error = L"Unable to create config directory: " + configDir.wstring();
        }
        return false;
    }

    if (!WriteIniString(kSectionUi, kKeyMapPath, data.mapPath.c_str(), configPath, out_error)) {
        return false;
    }
    if (!WriteIniString(kSectionUi, kKeyRamLimit, data.ramLimitText.c_str(), configPath, out_error)) {
        return false;
    }
    return true;
}
