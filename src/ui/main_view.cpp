#include "ui/main_view.h"

#include <array>
#include <filesystem>
#include <vector>

#include "generated/build_version.h"
#include "resource.h"
#include "util/format_util.h"
#include "util/string_util.h"

namespace {

constexpr UINT WM_APP_DEFERRED_INIT = WM_APP + 100;

struct MetricRow {
    std::wstring field;
    std::wstring value;
    std::wstring source;
};

std::wstring MakeWide(const CString& text) {
    return std::wstring(text.GetString());
}

void SetListItem(CListCtrl& list, int row, int column, const std::wstring& text) {
    if (column == 0) {
        list.InsertItem(row, text.c_str());
        return;
    }
    list.SetItemText(row, column, text.c_str());
}

}  // namespace

BEGIN_MESSAGE_MAP(CMainView, CWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CMainView::OnBrowseClicked)
    ON_BN_CLICKED(IDC_BUTTON_PARSE, &CMainView::OnParseClicked)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECK_COMPILER_KEIL, IDC_CHECK_COMPILER_RH850, &CMainView::OnManualParserClicked)
    ON_MESSAGE(WM_APP_DEFERRED_INIT, &CMainView::OnDeferredInit)
END_MESSAGE_MAP()

CMainView::~CMainView() = default;

BOOL CMainView::Create(CWnd* parent) {
    CString className = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        nullptr);
    return CWnd::CreateEx(0, className, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                          CRect(0, 0, 0, 0), parent, 0);
}

int CMainView::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CWnd::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    CreateFonts();

    m_mapPathLabel.Create(L"Map File", WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this);
    m_mapPathEdit.CreateEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                           CRect(0, 0, 0, 0), this, IDC_EDIT_MAP_PATH);
    m_browseButton.Create(L"Browse", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(0, 0, 0, 0), this, IDC_BUTTON_BROWSE);

    m_buildVersionStatic.Create(L"", WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0, 0, 0, 0), this, IDC_STATIC_VERSION);
    m_buildDateStatic.Create(L"", WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0, 0, 0, 0), this, IDC_STATIC_BUILD_DATE);

    m_detectedFormatStatic.Create(L"Detected: N/A", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS,
                                  CRect(0, 0, 0, 0), this, IDC_STATIC_DETECTED_FORMAT);
    m_manualParserStatic.Create(L"Manual Parser Fallback", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                CRect(0, 0, 0, 0), this, IDC_STATIC_MANUAL_PARSER);

    m_keilCheckBox.Create(L"KEIL", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CHECK_COMPILER_KEIL);
    m_iarCheckBox.Create(L"IAR", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CHECK_COMPILER_IAR);
    m_gccCheckBox.Create(L"GCC", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CHECK_COMPILER_GCC);
    m_rl78CheckBox.Create(L"RL78", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CHECK_COMPILER_RL78);
    m_rh850CheckBox.Create(L"RH850", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CHECK_COMPILER_RH850);

    m_ramLimitLabel.Create(L"RAM Limit (optional hex)", WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this);
    m_ramLimitEdit.CreateEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                            CRect(0, 0, 0, 0), this, IDC_EDIT_RAM_LIMIT);

    m_logEdit.CreateEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL |
                                                         WS_VSCROLL | ES_READONLY,
                       CRect(0, 0, 0, 0), this, IDC_EDIT_LOG);
    m_reportEdit.CreateEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL |
                                                            WS_VSCROLL | ES_READONLY,
                          CRect(0, 0, 0, 0), this, IDC_EDIT_REPORT);

    m_metricsList.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
                         CRect(0, 0, 0, 0), this, IDC_LIST_METRICS);
    m_recommendationList.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
                                CRect(0, 0, 0, 0), this, IDC_LIST_RECOMMENDATIONS);

    m_parseButton.Create(L"Parse", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                         CRect(0, 0, 0, 0), this, IDC_BUTTON_PARSE);

    PostMessage(WM_APP_DEFERRED_INIT);

    return 0;
}

void CMainView::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    LayoutControls(cx, cy);
}

void CMainView::CreateFonts() {
    LOGFONTW ui = {};
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(ui), &ui, 0);
    wcscpy_s(ui.lfFaceName, L"Segoe UI");
    ui.lfHeight = -18;
    m_uiFont.CreateFontIndirectW(&ui);

    LOGFONTW bold = ui;
    bold.lfWeight = FW_BOLD;
    m_uiBoldFont.CreateFontIndirectW(&bold);

    LOGFONTW mono = ui;
    wcscpy_s(mono.lfFaceName, L"Consolas");
    mono.lfHeight = -17;
    m_monoFont.CreateFontIndirectW(&mono);
}

void CMainView::ApplyFonts() {
    CWnd* controls[] = {
        &m_mapPathLabel, &m_mapPathEdit, &m_browseButton, &m_buildVersionStatic, &m_buildDateStatic,
        &m_detectedFormatStatic, &m_manualParserStatic, &m_keilCheckBox, &m_iarCheckBox,
        &m_gccCheckBox, &m_rl78CheckBox, &m_rh850CheckBox, &m_ramLimitLabel, &m_ramLimitEdit, &m_parseButton
    };
    for (CWnd* control : controls) {
        if (control && control->GetSafeHwnd()) {
            control->SetFont(&m_uiFont);
        }
    }

    if (m_logEdit.GetSafeHwnd()) {
        m_logEdit.SetFont(&m_monoFont);
    }
    if (m_reportEdit.GetSafeHwnd()) {
        m_reportEdit.SetFont(&m_monoFont);
    }
    if (m_metricsList.GetSafeHwnd()) {
        m_metricsList.SetFont(&m_uiFont);
    }
    if (m_recommendationList.GetSafeHwnd()) {
        m_recommendationList.SetFont(&m_uiFont);
    }
}

int CMainView::MeasureTextWidth(const std::wstring& text, CFont& font) const {
    CClientDC dc(const_cast<CMainView*>(this));
    CFont* oldFont = dc.SelectObject(&font);
    const CSize size = dc.GetTextExtent(text.c_str(), static_cast<int>(text.size()));
    dc.SelectObject(oldFont);
    return size.cx;
}

void CMainView::RelayoutCurrentClientArea() {
    if (!GetSafeHwnd()) {
        return;
    }

    CRect rect;
    GetClientRect(&rect);
    LayoutControls(rect.Width(), rect.Height());
}

void CMainView::InitializeMetricsList() {
    m_metricsList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_metricsList.InsertColumn(0, L"Field", LVCFMT_LEFT, 220);
    m_metricsList.InsertColumn(1, L"Value", LVCFMT_LEFT, 220);
    m_metricsList.InsertColumn(2, L"Source / Formula", LVCFMT_LEFT, 580);
}

void CMainView::InitializeRecommendationList() {
    m_recommendationList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_recommendationList.InsertColumn(0, L"Item", LVCFMT_LEFT, 160);
    m_recommendationList.InsertColumn(1, L"reserved / estimated", LVCFMT_LEFT, 220);
    m_recommendationList.InsertColumn(2, L"+10%", LVCFMT_LEFT, 160);
    m_recommendationList.InsertColumn(3, L"+20%", LVCFMT_LEFT, 160);
}

void CMainView::LayoutControls(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    const int margin = 10;
    const int gap = 8;
    const int labelHeight = 22;
    const int editHeight = 28;
    const int topRightWidth = 270;
    const int buttonWidth = 96;
    const int metricsHeight = 238;
    const int bottomHeight = 156;
    const int selectionRowHeight = 28;

    int x = margin;
    int y = margin;
    const int rightPanelX = width - margin - topRightWidth;
    int pathEditWidth = rightPanelX - x - 90 - buttonWidth - (gap * 2);
    if (pathEditWidth < 320) {
        pathEditWidth = 320;
    }

    m_mapPathLabel.MoveWindow(x, y + 4, 70, labelHeight);
    m_mapPathEdit.MoveWindow(x + 72, y, pathEditWidth, editHeight);
    m_browseButton.MoveWindow(x + 72 + pathEditWidth + gap, y, buttonWidth, editHeight);

    m_buildVersionStatic.MoveWindow(rightPanelX, y, topRightWidth, labelHeight);
    m_buildDateStatic.MoveWindow(rightPanelX, y + labelHeight, topRightWidth, labelHeight);

    y += editHeight + gap;
    int detectedWidth = rightPanelX - margin - gap;
    if (detectedWidth < 320) {
        detectedWidth = 320;
    }
    m_detectedFormatStatic.MoveWindow(margin, y, detectedWidth, labelHeight);

    y += labelHeight + gap;
    const int manualLabelWidth = 160;
    const int checkboxGap = 10;
    const int checkboxPadding = 34;
    m_manualParserStatic.MoveWindow(margin, y + 4, manualLabelWidth, labelHeight);

    const std::array<CompilerFamily, 5> families = {
        CompilerFamily::Keil,
        CompilerFamily::Iar,
        CompilerFamily::Gcc,
        CompilerFamily::Rl78,
        CompilerFamily::Rh850,
    };
    const std::array<CButton*, 5> buttons = {
        &m_keilCheckBox,
        &m_iarCheckBox,
        &m_gccCheckBox,
        &m_rl78CheckBox,
        &m_rh850CheckBox,
    };

    int checkX = margin + manualLabelWidth + gap;
    for (size_t i = 0; i < families.size(); ++i) {
        const std::wstring normalText = CompilerFamilyToDisplayName(families[i]);
        const std::wstring autoText = normalText + L" [Auto]";
        const int normalWidth = MeasureTextWidth(normalText, m_uiFont);
        const int autoWidth = MeasureTextWidth(autoText, m_uiBoldFont);
        const int checkWidth = max(normalWidth, autoWidth) + checkboxPadding;
        buttons[i]->MoveWindow(checkX, y, checkWidth, selectionRowHeight);
        checkX += checkWidth + checkboxGap;
    }

    y += selectionRowHeight + gap;
    m_ramLimitLabel.MoveWindow(x, y + 4, 170, labelHeight);
    m_ramLimitEdit.MoveWindow(x + 172, y, 180, editHeight);
    m_parseButton.MoveWindow(x + 172 + 180 + gap, y, buttonWidth, editHeight);

    y += editHeight + gap;
    int contentHeight = height - y - metricsHeight - bottomHeight - (margin * 2);
    if (contentHeight < 180) {
        contentHeight = 180;
    }

    const int leftWidth = (width - margin * 2 - gap) / 3;
    const int rightWidth = width - margin * 2 - gap - leftWidth;

    m_logEdit.MoveWindow(margin, y, leftWidth, contentHeight);
    m_reportEdit.MoveWindow(margin + leftWidth + gap, y, rightWidth, contentHeight);

    y += contentHeight + gap;
    m_metricsList.MoveWindow(margin, y, width - margin * 2, metricsHeight);

    y += metricsHeight + gap;
    int recommendationWidth = width - margin * 2 - buttonWidth - gap;
    if (recommendationWidth < 420) {
        recommendationWidth = 420;
    }
    m_recommendationList.MoveWindow(margin, y, recommendationWidth, bottomHeight);

    m_metricsList.SetColumnWidth(0, 220);
    m_metricsList.SetColumnWidth(1, 220);
    m_metricsList.SetColumnWidth(2, width - margin * 2 - 460);

    const int recWidth = recommendationWidth;
    m_recommendationList.SetColumnWidth(0, 160);
    m_recommendationList.SetColumnWidth(1, 220);
    m_recommendationList.SetColumnWidth(2, (recWidth - 380) / 2);
    m_recommendationList.SetColumnWidth(3, (recWidth - 380) / 2);
}

void CMainView::RefreshBuildInfo() {
    CString versionText;
    versionText.Format(L"Build Version: %s", buildinfo::kBuildVersionText);
    m_buildVersionStatic.SetWindowTextW(versionText);

    CString dateText;
    dateText.Format(L"Build Date: %s", buildinfo::kBuildDateText);
    m_buildDateStatic.SetWindowTextW(dateText);
}

void CMainView::FinishInitialization() {
    ApplyFonts();
    InitializeMetricsList();
    InitializeRecommendationList();
    RefreshBuildInfo();
    LoadUiState();
    ShowPlaceholderForCurrentSelection();
}

void CMainView::LoadUiState() {
    AppConfigData config;
    std::wstring errorText;
    if (!m_appConfig.Load(&config, &errorText)) {
        ShowConfigError(errorText);
    } else {
        if (!config.mapPath.empty()) {
            m_mapPathEdit.SetWindowTextW(config.mapPath.c_str());
        } else {
            const std::filesystem::path defaultPath = std::filesystem::current_path() / "APROM_application.map";
            if (std::filesystem::exists(defaultPath)) {
                m_mapPathEdit.SetWindowTextW(defaultPath.wstring().c_str());
            }
        }
        m_ramLimitEdit.SetWindowTextW(config.ramLimitText.c_str());
    }
    SetSelectedFamily(CompilerFamily::Keil);
    RefreshDetectionForCurrentPath();
}

bool CMainView::SaveUiState(bool showError) {
    AppConfigData config;
    config.mapPath = stringutil::Trim(GetWindowTextValue(m_mapPathEdit));
    config.ramLimitText = stringutil::Trim(GetWindowTextValue(m_ramLimitEdit));

    std::wstring errorText;
    if (m_appConfig.Save(config, &errorText)) {
        return true;
    }

    if (showError) {
        ShowConfigError(errorText);
    }
    return false;
}

CompilerFamily CMainView::GetSelectedFamily() const {
    if (m_iarCheckBox.GetCheck() == BST_CHECKED) {
        return CompilerFamily::Iar;
    }
    if (m_gccCheckBox.GetCheck() == BST_CHECKED) {
        return CompilerFamily::Gcc;
    }
    if (m_rl78CheckBox.GetCheck() == BST_CHECKED) {
        return CompilerFamily::Rl78;
    }
    if (m_rh850CheckBox.GetCheck() == BST_CHECKED) {
        return CompilerFamily::Rh850;
    }
    return CompilerFamily::Keil;
}

void CMainView::SetSelectedFamily(CompilerFamily family) {
    m_keilCheckBox.SetCheck(family == CompilerFamily::Keil ? BST_CHECKED : BST_UNCHECKED);
    m_iarCheckBox.SetCheck(family == CompilerFamily::Iar ? BST_CHECKED : BST_UNCHECKED);
    m_gccCheckBox.SetCheck(family == CompilerFamily::Gcc ? BST_CHECKED : BST_UNCHECKED);
    m_rl78CheckBox.SetCheck(family == CompilerFamily::Rl78 ? BST_CHECKED : BST_UNCHECKED);
    m_rh850CheckBox.SetCheck(family == CompilerFamily::Rh850 ? BST_CHECKED : BST_UNCHECKED);
    UpdateManualParserHighlight();
}

void CMainView::RefreshDetectionForCurrentPath() {
    const std::wstring mapPath = stringutil::Trim(GetWindowTextValue(m_mapPathEdit));
    UpdateDetectionSummary(mapPath);
}

void CMainView::UpdateDetectionSummary(const std::wstring& mapPath) {
    if (mapPath.empty() || !std::filesystem::exists(mapPath)) {
        m_lastDetection = MapFormatDetectionResult{};
        m_detectedFormatStatic.SetWindowTextW(L"Detected: N/A");
        UpdateManualParserHighlight();
        return;
    }

    m_lastDetection = m_detector.DetectFromFile(mapPath);
    std::wstring summary = L"Detected: " + MapFormatToDisplayName(m_lastDetection.detectedFormat) +
                           L" (" + std::to_wstring(m_lastDetection.confidenceScore) + L"%)";
    if (m_lastDetection.confidenceScore >= 60) {
        const std::optional<CompilerFamily> detectedFamily = TryMapFormatToCompilerFamily(m_lastDetection.detectedFormat);
        if (detectedFamily.has_value()) {
            summary += L"  Auto: " + CompilerFamilyToDisplayName(*detectedFamily);
        }
    } else {
        summary += L"  Manual: " + CompilerFamilyToDisplayName(GetSelectedFamily());
    }
    m_detectedFormatStatic.SetWindowTextW(summary.c_str());
    UpdateManualParserHighlight();
}

void CMainView::UpdateManualParserHighlight() {
    struct CheckBoxMeta {
        CButton* button;
        CompilerFamily family;
    };

    const CheckBoxMeta checkBoxes[] = {
        {&m_keilCheckBox, CompilerFamily::Keil},
        {&m_iarCheckBox, CompilerFamily::Iar},
        {&m_gccCheckBox, CompilerFamily::Gcc},
        {&m_rl78CheckBox, CompilerFamily::Rl78},
        {&m_rh850CheckBox, CompilerFamily::Rh850},
    };

    for (const CheckBoxMeta& item : checkBoxes) {
        if (!item.button->GetSafeHwnd()) {
            continue;
        }
        item.button->SetWindowTextW(CompilerFamilyToDisplayName(item.family).c_str());
        item.button->SetFont(&m_uiFont);
    }

    const std::optional<CompilerFamily> detectedFamily =
        m_lastDetection.confidenceScore >= 60 ? TryMapFormatToCompilerFamily(m_lastDetection.detectedFormat) : std::nullopt;
    if (!detectedFamily.has_value() || *detectedFamily == GetSelectedFamily()) {
        return;
    }

    CButton* detectedCheckBox = nullptr;
    switch (*detectedFamily) {
    case CompilerFamily::Keil:
        detectedCheckBox = &m_keilCheckBox;
        break;
    case CompilerFamily::Iar:
        detectedCheckBox = &m_iarCheckBox;
        break;
    case CompilerFamily::Gcc:
        detectedCheckBox = &m_gccCheckBox;
        break;
    case CompilerFamily::Rl78:
        detectedCheckBox = &m_rl78CheckBox;
        break;
    case CompilerFamily::Rh850:
        detectedCheckBox = &m_rh850CheckBox;
        break;
    default:
        break;
    }
    if (!detectedCheckBox || !detectedCheckBox->GetSafeHwnd()) {
        return;
    }

    detectedCheckBox->SetWindowTextW((CompilerFamilyToDisplayName(*detectedFamily) + L" [Auto]").c_str());
    detectedCheckBox->SetFont(&m_uiBoldFont);
    RelayoutCurrentClientArea();
}

void CMainView::ShowPlaceholderForCurrentSelection() {
    RefreshDetectionForCurrentPath();

    const CompilerFamily family = GetSelectedFamily();
    std::vector<std::wstring> logLines;
    logLines.push_back(L"Ready.");
    logLines.push_back(L"Manual parser fallback: " + CompilerFamilyToDisplayName(family));
    logLines.push_back(L"Automatic map-format detection is enabled.");
    if (m_lastDetection.detectedFormat != MapFormat::Unknown) {
        logLines.push_back(L"Current detected format: " + MapFormatToDisplayName(m_lastDetection.detectedFormat));
        logLines.push_back(L"Detection confidence: " + std::to_wstring(m_lastDetection.confidenceScore) + L"%");
    }
    SetLogText(logLines);

    std::vector<std::wstring> reportLines;
    reportLines.push_back(L"Load a .map file and press Parse.");
    reportLines.push_back(L"");
    reportLines.push_back(L"This tool now detects map format before selecting a parser.");
    reportLines.push_back(L"Supported detection formats:");
    reportLines.push_back(L"  - KEIL_ARMCC_ARMCLANG");
    reportLines.push_back(L"  - GCC_GNU_LD");
    reportLines.push_back(L"  - IAR_ILINK");
    reportLines.push_back(L"  - RENESAS_CC_RL");
    reportLines.push_back(L"  - RENESAS_CC_RH");
    reportLines.push_back(L"  - GHS");
    reportLines.push_back(L"");
    reportLines.push_back(L"Manual parser fallback is used when confidence is low or detection is unknown.");
    reportLines.push_back(L"RAM Limit means total usable RAM capacity. If the map cannot provide it, enter the MCU RAM size manually.");
    reportLines.push_back(L"Recommendation output is aligned to 0x100 units.");
    SetReportText(stringutil::JoinLines(reportLines));

    m_metricsList.DeleteAllItems();
    m_recommendationList.DeleteAllItems();
}

void CMainView::ShowConfigError(const std::wstring& errorText) {
    if (errorText.empty()) {
        return;
    }

    std::wstring message = L"Config file update failed.\r\n\r\n" + errorText +
                           L"\r\n\r\nPath: " + m_appConfig.GetConfigPath();
    AfxMessageBox(message.c_str(), MB_ICONWARNING | MB_OK);
}

void CMainView::DisplayError(const std::wstring& errorText) {
    std::vector<std::wstring> logLines;
    logLines.push_back(L"Parse failed.");
    logLines.push_back(errorText);
    SetLogText(logLines);
    SetReportText(errorText);
    m_metricsList.DeleteAllItems();
    m_recommendationList.DeleteAllItems();
}

void CMainView::DisplayResult(const AnalysisResult& result) {
    SetLogText(result.logLines);
    SetReportText(result.narrative);
    PopulateMetricsList(result);
    PopulateRecommendationList(result);
}

void CMainView::SetLogText(const std::vector<std::wstring>& lines) {
    m_logEdit.SetWindowTextW(stringutil::JoinLines(lines).c_str());
}

void CMainView::SetReportText(const std::wstring& text) {
    m_reportEdit.SetWindowTextW(text.c_str());
}

void CMainView::PopulateMetricsList(const AnalysisResult& result) {
    m_metricsList.DeleteAllItems();

    const std::wstring ramLimitValue = result.effectiveRamLimitBytes.has_value()
        ? formatutil::BytesWithHex(*result.effectiveRamLimitBytes)
        : L"N/A";
    const std::wstring ramRemainingValue = result.effectiveRamLimitBytes.has_value()
        ? formatutil::BytesWithHex(result.report.ramRemainingBytes)
        : L"N/A";

    std::vector<MetricRow> rows = {
        {L"Code", formatutil::BytesWithHex(result.programSize.codeBytes), L"Program Size / compiler summary"},
        {L"RO-data", formatutil::BytesWithHex(result.programSize.roDataBytes), L"Program Size / compiler summary"},
        {L"RW-data", formatutil::BytesWithHex(result.programSize.rwDataBytes), L"Program Size / compiler summary"},
        {L"ZI-data", formatutil::BytesWithHex(result.programSize.ziDataBytes), L"Program Size / compiler summary"},
        {L"Total Flash Estimate", formatutil::BytesWithHex(result.report.flashUsedBytes), L"Code + RO-data + RW-data"},
        {L"Total RAM Static", formatutil::BytesWithHex(result.report.staticRamUsedBytes), L"RW-data + ZI-data"},
        {L"Stack Reserved", formatutil::BytesWithHex(result.stackHeapInfo.stackSizeBytes), result.stackHeapInfo.source.empty() ? L"Map stack section / symbol" : result.stackHeapInfo.source},
        {L"Heap Reserved", formatutil::BytesWithHex(result.stackHeapInfo.heapSizeBytes), result.stackHeapInfo.source.empty() ? L"Map heap section / symbol" : result.stackHeapInfo.source},
        {L"Estimated RAM Required", formatutil::BytesWithHex(result.report.estimatedRamRequiredBytes), L"RW-data + ZI-data + stack/heap recommendation, without double-counting already-included reserved sections"},
        {L"RAM Limit", ramLimitValue, result.effectiveRamLimitBytes.has_value() ? L"User input or parsed region capacity" : L"Missing"},
        {L"RAM Remaining", ramRemainingValue, L"RAM Limit - Estimated RAM Required"},
        {L"Risk Level", result.report.riskLevel, result.effectiveRamLimitBytes.has_value() ? formatutil::Percent(result.report.ramUsagePercent) : L"N/A"},
    };

    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        SetListItem(m_metricsList, i, 0, rows[i].field);
        SetListItem(m_metricsList, i, 1, rows[i].value);
        SetListItem(m_metricsList, i, 2, rows[i].source);
    }
}

void CMainView::PopulateRecommendationList(const AnalysisResult& result) {
    m_recommendationList.DeleteAllItems();

    auto addRow = [this](int rowIndex, const std::wstring& label, std::uint64_t baseValue) {
        const std::uint64_t unit = 0x100;
        const std::uint64_t recommend = formatutil::RoundUpToUnit(baseValue, unit);
        std::uint64_t up10 = formatutil::AddPercentRoundedUp(recommend, 10, unit);
        std::uint64_t up20 = formatutil::AddPercentRoundedUp(recommend, 20, unit);
        if (recommend > 0 && up10 <= recommend) {
            up10 = recommend + unit;
        }
        if (recommend > 0 && up20 <= up10) {
            up20 = up10 + unit;
        }
        SetListItem(m_recommendationList, rowIndex, 0, label);
        SetListItem(m_recommendationList, rowIndex, 1, formatutil::Hex(recommend));
        SetListItem(m_recommendationList, rowIndex, 2, formatutil::Hex(up10));
        SetListItem(m_recommendationList, rowIndex, 3, formatutil::Hex(up20));
    };

    addRow(0, L"Stack", result.stackHeapInfo.stackRecommendedBytes);
    addRow(1, L"Heap", result.stackHeapInfo.heapRecommendedBytes);
}

std::wstring CMainView::GetWindowTextValue(const CWnd& window) const {
    CString text;
    window.GetWindowTextW(text);
    return MakeWide(text);
}

void CMainView::OnBrowseClicked() {
    CFileDialog dialog(TRUE, L"map", nullptr,
                       OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                       L"Map Files (*.map)|*.map|All Files (*.*)|*.*||",
                       this);
    if (dialog.DoModal() == IDOK) {
        m_mapPathEdit.SetWindowTextW(dialog.GetPathName());
        RefreshDetectionForCurrentPath();
        ShowPlaceholderForCurrentSelection();
        SaveUiState(true);
    }
}

void CMainView::OnParseClicked() {
    const std::wstring mapPath = stringutil::Trim(GetWindowTextValue(m_mapPathEdit));
    if (mapPath.empty()) {
        DisplayError(L"Map path is empty.");
        return;
    }

    std::optional<std::uint64_t> ramLimitBytes;
    const std::wstring ramLimitText = stringutil::Trim(GetWindowTextValue(m_ramLimitEdit));
    if (!ramLimitText.empty()) {
        std::uint64_t value = 0;
        if (!stringutil::TryParseHexOrDecimal(ramLimitText, &value)) {
            DisplayError(L"RAM Limit must be a valid hexadecimal or decimal value.");
            return;
        }
        ramLimitBytes = value;
    }

    RefreshDetectionForCurrentPath();

    std::vector<std::wstring> progressLog;
    progressLog.push_back(L"Parsing started...");
    progressLog.push_back(L"Map file: " + mapPath);
    progressLog.push_back(L"Manual parser fallback: " + CompilerFamilyToDisplayName(GetSelectedFamily()));
    progressLog.push_back(L"Detected map format: " + MapFormatToDisplayName(m_lastDetection.detectedFormat));
    progressLog.push_back(L"Detection confidence: " + std::to_wstring(m_lastDetection.confidenceScore) + L"%");
    if (ramLimitBytes.has_value()) {
        progressLog.push_back(L"RAM limit override: " + formatutil::Hex(*ramLimitBytes));
    }
    SetLogText(progressLog);

    AnalysisResult result = m_engine.Run(AnalysisRequest{GetSelectedFamily(), mapPath, ramLimitBytes, true});
    if (!result.success) {
        if (!result.logLines.empty()) {
            SetLogText(result.logLines);
        }
        SetReportText(result.errorMessage);
        m_metricsList.DeleteAllItems();
        m_recommendationList.DeleteAllItems();
        SaveUiState(true);
        return;
    }
    DisplayResult(result);
    SaveUiState(true);
}

void CMainView::OnManualParserClicked(UINT controlId) {
    switch (controlId) {
    case IDC_CHECK_COMPILER_KEIL:
        SetSelectedFamily(CompilerFamily::Keil);
        break;
    case IDC_CHECK_COMPILER_IAR:
        SetSelectedFamily(CompilerFamily::Iar);
        break;
    case IDC_CHECK_COMPILER_GCC:
        SetSelectedFamily(CompilerFamily::Gcc);
        break;
    case IDC_CHECK_COMPILER_RL78:
        SetSelectedFamily(CompilerFamily::Rl78);
        break;
    case IDC_CHECK_COMPILER_RH850:
        SetSelectedFamily(CompilerFamily::Rh850);
        break;
    default:
        SetSelectedFamily(CompilerFamily::Keil);
        break;
    }

    RefreshDetectionForCurrentPath();
    ShowPlaceholderForCurrentSelection();
    SaveUiState(true);
}

LRESULT CMainView::OnDeferredInit(WPARAM, LPARAM) {
    FinishInitialization();
    return 0;
}

void CMainView::OnDestroy() {
    SaveUiState(false);
    CWnd::OnDestroy();
}
