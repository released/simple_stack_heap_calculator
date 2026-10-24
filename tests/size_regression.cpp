#include "core/analysis_engine.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { throw std::runtime_error(message); }
}

void WriteFixture(const std::filesystem::path& path, const char* text) {
    Require(!std::filesystem::exists(path), "Fixture already exists");
    std::ofstream output(path, std::ios::binary);
    output << text;
    Require(output.good(), "Cannot write test-owned fixture");
}

void CheckTotals(const AnalysisResult& result) {
    Require(result.success, "Fixture analysis failed");
    Require(result.programSize.codeBytes == 1024, "Code changed");
    Require(result.programSize.roDataBytes == 64, "RO-data changed");
    Require(result.programSize.rwDataBytes == 32, "RW-data changed");
    Require(result.programSize.ziDataBytes == 128, "ZI-data changed");
    Require(result.report.flashUsedBytes == 1120, "Flash calculation changed");
    Require(result.report.staticRamUsedBytes == 160, "Static RAM changed");
    Require(!result.narrative.empty(), "Missing report");
}
}

int wmain(int argc, wchar_t** argv) {
    try {
        Require(argc == 2, "Pass a new test-owned fixture directory");
        const std::filesystem::path root(argv[1]);
        Require(std::filesystem::create_directory(root), "Fixture directory must be new");
        WriteFixture(root / L"gcc.map",
            "Memory Configuration\n"
            "Name Origin Length Attributes\n"
            "FLASH 0x00000000 0x00010000 xr\n"
            "RAM 0x20000000 0x00004000 xrw\n"
            "Linker script and memory map\n"
            ".text 0x00000000 0x00000400\n"
            ".rodata 0x00000400 0x00000040\n"
            ".data 0x20000000 0x00000020\n"
            ".bss 0x20000020 0x00000080\n");
        WriteFixture(root / L"keil.map",
            "ARM Linker\nMemory Map of the image\n"
            "Load Region LR_IROM1\nImage Symbol Table\n"
            "Program Size: Code=1024 RO-data=64 RW-data=32 ZI-data=128\n"
            "1024 0 64 32 128 0 Grand Totals\n");
        WriteFixture(root / L"invalid.map", "not a supported linker map\n");
        AnalysisEngine engine;
        AnalysisRequest request;
        request.mapPath = (root / L"gcc.map").wstring();
        request.manualFamily = CompilerFamily::Gcc;
        const auto gcc = engine.Run(request);
        CheckTotals(gcc);
        Require(gcc.effectiveRamLimitBytes == 16384, "GCC RAM limit changed");
        request.ramLimitOverrideBytes = 80;
        const auto smallRam = engine.Run(request);
        Require(smallRam.success && smallRam.report.ramRemainingBytes == 0 &&
            !smallRam.report.warnings.empty(), "RAM overflow warning changed");
        request.mapPath = (root / L"keil.map").wstring();
        request.manualFamily = CompilerFamily::Keil;
        request.ramLimitOverrideBytes.reset();
        CheckTotals(engine.Run(request));
        request.mapPath = (root / L"invalid.map").wstring();
        Require(!engine.Run(request).success, "Malformed input must fail");
        request.mapPath = (root / L"missing.map").wstring();
        Require(!engine.Run(request).success, "Missing input must fail");
        std::cout << "PASS: GCC/Keil totals, RAM overflow, malformed/missing input\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
