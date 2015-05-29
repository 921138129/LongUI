
#include "LongUI.h"

#if defined(_DEBUG)  && 1
#define TRACE_FUCTION UIManager << DL_Log << L"Trace:<%S> called" << LongUI::endl
#else
#define TRACE_FUCTION
#endif


// Render ��Ⱦ 
HRESULT LongUIMethodCall LongUI::UIRichEdit::Render() noexcept {
    HRESULT hr = S_OK;
    RECT draw_rect = { 0, 0, 100, 100 }; //AdjustRectT(LONG);
    if (m_pTextServices) {
        hr = m_pTextServices->TxDrawD2D(
            m_pRenderTarget,
            reinterpret_cast<RECTL*>(&draw_rect),
            nullptr,
            TXTVIEW_ACTIVE
            );
    }
    // �̻����
    if (SUCCEEDED(hr) && m_unused[Unused_ShowCaret]) {
        D2D1_RECT_F caretRect = {
            m_ptCaret.x, m_ptCaret.y,
            m_ptCaret.x + m_sizeCaret.width,m_ptCaret.y + m_sizeCaret.height
        };
        m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
        m_pRenderTarget->FillRectangle(caretRect, m_pFontBrush);
        m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }
    return S_OK;
}


// UIRichEdit ���캯��
LongUI::UIRichEdit::UIRichEdit(pugi::xml_node node) noexcept: Super(node){
    static_assert(UNUSED_SIZE <= lengthof(m_unused), "unused size!");
}

// UIRichEdit ��������
LongUI::UIRichEdit::~UIRichEdit() noexcept{
    ::SafeRelease(m_pFontBrush);
    if (m_pTextServices) {
        m_pTextServices->OnTxInPlaceDeactivate();
    }
    // �رշ���
    UIRichEdit::ShutdownTextServices(m_pTextServices);
    //::SafeRelease(m_pTextServices);
}

// UIRichEdit::CreateControl ����
LongUI::UIControl* LongUI::UIRichEdit::CreateControl(pugi::xml_node node) noexcept {
    if (!node) {
        UIManager << DL_Warning << L"node null" << LongUI::endl;
    }
    // ����ռ�
    auto pControl = LongUI::UIControl::AllocRealControl<LongUI::UIRichEdit>(
        node,
        [=](void* p) noexcept { new(p) UIRichEdit(node);}
    );
    if (!pControl) {
        UIManager << DL_Error << L"alloc null" << LongUI::endl;
    }
    return pControl;
}



// do event �¼�����
bool LongUIMethodCall LongUI::UIRichEdit::DoEvent(LongUI::EventArgument& arg) noexcept {
    if (arg.sender) {
        switch (arg.event)
        {
        case LongUI::Event::Event_FindControl: // ���ұ��ռ�
            if (IsPointInRect(this->show_zone, arg.pt)) {
                arg.ctrl = this;
            }
            return true;
        case LongUI::Event::Event_MouseEnter:
            m_pWindow->now_cursor = m_hCursorI;
            break;
        case LongUI::Event::Event_MouseLeave:
            m_pWindow->now_cursor = m_pWindow->default_cursor;
            break;
        case LongUI::Event::Event_SetFocus:
            if (m_pTextServices) {
                m_pTextServices->OnTxUIActivate();
                m_pTextServices->TxSendMessage(WM_SETFOCUS, 0, 0, 0);
            }
            return true;
        case LongUI::Event::Event_KillFocus:
            if (m_pTextServices) {
                m_pTextServices->OnTxUIDeactivate();
                m_pTextServices->TxSendMessage(WM_KILLFOCUS, 0, 0, 0);
                this->TxShowCaret(FALSE);
            }
            return true;
        }
    }
    // ����ϵͳ��Ϣ
    else if(m_pTextServices) {
        // ���
        if (m_pTextServices->TxSendMessage(arg.msg, arg.wParam_sys, arg.lParam_sys, &arg.lr) != S_FALSE) {
            // �Ѿ�������
            return true;
        }
    }
    return false;
}

// recreate �ؽ�
HRESULT LongUIMethodCall LongUI::UIRichEdit::Recreate(LongUIRenderTarget* newRT) noexcept {
    HRESULT hr = S_OK;
    if (m_pTextServices) {
        m_pTextServices->OnTxInPlaceDeactivate();
    }
    ::SafeRelease(m_pTextServices);
    ::SafeRelease(m_pFontBrush);
    // �����µı�ˢ
    m_pFontBrush = UIManager.GetBrush(LongUIDefaultTextFormatIndex);
    // ��ȡ���ھ��
    m_hwnd = m_pWindow->GetHwnd();
    IUnknown* pUk = nullptr;
    hr = UIRichEdit::CreateTextServices(nullptr, this, &pUk);
    // �����ı�����
    if (SUCCEEDED(hr)) {
        hr = pUk->QueryInterface(
            *UIRichEdit::IID_ITextServices2, reinterpret_cast<void**>(&m_pTextServices)
            );
    }
    if (SUCCEEDED(hr)) {
        hr = m_pTextServices->TxSetText(L"Hello, World!");
    }
    // �͵ؼ���ı��ؼ�
    if (SUCCEEDED(hr)) {
        hr = m_pTextServices->OnTxInPlaceActivate(nullptr);
    }
    ::SafeRelease(pUk);
    return Super::Recreate(newRT);
}

// close this control �رտؼ�
void LongUIMethodCall LongUI::UIRichEdit::Close() noexcept {
    delete this;
}

// ----- ITextHost
// ITextHost::TxGetDC ʵ��: ��ȡDC
HDC LongUI::UIRichEdit::TxGetDC() {
    // ��֧��!!
    assert(!"- GDI MODE - Not Supported");
    TRACE_FUCTION;
    return nullptr;
}

// ITextHost::TxReleaseDC ʵ��: �ͷ�DC
INT LongUI::UIRichEdit::TxReleaseDC(HDC hdc){
    assert(!"- GDI MODE - Not Supported");
    TRACE_FUCTION;
    return 0;
}

// ITextHost::TxShowScrollBar ʵ��: ��ʾ������
BOOL LongUI::UIRichEdit::TxShowScrollBar(INT fnBar, BOOL fShow){
    TRACE_FUCTION;
    return FALSE;
}

BOOL LongUI::UIRichEdit::TxEnableScrollBar(INT fuSBFlags, INT fuArrowflags){
    TRACE_FUCTION;
    return FALSE;
}

BOOL LongUI::UIRichEdit::TxSetScrollRange(INT fnBar, LONG nMinPos, INT nMaxPos, BOOL fRedraw){
    TRACE_FUCTION;
    return FALSE;
}

BOOL LongUI::UIRichEdit::TxSetScrollPos(INT fnBar, INT nPos, BOOL fRedraw){
    TRACE_FUCTION;
    return FALSE;
}

void LongUI::UIRichEdit::TxInvalidateRect(LPCRECT prc, BOOL fMode){
    TRACE_FUCTION;
}

void LongUI::UIRichEdit::TxViewChange(BOOL fUpdate){
    TRACE_FUCTION;
    //if (fUpdate) {
        m_pWindow->Invalidate(this);
    //}
}

// ITextHost::TxCreateCaret ʵ��:�������
BOOL LongUI::UIRichEdit::TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight){
    TRACE_FUCTION;
    m_sizeCaret = {static_cast<float>(xWidth), static_cast<float>(yHeight) };
    // ��������
    ::DestroyCaret();
    ::CreateCaret(m_hwnd, nullptr, xWidth, yHeight);
    return TRUE;
}

// ITextHost::TxShowCaret ʵ��:��ʾ/���ع��
BOOL LongUI::UIRichEdit::TxShowCaret(BOOL fShow){
    TRACE_FUCTION;
    m_unused[Unused_ShowCaret] = (fShow != 0);
    return TRUE;
}

BOOL LongUI::UIRichEdit::TxSetCaretPos(INT x, INT y){
    TRACE_FUCTION;
    m_ptCaret = {static_cast<float>(x), static_cast<float>(y) };
    m_pWindow->Invalidate(this);
    ::SetCaretPos(x, y);
    return TRUE;
}

BOOL LongUI::UIRichEdit::TxSetTimer(UINT idTimer, UINT uTimeout){
    ::SetTimer(m_hwnd, idTimer, uTimeout, nullptr);
    TRACE_FUCTION;
    return FALSE;
}

void LongUI::UIRichEdit::TxKillTimer(UINT idTimer){
    ::KillTimer(m_hwnd, idTimer);
    TRACE_FUCTION;
}

void LongUI::UIRichEdit::TxScrollWindowEx(INT dx, INT dy, LPCRECT lprcScroll,
    LPCRECT lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate, UINT fuScroll){
    TRACE_FUCTION;
}

void LongUI::UIRichEdit::TxSetCapture(BOOL fCapture) {
    TRACE_FUCTION;
    if (fCapture)
        ::SetCapture(m_hwnd);
    else
        ::ReleaseCapture();
}

void LongUI::UIRichEdit::TxSetFocus(){
    TRACE_FUCTION;
}

void LongUI::UIRichEdit::TxSetCursor(HCURSOR hcur, BOOL fText){
    TRACE_FUCTION;
}

BOOL LongUI::UIRichEdit::TxScreenToClient(LPPOINT lppt){
    TRACE_FUCTION;
    return FALSE;
}

BOOL LongUI::UIRichEdit::TxClientToScreen(LPPOINT lppt){
    TRACE_FUCTION;
    return FALSE;
}

HRESULT LongUI::UIRichEdit::TxActivate(LONG* plOldState){
    TRACE_FUCTION;
    return E_FAIL;
}

HRESULT LongUI::UIRichEdit::TxDeactivate(LONG lNewState){
    TRACE_FUCTION;
    return E_FAIL;
}

HRESULT LongUI::UIRichEdit::TxGetClientRect(LPRECT prc){
    //TRACE_FUCTION;
    *prc = { 0, 0, 100, 100 };
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxGetViewInset(LPRECT prc){
    // Set zero sized margins
    *prc = { 0, 0, 0, 0 };
    TRACE_FUCTION;
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxGetCharFormat(const CHARFORMATW **ppCF){
    /*METHOD_PROLOGUE(CRichDrawText, TextHost)

        // Return the default character format set up in the constructor
        *ppCF = &(pThis->m_CharFormat);*/
    TRACE_FUCTION;
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxGetParaFormat(const PARAFORMAT **ppPF){
   /* METHOD_PROLOGUE(CRichDrawText, TextHost)

        // Return the default paragraph format set up in the constructor
        *ppPF = &(pThis->m_ParaFormat);*/
    TRACE_FUCTION;
    return S_OK;
}

COLORREF LongUI::UIRichEdit::TxGetSysColor(int nIndex){
    // Pass requests for colours on to Windows
    TRACE_FUCTION;
    return ::GetSysColor(nIndex);
}

HRESULT LongUI::UIRichEdit::TxGetBackStyle(TXTBACKSTYLE *pstyle){
    // Do not erase what is underneath the drawing area
    *pstyle = TXTBACK_TRANSPARENT;
    TRACE_FUCTION;
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxGetMaxLength(DWORD *plength){
    // Set the maximum size of text to be arbitrarily large
    *plength = 1024 * 1024 * 16;
    TRACE_FUCTION;
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxGetScrollBars(DWORD *pdwScrollBar){
    *pdwScrollBar = 0;
    TRACE_FUCTION;
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxGetPasswordChar(TCHAR *pch){
    TRACE_FUCTION;
    return S_FALSE;
}

HRESULT LongUI::UIRichEdit::TxGetAcceleratorPos(LONG *pcp){
    *pcp = -1;
    TRACE_FUCTION;
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxGetExtent(LPSIZEL lpExtent){
    TRACE_FUCTION;
    return E_NOTIMPL;
}

HRESULT LongUI::UIRichEdit::OnTxCharFormatChange(const CHARFORMATW * pcf){
    TRACE_FUCTION;
    return E_FAIL;
}

HRESULT LongUI::UIRichEdit::OnTxParaFormatChange(const PARAFORMAT * ppf){
    TRACE_FUCTION;
    return E_FAIL;
}

HRESULT LongUI::UIRichEdit::TxGetPropertyBits(DWORD dwMask, DWORD *pdwBits){
    // Set the windowless control as being multiple lines of wrapping rich text
    DWORD bits = TXTBIT_MULTILINE | TXTBIT_RICHTEXT | TXTBIT_WORDWRAP | TXTBIT_D2DDWRITE;
    *pdwBits = bits & dwMask;
    TRACE_FUCTION;
    return S_OK;
}

HRESULT LongUI::UIRichEdit::TxNotify(DWORD iNotify, void *pv){
    // Claim to have handled the notifcation, even though we always ignore it
    TRACE_FUCTION;
    return S_OK;
}

HIMC LongUI::UIRichEdit::TxImmGetContext(){
    TRACE_FUCTION;
    return 0;
}

void LongUI::UIRichEdit::TxImmReleaseContext(HIMC himc){
    TRACE_FUCTION;
}

HRESULT LongUI::UIRichEdit::TxGetSelectionBarWidth(LONG *lSelBarWidth){
    // No selection bar
    *lSelBarWidth = 0;
    TRACE_FUCTION;
    return S_OK;
}

// ----- ITextHost2
/*#undef TRACE_FUCTION
#define TRACE_FUCTION (L"Trace:<%S> called\n", __FUNCTION__)*/

//@cmember Is a double click in the message queue?
BOOL    LongUI::UIRichEdit::TxIsDoubleClickPending() {
    TRACE_FUCTION;
    return FALSE;
}

//@cmember Get the overall window for this control     
HRESULT LongUI::UIRichEdit::TxGetWindow(HWND *phwnd) {
    *phwnd = m_hwnd;
    TRACE_FUCTION;
    return S_OK;
}

//@cmember Set controlwindow to foreground
HRESULT LongUI::UIRichEdit::TxSetForegroundWindow() {
    TRACE_FUCTION;
    return S_FALSE;
}

//@cmember Set control window to foreground
HPALETTE LongUI::UIRichEdit::TxGetPalette() {
    TRACE_FUCTION;
    return nullptr;
}

//@cmember Get East Asian flags
HRESULT LongUI::UIRichEdit::TxGetEastAsianFlags(LONG *pFlags) {
    *pFlags = ES_NOIME;
    TRACE_FUCTION;
    return S_FALSE;
}

//@cmember Routes the cursor change to the winhost
HCURSOR LongUI::UIRichEdit::TxSetCursor2(HCURSOR hcur, BOOL bText) {
    TRACE_FUCTION;
    return hcur;
}

//@cmember Notification that text services is freed
void    LongUI::UIRichEdit::TxFreeTextServicesNotification() {

    TRACE_FUCTION;
}

//@cmember Get Edit Style flags
HRESULT LongUI::UIRichEdit::TxGetEditStyle(DWORD dwItem, DWORD *pdwData) {
    TRACE_FUCTION;
    return FALSE;
}

//@cmember Get Window Style bits
HRESULT LongUI::UIRichEdit::TxGetWindowStyles(DWORD *pdwStyle, DWORD *pdwExStyle) {
    *pdwStyle = WS_OVERLAPPEDWINDOW;
    *pdwExStyle = 0;
    TRACE_FUCTION;
    return S_OK;
}

//@cmember Show / hide drop caret (D2D-only)
HRESULT LongUI::UIRichEdit::TxShowDropCaret(BOOL fShow, HDC hdc, LPCRECT prc) {
    TRACE_FUCTION;
    return S_FALSE;
}

//@cmember Destroy caret (D2D-only)
HRESULT LongUI::UIRichEdit::TxDestroyCaret() {
    TRACE_FUCTION;
    return S_FALSE;
}

//@cmember Get Horizontal scroll extent
HRESULT LongUI::UIRichEdit::TxGetHorzExtent(LONG *plHorzExtent) {
    TRACE_FUCTION;
    return S_FALSE;
}

/*
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetScrollBars> called
Trace:<TxGetScrollBars> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetScrollBars> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxShowCaret> called
Trace:<TxCreateCaret> called
Trace:<TxSetCaretPos> called
Trace:<TxShowCaret> called
Trace:<TxInvalidateRect> called
Trace:<TxInvalidateRect> called
Trace:<TxViewChange> called
Trace:<TxShowCaret> called
Trace:<TxGetScrollBars> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxShowCaret> called
Trace:<TxCreateCaret> called
Trace:<TxSetCaretPos> called
Trace:<TxShowCaret> called
Trace:<TxGetWindow> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxGetViewInset> called
Trace:<TxGetSelectionBarWidth> called
Trace:<TxGetWindowStyles> called
Trace:<TxNotify> called
Trace:<TxGetSysColor> called
Trace:<TxGetSysColor> called
Trace:<TxDestroyCaret> called
Trace:<TxNotify> called
Trace:<TxShowCaret> called
*/