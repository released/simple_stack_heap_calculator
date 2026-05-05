#include "app.h"

#include "main_frame.h"

BEGIN_MESSAGE_MAP(CSimpleStackHeapCalculatorApp, CWinApp)
END_MESSAGE_MAP()

BOOL CSimpleStackHeapCalculatorApp::InitInstance() {
    CWinApp::InitInstance();

    auto* frame = new CMainFrame();
    if (!frame->Create(nullptr, L"Simple Stack Heap Calculator", WS_OVERLAPPEDWINDOW, CRect(0, 0, 1480, 960))) {
        delete frame;
        return FALSE;
    }

    m_pMainWnd = frame;
    frame->ShowWindow(SW_SHOWMAXIMIZED);
    frame->UpdateWindow();
    return TRUE;
}
