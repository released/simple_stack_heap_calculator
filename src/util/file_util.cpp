#include "util/file_util.h"

#include <filesystem>
#include <fstream>

bool fileutil::ReadAllLines(const std::wstring& path, std::vector<std::string>* out_lines, std::wstring* out_error) {
    if (!out_lines) {
        if (out_error) {
            *out_error = L"Output buffer is null.";
        }
        return false;
    }

    std::ifstream stream(std::filesystem::path(path), std::ios::binary);
    if (!stream) {
        if (out_error) {
            *out_error = L"Unable to open file.";
        }
        return false;
    }

    out_lines->clear();
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        out_lines->push_back(line);
    }

    return true;
}
