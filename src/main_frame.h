#pragma once

#include <afxwin.h>

#include "ui/main_view.h"

class CMainFrame : public CFrameWnd {
public:
    CMainFrame() = default;

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);

    DECLARE_MESSAGE_MAP()

private:
    void LayoutChildren(int cx, int cy);

    CMainView m_mainView;
};
