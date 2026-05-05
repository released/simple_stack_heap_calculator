#include "main_frame.h"

#include "resource.h"

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
    if (!CFrameWnd::PreCreateWindow(cs)) {
        return FALSE;
    }

    cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    HICON icon = AfxGetApp()->LoadIcon(IDI_APP_ICON);
    cs.lpszClass = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1),
        icon);
    return TRUE;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1) {
        return -1;
    }

    if (!m_mainView.Create(this)) {
        return -1;
    }

    HICON iconLarge = static_cast<HICON>(::LoadImageW(
        AfxGetInstanceHandle(),
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        32,
        32,
        LR_DEFAULTCOLOR));
    HICON iconSmall = static_cast<HICON>(::LoadImageW(
        AfxGetInstanceHandle(),
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        16,
        16,
        LR_DEFAULTCOLOR));
    if (iconLarge) {
        SetIcon(iconLarge, TRUE);
    }
    if (iconSmall) {
        SetIcon(iconSmall, FALSE);
    }

    return 0;
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) {
    CFrameWnd::OnSize(nType, cx, cy);
    LayoutChildren(cx, cy);
}

void CMainFrame::LayoutChildren(int cx, int cy) {
    if (!m_mainView.GetSafeHwnd() || cx <= 0 || cy <= 0) {
        return;
    }
    m_mainView.MoveWindow(0, 0, cx, cy);
}
