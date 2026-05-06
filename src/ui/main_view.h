#pragma once

#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxwin.h>

#include "config/app_config.h"
#include "core/analysis_engine.h"
#include "detector/map_format_detector.h"

class CMainView : public CWnd {
public:
    CMainView() = default;
    ~CMainView() override;

    BOOL Create(CWnd* parent);

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnBrowseClicked();
    afx_msg void OnParseClicked();
    afx_msg void OnManualParserClicked(UINT controlId);
    afx_msg void OnDestroy();
    afx_msg LRESULT OnDeferredInit(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    CompilerFamily GetSelectedFamily() const;
    void SetSelectedFamily(CompilerFamily family);
    void CreateFonts();
    void ApplyFonts();
    int MeasureTextWidth(const std::wstring& text, CFont& font) const;
    void RelayoutCurrentClientArea();
    void UpdateManualParserHighlight();
    void InitializeMetricsList();
    void InitializeRecommendationList();
    void LayoutControls(int width, int height);
    void RefreshBuildInfo();
    void LoadUiState();
    bool SaveUiState(bool showError);
    void RefreshDetectionForCurrentPath();
    void UpdateDetectionSummary(const std::wstring& mapPath);
    void ShowPlaceholderForCurrentSelection();
    void FinishInitialization();
    void ShowConfigError(const std::wstring& errorText);
    void DisplayError(const std::wstring& errorText);
    void DisplayResult(const AnalysisResult& result);
    void SetLogText(const std::vector<std::wstring>& lines);
    void SetReportText(const std::wstring& text);
    void PopulateMetricsList(const AnalysisResult& result);
    void PopulateRecommendationList(const AnalysisResult& result);
    std::wstring GetWindowTextValue(const CWnd& window) const;

    AnalysisEngine m_engine;
    MapFormatDetector m_detector;
    AppConfig m_appConfig;
    MapFormatDetectionResult m_lastDetection;

    CEdit m_mapPathEdit;
    CButton m_browseButton;
    CEdit m_ramLimitEdit;
    CButton m_keilCheckBox;
    CButton m_iarCheckBox;
    CButton m_gccCheckBox;
    CButton m_rl78CheckBox;
    CButton m_rh850CheckBox;
    CEdit m_logEdit;
    CEdit m_reportEdit;
    CListCtrl m_metricsList;
    CListCtrl m_recommendationList;
    CButton m_parseButton;
    CStatic m_buildVersionStatic;
    CStatic m_buildDateStatic;
    CStatic m_detectedFormatStatic;
    CStatic m_manualParserStatic;
    CStatic m_mapPathLabel;
    CStatic m_ramLimitLabel;

    CFont m_uiFont;
    CFont m_uiBoldFont;
    CFont m_monoFont;
};
