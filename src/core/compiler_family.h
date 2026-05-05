#pragma once

#include <string>

enum class CompilerFamily {
    Keil = 0,
    Iar,
    Gcc,
    Rl78,
    Rh850,
    Ghs,
};

inline std::wstring CompilerFamilyToDisplayName(CompilerFamily family) {
    switch (family) {
    case CompilerFamily::Keil:
        return L"KEIL";
    case CompilerFamily::Iar:
        return L"IAR";
    case CompilerFamily::Gcc:
        return L"GCC";
    case CompilerFamily::Rl78:
        return L"RL78";
    case CompilerFamily::Rh850:
        return L"RH850";
    case CompilerFamily::Ghs:
        return L"GHS";
    default:
        return L"Unknown";
    }
}
