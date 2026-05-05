#pragma once

#include <string>
#include <vector>

namespace fileutil {

bool ReadAllLines(const std::wstring& path, std::vector<std::string>* out_lines, std::wstring* out_error);

}
