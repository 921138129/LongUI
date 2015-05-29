
#include "LongUI.h"


// �Ƴ��ַ���
auto __fastcall UIEditaleText_RemoveText(
    LongUI::DynamicString& text, uint32_t pos, uint32_t len, bool readonly) noexcept {
    HRESULT hr = S_OK;
    if (readonly) {
        hr = S_FALSE;
        ::MessageBeep(MB_ICONERROR);
    }
    else {
        try { text.erase(pos, len); } CATCH_HRESULT(hr);
    }
    return hr;
}

// DWrite���ִ���ο�: 
// http://msdn.microsoft.com/zh-cn/library/windows/desktop/dd941792(v=vs.85).aspx



// ˢ�²���
auto LongUI::CUIEditaleText::refresh(bool update) noexcept ->UIWindow* {
    if (!m_bThisFocused) return nullptr;
    RectLTWH_F rect; this->GetCaretRect(rect);
    register auto* window = m_pHost->GetWindow();
    window->CreateCaret(rect.width, rect.height);
    window->SetCaretPos(m_pHost, rect.left, rect.top);
    if (update) {
        window->Invalidate(m_pHost);
    }
    return window;
}

// ���´�������
void LongUI::CUIEditaleText::recreate_layout() noexcept {
    ::SafeRelease(this->layout);
    // ��������
    m_pFactory->CreateTextLayout(
        m_text.c_str(), m_text.length(),
        m_pBasicFormat,
        m_size.width, m_size.height,
        &this->layout
        );
}

// �����ַ�(��)
auto LongUI::CUIEditaleText::insert(
    const wchar_t * str, uint32_t pos, uint32_t length) noexcept -> HRESULT {
    HRESULT hr = S_OK;
    // ֻ��
    if (UIEditaleText_IsReadOnly) {
        ::MessageBeep(MB_ICONERROR);
        return S_FALSE;
    }
    auto old_length = static_cast<uint32_t>(m_text.length());
    // �����ַ�
    try { m_text.insert(pos, str, length); }
    CATCH_HRESULT(hr);
    // �����ɲ���
    auto old_layout = ::SafeAcquire(this->layout);
    // ���´�������
    this->recreate_layout();
    // ���ı������?
    if (old_layout && UIEditaleText_IsRiched && SUCCEEDED(hr)) {
        // ����ȫ������
        CUIEditaleText::CopyGlobalProperties(old_layout, this->layout);
        // ����ÿ������, ��ȡ��Ӧ�õ��²���
        // ����λ?
        if (pos) {
            // ��һ��
            CUIEditaleText::CopyRangedProperties(old_layout, this->layout, 0, pos, 0);
            // �����
            CUIEditaleText::CopySinglePropertyRange(old_layout, pos - 1, this->layout, pos, length);
            // ������
            CUIEditaleText::CopyRangedProperties(old_layout, this->layout, pos, old_length, length);
        }
        else {
            // �����
            CUIEditaleText::CopySinglePropertyRange(old_layout, 0, this->layout, 0, length);
            // ������
            CUIEditaleText::CopyRangedProperties(old_layout, this->layout, 0, old_length, length);
        }
        // ĩβ
        CUIEditaleText::CopySinglePropertyRange(old_layout, old_length, this->layout, static_cast<uint32_t>(m_text.length()), UINT32_MAX);
    }
    ::SafeRelease(old_layout);
    return hr;
}


// ���ص�ǰѡ������
auto LongUI::CUIEditaleText::GetSelectionRange() const noexcept-> DWRITE_TEXT_RANGE {
    // ���ص�ǰѡ�񷵻�
    auto caretBegin = m_u32CaretAnchor;
    auto caretEnd = m_u32CaretPos + m_u32CaretPosOffset;
    // �෴�򽻻�
    if (caretBegin > caretEnd) {
        std::swap(caretBegin, caretEnd);
    }
    // ���Ʒ�Χ���ı�����֮��
    auto textLength = static_cast<uint32_t>(m_text.size());
    caretBegin = std::min(caretBegin, textLength);
    caretEnd = std::min(caretEnd, textLength);
    // ���ط�Χ
    return{ caretBegin, caretEnd - caretBegin };
}

// ����ѡ����
auto LongUI::CUIEditaleText::SetSelection(
    SelectionMode mode, uint32_t advance, bool exsel, bool update) noexcept-> HRESULT {
    //uint32_t line = uint32_t(-1);
    uint32_t absolute_position = m_u32CaretPos + m_u32CaretPosOffset;
    uint32_t oldabsolute_position = absolute_position;
    uint32_t old_caret_anchor = m_u32CaretAnchor;
    DWRITE_TEXT_METRICS textMetrics;
    // CASE
    switch (mode)
    {
    case SelectionMode::Mode_Left:
        m_u32CaretPos += m_u32CaretPosOffset;
        if (m_u32CaretPos > 0) {
            --m_u32CaretPos;
            this->AlignCaretToNearestCluster(false, true);
            absolute_position = m_u32CaretPos + m_u32CaretPosOffset;
            // ��黻�з�
            absolute_position = m_u32CaretPos + m_u32CaretPosOffset;
            if (absolute_position >= 1
                && absolute_position < m_text.size()
                && m_text[absolute_position - 1] == '\r'
                &&  m_text[absolute_position] == '\n')
            {
                m_u32CaretPos = absolute_position - 1;
                this->AlignCaretToNearestCluster(false, true);
            }
        }
        break;

    case SelectionMode::Mode_Right:
        m_u32CaretPos = absolute_position;
        this->AlignCaretToNearestCluster(true, true);
        absolute_position = m_u32CaretPos + m_u32CaretPosOffset;
        if (absolute_position >= 1
            && absolute_position < m_text.size()
            && m_text[absolute_position - 1] == '\r'
            &&  m_text[absolute_position] == '\n')
        {
            m_u32CaretPos = absolute_position + 1;
            this->AlignCaretToNearestCluster(false, true);
        }
        break;
    case SelectionMode::Mode_LeftChar:
        m_u32CaretPos = absolute_position;
        m_u32CaretPos -= std::min(advance, absolute_position);
        m_u32CaretPosOffset = 0;
        break;
    case SelectionMode::Mode_RightChar:
        m_u32CaretPos = absolute_position + advance;
        m_u32CaretPosOffset = 0;
        {
            // Use hit-testing to limit text position.
            DWRITE_HIT_TEST_METRICS hitTestMetrics;
            float caretX, caretY;
            this->layout->HitTestTextPosition(
                m_u32CaretPos,
                false,
                &caretX,
                &caretY,
                &hitTestMetrics
                );
            m_u32CaretPos = std::min(m_u32CaretPos, hitTestMetrics.textPosition + hitTestMetrics.length);
        }
        break;
    case SelectionMode::Mode_Up:
    case SelectionMode::Mode_Down:
    {
        SimpleSmallBuffer<DWRITE_LINE_METRICS, 10> metrice_buffer;
        // ��ȡ��ָ��
        this->layout->GetMetrics(&textMetrics);
        metrice_buffer.NewSize(textMetrics.lineCount);
        this->layout->GetLineMetrics(
            metrice_buffer.data,
            textMetrics.lineCount,
            &textMetrics.lineCount
            );
        // ��ȡ��
        uint32_t line, linePosition;
        CUIEditaleText::GetLineFromPosition(
            metrice_buffer.data,
            metrice_buffer.data_length,
            m_u32CaretPos,
            &line,
            &linePosition
            );
        // ���ƻ�����
        if (mode == SelectionMode::Mode_Up) {
            if (line <= 0) break;
            line--;
            linePosition -= metrice_buffer.data[line].length;
        }
        else {
            linePosition += metrice_buffer.data[line].length;
            line++;
            if (line >= metrice_buffer.data_length)  break;
        }
        DWRITE_HIT_TEST_METRICS hitTestMetrics;
        float caretX, caretY, dummyX;
        // ��ȡ��ǰ�ı�Xλ��
        this->layout->HitTestTextPosition(
            m_u32CaretPos,
            m_u32CaretPosOffset > 0, // trailing if nonzero, else leading edge
            &caretX,
            &caretY,
            &hitTestMetrics
            );
        // ��ȡ��λ��Y����
        this->layout->HitTestTextPosition(
            linePosition,
            false, // leading edge
            &dummyX,
            &caretY,
            &hitTestMetrics
            );
        // ��ȡ��x, y ���ı�λ��
        BOOL isInside, isTrailingHit;
        this->layout->HitTestPoint(
            caretX, caretY,
            &isTrailingHit,
            &isInside,
            &hitTestMetrics
            );
        m_u32CaretPos = hitTestMetrics.textPosition;
        m_u32CaretPosOffset = isTrailingHit ? (hitTestMetrics.length > 0) : 0;
    }
    break;
    case SelectionMode::Mode_LeftWord:
    case SelectionMode::Mode_RightWord:
    {
        // ���������ַ�����
        SimpleSmallBuffer<DWRITE_CLUSTER_METRICS, 64> metrice_buffer;
        UINT32 clusterCount;
        this->layout->GetClusterMetrics(nullptr, 0, &clusterCount);
        if (clusterCount == 0) break;
        // ���ô�С
        metrice_buffer.NewSize(clusterCount);
        this->layout->GetClusterMetrics(metrice_buffer.data, clusterCount, &clusterCount);
        m_u32CaretPos = absolute_position;
        UINT32 clusterPosition = 0;
        UINT32 oldCaretPosition = m_u32CaretPos;
        // ����
        if (mode == SelectionMode::Mode_LeftWord) {
            m_u32CaretPos = 0;
            m_u32CaretPosOffset = 0; // leading edge
            for (UINT32 cluster = 0; cluster < clusterCount; ++cluster) {
                clusterPosition += metrice_buffer.data[cluster].length;
                if (metrice_buffer.data[cluster].canWrapLineAfter) {
                    if (clusterPosition >= oldCaretPosition)
                        break;
                    // ˢ��.
                    m_u32CaretPos = clusterPosition;
                }
            }
        }
        else {
            // ֮��
            for (UINT32 cluster = 0; cluster < clusterCount; ++cluster) {
                UINT32 clusterLength = metrice_buffer.data[cluster].length;
                m_u32CaretPos = clusterPosition;
                m_u32CaretPosOffset = clusterLength; // trailing edge
                if (clusterPosition >= oldCaretPosition &&
                    metrice_buffer.data[cluster].canWrapLineAfter) {
                    break;
                }
                clusterPosition += clusterLength;
            }
        }
        int a = 0;
    }
    break;
    case SelectionMode::Mode_Home:
    case SelectionMode::Mode_End:
    {
        // ��ȡԤ֪����λ�û���ĩλ��
        SimpleSmallBuffer<DWRITE_LINE_METRICS, 10> metrice_buffer;
        // ��ȡ��ָ��
        this->layout->GetMetrics(&textMetrics);
        metrice_buffer.NewSize(textMetrics.lineCount);
        this->layout->GetLineMetrics(
            metrice_buffer.data,
            textMetrics.lineCount,
            &textMetrics.lineCount
            );
        uint32_t line;
        CUIEditaleText::GetLineFromPosition(
            metrice_buffer.data,
            metrice_buffer.data_length,
            m_u32CaretPos,
            &line,
            &m_u32CaretPos
            );
        m_u32CaretPosOffset = 0;
        if (mode == SelectionMode::Mode_End) {
            // ���ò������
            UINT32 lineLength = metrice_buffer.data[line].length -
                metrice_buffer.data[line].newlineLength;
            m_u32CaretPosOffset = std::min(lineLength, 1u);
            m_u32CaretPos += lineLength - m_u32CaretPosOffset;
            this->AlignCaretToNearestCluster(true);
        }
    }
    break;
    case SelectionMode::Mode_First:
        m_u32CaretPos = 0;
        m_u32CaretPosOffset = 0;
        break;

    case SelectionMode::Mode_SelectAll:
        m_u32CaretAnchor = 0;
        exsel = true;
        __fallthrough;
    case SelectionMode::Mode_Last:
        m_u32CaretPos = UINT32_MAX;
        m_u32CaretPosOffset = 0;
        this->AlignCaretToNearestCluster(true);
        break;
    case SelectionMode::Mode_Leading:
        m_u32CaretPos = advance;
        m_u32CaretPosOffset = 0;
        break;
    case SelectionMode::Mode_Trailing:
        m_u32CaretPos = advance;
        this->AlignCaretToNearestCluster(true);
        break;
    }
    absolute_position = m_u32CaretPos + m_u32CaretPosOffset;
    // ����ѡ��
    if (!exsel) {
        m_u32CaretAnchor = absolute_position;
    }
    // ����ƶ�
    bool caretMoved = (absolute_position != oldabsolute_position)
        || (m_u32CaretAnchor != old_caret_anchor);
    // �ƶ���?
    if (caretMoved) {
        // ���¸�ʽ
        if (update) {

        }
        // ˢ�²������
        this->refresh(true);
    }

    return caretMoved ? S_OK : S_FALSE;
    //return S_OK;
}

// ɾ��ѡ��������
auto LongUI::CUIEditaleText::DeleteSelection() noexcept-> HRESULT {
    HRESULT hr = S_FALSE;
    DWRITE_TEXT_RANGE selection = this->GetSelectionRange();
    if (selection.length == 0 || UIEditaleText_IsReadOnly) return hr;
    // ɾ��
    hr = UIEditaleText_RemoveText(
        m_text, selection.startPosition, selection.length,
        UIEditaleText_IsReadOnly
        );
    // �ɹ��Ļ�����ѡ����
    if (hr == S_OK) {
        hr = this->SetSelection(Mode_Leading, selection.startPosition, false);
    }
    return hr;
}


// ����ѡ����
bool LongUI::CUIEditaleText::SetSelectionFromPoint(float x, float y, bool exsel) noexcept {
    BOOL isTrailingHit;
    BOOL isInside;
    DWRITE_HIT_TEST_METRICS caret_metrics;
    // ��ȡ��ǰ���λ��
    this->layout->HitTestPoint(
        x, y,
        &isTrailingHit,
        &isInside,
        &caret_metrics
        );
    // ���µ�ǰѡ����
    this->SetSelection(
        isTrailingHit ? SelectionMode::Mode_Trailing : SelectionMode::Mode_Leading,
        caret_metrics.textPosition,
        exsel
        );
    return true;
}


// ����
bool LongUI::CUIEditaleText::OnDragEnter(IDataObject* data, DWORD* effect) noexcept {
    m_bDragFormatOK = false;
    m_bThisFocused = true;
    m_bDragFromThis = m_pDataObject == data;
    assert(data && effect && "bad argument");
    m_pHost->GetWindow()->ShowCaret();
    ::ReleaseStgMedium(&m_recentMedium);
    // ���֧�ָ�ʽ: Unicode-Text
    FORMATETC fmtetc = { CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    if (SUCCEEDED(data->GetData(&fmtetc, &m_recentMedium))) {
        m_bDragFormatOK = true;
    }
    return m_bDragFromThis ? false : m_bDragFormatOK;
}


// ����
bool LongUI::CUIEditaleText::OnDragOver(float x, float y) noexcept {
    // �Լ���?������ѡ��Χ��?
    if (m_bDragFromThis) {
        register auto range = m_dragRange;
        BOOL trailin, inside;
        DWRITE_HIT_TEST_METRICS caret_metrics;
        // ��ȡ��ǰ���λ��
        this->layout->HitTestPoint(x, y, &trailin, &inside, &caret_metrics);
        register bool inzone = caret_metrics.textPosition >= range.startPosition &&
            caret_metrics.textPosition < range.startPosition + range.length;
        if (inzone) return false;
    }
    // ѡ��λ��
    if (m_bDragFormatOK) {
        this->SetSelectionFromPoint(x, y, false);
        // ��ʾ�������
        this->refresh(false);
        return true;
    }
    return false;
}

// �ؽ�
void LongUI::CUIEditaleText::Recreate(ID2D1RenderTarget* target) noexcept {
    // ��Ч����
    assert(target); if (!target) return;
    // ���´�����Դ
    ::SafeRelease(m_pRenderTarget);
    ::SafeRelease(m_pSelectionColor);
    m_pRenderTarget = ::SafeAcquire(target);
    target->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::LightSkyBlue),
        &m_pSelectionColor
        );
}

// ����һ���ַ�ʱ
void LongUI::CUIEditaleText::OnChar(char32_t ch) noexcept {
    // �ַ���Ч
    if ((ch >= 0x20 || ch == 9)) {
        if (UIEditaleText_IsReadOnly) {
            ::MessageBeep(MB_ICONERROR);
            return;
        }
        // ɾ��ѡ�����ַ���
        this->DeleteSelection();
        //
        uint32_t length = 1;
        wchar_t chars[] = { static_cast<wchar_t>(ch), 0, 0 };
        // sizeof(wchar_t) == sizeof(char32_t) �����
        static_assert(sizeof(wchar_t) == sizeof(char16_t), "change it");
#if 0 // sizeof(wchar_t) == sizeof(char32_t)

#else
        // ����Ƿ���Ҫת��
        if (ch > 0xFFFF) {
            // From http://unicode.org/faq/utf_bom.html#35
            chars[0] = wchar_t(0xD800 + (ch >> 10) - (0x10000 >> 10));
            chars[1] = wchar_t(0xDC00 + (ch & 0x3FF));
            length++;
        }
#endif
        // ����
        this->insert(chars, m_u32CaretPos + m_u32CaretPosOffset, length);
        // ����ѡ����
        this->SetSelection(SelectionMode::Mode_Right, length, false, false);
        // ˢ��
        this->refresh(true);
    }
}


// ����ʱ
void LongUI::CUIEditaleText::OnKey(uint32_t keycode) noexcept {
    // ��鰴�� maybe IUIInput
    bool heldShift = (::GetKeyState(VK_SHIFT) & 0x80) != 0;
    bool heldControl = (::GetKeyState(VK_CONTROL) & 0x80) != 0;
    // ����λ��
    UINT32 absolutePosition = m_u32CaretPos + m_u32CaretPosOffset;

    switch (keycode)
    {
    case VK_RETURN:     // �س���
        // ���� - ��CRLF�ַ�
        if (UIEditaleText_IsMultiLine) {
            this->DeleteSelection();
            this->insert(L"\r\n", absolutePosition, 2);
            this->SetSelection(SelectionMode::Mode_Leading, absolutePosition + 2, false, false);
            // �޸�
            this->refresh();
        }
        // ���� - �򴰿ڷ������������Ϣ
        else {
            LongUI::EventArgument arg = { 0 };
            arg.event = LongUI::Event::Event_EditReturn;
            arg.sender = m_pHost;
            m_pHost->GetWindow()->DoEvent(arg);
            // TODO: single line
        }
        break;
    case VK_BACK:       // �˸��
                        // ��ѡ��Ļ�
        if (absolutePosition != m_u32CaretAnchor) {
            // ɾ��ѡ����
            this->DeleteSelection();
            // �ؽ�����
            this->recreate_layout();
        }
        else if (absolutePosition > 0) {
            UINT32 count = 1;
            // ����CR/LF �ر���
            if (absolutePosition >= 2 && absolutePosition <= m_text.size()) {
                auto* __restrict base = m_text.data() + absolutePosition;
                if (base[-2] == L'\r' && base[-1] == L'\n') {
                    count = 2;
                }
            }
            // ����
            this->SetSelection(SelectionMode::Mode_LeftChar, count, false);
            // �ַ���: ɾ��count���ַ�
            if (::UIEditaleText_RemoveText(m_text, m_u32CaretPos, count, UIEditaleText_IsReadOnly) == S_OK) {
                this->recreate_layout();
            }
        }
        // �޸�
        this->refresh();
        break;
    case VK_DELETE:     // ɾ����
                        // ��ѡ��Ļ�
        if (absolutePosition != m_u32CaretAnchor) {
            // ɾ��ѡ����
            this->DeleteSelection();
            // �ؽ�����
            this->recreate_layout();
        }
        // ɾ����һ�����ַ�
        else {
            DWRITE_HIT_TEST_METRICS hitTestMetrics;
            float caretX, caretY;
            // ��ȡ��Ⱥ��С
            this->layout->HitTestTextPosition(
                absolutePosition,
                false,
                &caretX,
                &caretY,
                &hitTestMetrics
                );
            // CR-LF?
            if (hitTestMetrics.textPosition + 2 < m_text.length()) {
                auto* __restrict base = m_text.data() + hitTestMetrics.textPosition;
                if (base[0] == L'\r' && base[1] == L'\n') {
                    ++hitTestMetrics.length;
                }
            }
            // �޸�
            this->SetSelection(SelectionMode::Mode_Leading, hitTestMetrics.textPosition, false);
            // ɾ���ַ�
            if (::UIEditaleText_RemoveText(m_text,
                hitTestMetrics.textPosition, hitTestMetrics.length,
                UIEditaleText_IsReadOnly
                ) == S_OK) {
                this->recreate_layout();
            }
        }
        // �޸�
        this->refresh();
        break;
    case VK_TAB:        // Tab��
        break;
    case VK_LEFT:       // �������һ���ַ�/��Ⱥ
        this->SetSelection(heldControl ? SelectionMode::Mode_LeftWord : SelectionMode::Mode_Left, 1, heldShift);
        break;
    case VK_RIGHT:      // �������һ���ַ�/��Ⱥ
        this->SetSelection(heldControl ? SelectionMode::Mode_RightWord : SelectionMode::Mode_Right, 1, heldShift);
        break;
    case VK_UP:         // ����ģʽ: ����һ��
        if (UIEditaleText_IsMultiLine)
            this->SetSelection(SelectionMode::Mode_Up, 1, heldShift);
        break;
    case VK_DOWN:       // ����ģʽ: ����һ��
        if (UIEditaleText_IsMultiLine)
            this->SetSelection(SelectionMode::Mode_Down, 1, heldShift);
        break;
    case VK_HOME:       // HOME��
        this->SetSelection(heldControl ? SelectionMode::Mode_First : SelectionMode::Mode_Home, 0, heldShift);
        break;
    case VK_END:        // END��
        this->SetSelection(heldControl ? SelectionMode::Mode_Last : SelectionMode::Mode_End, 0, heldShift);
        break;
    case 'C':           // 'C'�� Ctrl+C ����
        if (heldControl) this->CopyToClipboard();
        break;
    case VK_INSERT:     // Insert��
        if (heldControl)    this->CopyToClipboard();
        else if (heldShift) this->PasteFromClipboard();
        break;
    case 'V':           // 'V'�� Ctrl+V ճ��
        if (heldControl)   this->PasteFromClipboard();
        break;
    case 'X':           // 'X'�� Ctrl+X ����
        if (heldControl) {
            this->CopyToClipboard();
            this->DeleteSelection();
            this->recreate_layout();
            this->refresh();
        }
        break;
    case 'A':           // 'A'�� Ctrl+A ȫѡ
        if (heldControl)
            this->SetSelection(SelectionMode::Mode_SelectAll, 0, true);
        break;
    case 'Z':
        break;
    case 'Y':
        break;
    default:
        break;
    }
}

// �����ý���ʱ
void LongUI::CUIEditaleText::OnSetFocus() noexcept {
    m_bThisFocused = true;
    this->refresh()->ShowCaret();
}

// ��ʧȥ����ʱ
void LongUI::CUIEditaleText::OnKillFocus() noexcept {
    register auto* window = m_pHost->GetWindow();
    window->HideCaret();
    m_bThisFocused = false;
}

// �������ʱ
void LongUI::CUIEditaleText::OnLButtonUp(float x, float y) noexcept {
    // ���
    if (m_bClickInSelection && m_ptStart.x == x && m_ptStart.y == y) {
        // ѡ��
        this->SetSelectionFromPoint(x, y, false);
        // ��ʾ�������
        this->refresh(false);
    }
    m_pHost->GetWindow()->ReleaseCapture();

}

// �������ʱ
void LongUI::CUIEditaleText::OnLButtonDown(float x, float y, bool shfit_hold) noexcept {
    // ������겶��
    m_pHost->GetWindow()->SetCapture(m_pHost);
    // ˢ��
    auto range = this->GetSelectionRange();
    this->RefreshSelectionMetrics(range);
    // ��¼���λ��
    m_ptStart = { x, y };
    // ѡ������?
    if (m_metriceBuffer.data_length) {
        // ����
        BOOL trailin, inside;
        DWRITE_HIT_TEST_METRICS caret_metrics;
        // ��ȡ��ǰ���λ��
        this->layout->HitTestPoint(x, y, &trailin, &inside, &caret_metrics);
        m_bClickInSelection = caret_metrics.textPosition >= range.startPosition &&
            caret_metrics.textPosition < range.startPosition + range.length;
    }
    else {
        m_bClickInSelection = false;
    }
    // ����
    if (!m_bClickInSelection) {
        // ѡ��
        this->SetSelectionFromPoint(x, y, shfit_hold);
        // ��ʾ�������
        // CUIEditaleText_ShowTheCaret
    }
}

// �����סʱ
void LongUI::CUIEditaleText::OnLButtonHold(float x, float y, bool shfit_hold) noexcept {
    // �����ѡ����
    if (!shfit_hold && m_bClickInSelection) {
        // ��ʼ��ק
        if (m_ptStart.x != x || m_ptStart.y != y) {
            // ��鷶Χ
            m_dragRange = this->GetSelectionRange();
            // ȥ����겶��
            m_pHost->GetWindow()->ReleaseCapture();
            // ����
            m_pDataObject->SetUnicodeText(this->CopyToGlobal());
            // ��ʼ��ק
            DWORD effect = DROPEFFECT_COPY;
            if (!(UIEditaleText_IsReadOnly)) effect |= DROPEFFECT_MOVE; 
            const register HRESULT hr = ::SHDoDragDrop(
                m_pHost->GetWindow()->GetHwnd(),
                m_pDataObject, m_pDropSource, effect, &effect
                );
            // �Ϸųɹ� ��Ϊ�ƶ�
            if (hr == DRAGDROP_S_DROP && effect == DROPEFFECT_MOVE) {
                //�Լ��ģ�
                if (m_bDragFromThis) {
                    m_dragRange.startPosition += m_dragRange.length;
                }
                // ɾ��
                if (UIEditaleText_RemoveText(
                    m_text, m_dragRange.startPosition, m_dragRange.length,
                    UIEditaleText_IsReadOnly
                    ) == S_OK) {
                    this->recreate_layout();
                    this->SetSelection(Mode_Left, 1, false);
                    this->SetSelection(Mode_Right, 1, false);
                }

            }
            // �ع�?
            //m_pHost->GetWindow()->SetCapture(m_pHost);
        }
    }
    else {
        this->SetSelectionFromPoint(x, y, true);
    }
}

// ��������ַ���
void LongUI::CUIEditaleText::AlignCaretToNearestCluster(bool hit, bool skip) noexcept {
    DWRITE_HIT_TEST_METRICS hitTestMetrics;
    float caretX, caretY;
    // ��������ַ���
    this->layout->HitTestTextPosition(
        m_u32CaretPos,
        false,
        &caretX,
        &caretY,
        &hitTestMetrics
        );
    // ����0
    m_u32CaretPos = hitTestMetrics.textPosition;
    m_u32CaretPosOffset = (hit) ? hitTestMetrics.length : 0;
    // ���ڲ����ӵ�
    if (skip && hitTestMetrics.width == 0) {
        m_u32CaretPos += m_u32CaretPosOffset;
        m_u32CaretPosOffset = 0;
    }
}

// ��ȡ������ž���
void LongUI::CUIEditaleText::GetCaretRect(RectLTWH_F& rect) noexcept {
    // ��鲼��
    if (this->layout) {
        // ��ȡ f(�ı�ƫ��) -> ����
        DWRITE_HIT_TEST_METRICS caretMetrics;
        float caretX, caretY;
        this->layout->HitTestTextPosition(
            m_u32CaretPos,
            m_u32CaretPosOffset > 0,
            &caretX,
            &caretY,
            &caretMetrics
            );
        // ��ȡ��ǰѡ��Χ
        DWRITE_TEXT_RANGE selectionRange = this->GetSelectionRange();
        if (selectionRange.length > 0) {
            UINT32 actualHitTestCount = 1;
            this->layout->HitTestTextRange(
                m_u32CaretPos,
                0, // length
                0, // x
                0, // y
                &caretMetrics,
                1,
                &actualHitTestCount
                );
            caretY = caretMetrics.top;
        }
        // ��ȡ������ſ��
        DWORD caretIntThickness = 2;
        ::SystemParametersInfoW(SPI_GETCARETWIDTH, 0, &caretIntThickness, FALSE);
        register float caretThickness = static_cast<float>(caretIntThickness);
        // �������λ��
        // XXX: ���draw_zone
        rect.left = caretX - caretThickness * 0.5f + m_pHost->draw_zone.left;
        rect.width = caretThickness;
        rect.top = caretY + m_pHost->draw_zone.top;
        rect.height = caretMetrics.height;
    }
}

// ��Ⱦ
void LongUI::CUIEditaleText::Render(float x, float y) noexcept {
    assert(m_pRenderTarget);
    this->refresh(false);
    // ���ѡ����
    auto range = this->GetSelectionRange();
    // ��Ч
    if (range.length > 0) {
        this->RefreshSelectionMetrics(range);
        if (m_metriceBuffer.data_length) {
            m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
            // ����
            for (auto itr = m_metriceBuffer.data;
            itr != m_metriceBuffer.data + m_metriceBuffer.data_length; ++itr) {
                const DWRITE_HIT_TEST_METRICS& htm = *itr;
                D2D1_RECT_F highlightRect = {
                    htm.left + x,
                    htm.top + y,
                    htm.left + htm.width + x,
                    htm.top + htm.height + y
                };
                m_pRenderTarget->FillRectangle(highlightRect, m_pSelectionColor);
            }
            m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }
    }
    // �̻�����
    this->layout->Draw(m_buffer.data, m_pTextRenderer, x, y);
}

// ���Ƶ� Ŀ��ȫ�־��
auto LongUI::CUIEditaleText::CopyToGlobal() noexcept-> HGLOBAL {
    HGLOBAL global = nullptr;
    auto selection = this->GetSelectionRange();
    // ��Ч
    if (selection.length) {
        // ��ȡѡ���ַ�����
        size_t byteSize = sizeof(wchar_t) * (selection.length + 1);
        global = ::GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);
        // ��Ч?
        if (global) {
            auto* memory = reinterpret_cast<wchar_t*>(::GlobalLock(global));
            // ����ȫ���ڴ�ɹ�
            if (memory) {
                const wchar_t* text = m_text.c_str();
                ::memcpy(memory, text + selection.startPosition, byteSize);
                memory[selection.length] = L'\0';
                ::GlobalUnlock(global);
            }
        }
    }
    return global;
}

// ���Ƶ� ���а�
auto LongUI::CUIEditaleText::CopyToClipboard() noexcept-> HRESULT {
    HRESULT hr = E_FAIL;
    auto selection = this->GetSelectionRange();
    if (selection.length) {
        // �򿪼��а�
        if (::OpenClipboard(m_pHost->GetWindow()->GetHwnd())) {
            if (::EmptyClipboard()) {
                // ��ȡ��ת�ַ�����
                size_t byteSize = sizeof(wchar_t) * (selection.length + 1);
                HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);
                // ��Ч?
                if (hClipboardData) {
                    auto* memory = reinterpret_cast<wchar_t*>(::GlobalLock(hClipboardData));
                    // ����ȫ���ڴ�ɹ�
                    if (memory) {
                        const wchar_t* text = m_text.c_str();
                        ::memcpy(memory, text + selection.startPosition, byteSize);
                        memory[selection.length] = L'\0';
                        ::GlobalUnlock(hClipboardData);
                        // ��ȡ���ı�����
                        if (UIEditaleText_IsRiched) {
                            assert(!"Unsupported Now");
                            // TODO: ���ı�
                        }
                        if (::SetClipboardData(CF_UNICODETEXT, hClipboardData) != nullptr) {
                            hClipboardData = nullptr;
                            hr = S_OK;
                        }
                    }
                }
                if (hClipboardData) {
                    ::GlobalFree(hClipboardData);
                }
            }
            //  �رռ��а�
            ::CloseClipboard();
        }
    }
    return hr;
}



// �� Ŀ��ȫ�־�� ���
auto LongUI::CUIEditaleText::PasteFromGlobal(HGLOBAL global) noexcept-> HRESULT {
    // ��ʽ��ʼ
    size_t byteSize = ::GlobalSize(global);
    // ��ȡ����
    void* memory = ::GlobalLock(global);
    HRESULT hr = E_INVALIDARG;
    // ������Ч?
    if (memory) {
        // �滻ѡ����
        this->DeleteSelection();
        const wchar_t* text = reinterpret_cast<const wchar_t*>(memory);
        // ���㳤��
        auto characterCount = static_cast<UINT32>(::wcsnlen(text, byteSize / sizeof(wchar_t)));
        // ����
        hr = this->insert(text, m_u32CaretPos + m_u32CaretPosOffset, characterCount);
        ::GlobalUnlock(global);
        // �ƶ�
        this->SetSelection(SelectionMode::Mode_RightChar, characterCount, true);
    }
    return hr;
}

// �� ���а� ���
auto LongUI::CUIEditaleText::PasteFromClipboard() noexcept-> HRESULT {
    // ��ʽ��ʼ
    HRESULT hr = S_OK;  uint32_t characterCount = 0ui32;
    // �򿪼��а�
    if (::OpenClipboard(m_pHost->GetWindow()->GetHwnd())) {
        // ��ȡ���ı�����
        if (UIEditaleText_IsRiched) {
            assert(!"Unsupported Now");
            // TODO
        }
        // ��ȡUnicode����
        else {
            auto data = ::GetClipboardData(CF_UNICODETEXT);
            hr = this->PasteFromGlobal(data);
        }
        // �رռ��а�
        ::CloseClipboard();
    }
    return hr;
}

// ��ȡ�б��
void LongUI::CUIEditaleText::GetLineFromPosition(
    const DWRITE_LINE_METRICS * lineMetrics,
    uint32_t lineCount, uint32_t textPosition,
    OUT uint32_t * lineOut,
    OUT uint32_t * linePositionOut) noexcept {
    uint32_t line = 0;
    uint32_t linePosition = 0;
    uint32_t nextLinePosition = 0;
    for (; line < lineCount; ++line) {
        linePosition = nextLinePosition;
        nextLinePosition = linePosition + lineMetrics[line].length;
        if (nextLinePosition > textPosition) {
            // ����Ҫ.
            break;
        }
    }
    *linePositionOut = linePosition;
    *lineOut = std::min(line, lineCount - 1);
}

// ����ѡ���������������
void LongUI::CUIEditaleText::RefreshSelectionMetrics(DWRITE_TEXT_RANGE selection) noexcept {
    // ��ѡ��������
    if (selection.length == 0) {
        m_metriceBuffer.data_length = 0;
        return;
    };
    // ���ѡ�����������
    uint32_t actualHitTestCount = 0;
    this->layout->HitTestTextRange(
        selection.startPosition,
        selection.length,
        0.f, // x
        0.f, // y
        nullptr,
        0, // metrics count
        &actualHitTestCount
        );
    // ��֤������ȷ
    m_metriceBuffer.NewSize(actualHitTestCount);
    if (!actualHitTestCount) return;
    // ��ʽ��ȡ
    this->layout->HitTestTextRange(
        selection.startPosition,
        selection.length,
        0.f, // x
        0.f, // y
        m_metriceBuffer.data,
        m_metriceBuffer.data_length,
        &actualHitTestCount
        );
}



// CUIEditaleText ��������
LongUI::CUIEditaleText::~CUIEditaleText() noexcept {
    ::ReleaseStgMedium(&m_recentMedium);
    ::SafeRelease(this->layout);
    ::SafeRelease(m_pBasicFormat);
    ::SafeRelease(m_pTextRenderer);
    ::SafeRelease(m_pRenderTarget);
    ::SafeRelease(m_pSelectionColor);
    ::SafeRelease(m_pDropSource);
    ::SafeRelease(m_pDataObject);
}




#define UIEditaleText_NewAttribute(a) { ::strcpy(attribute_buffer, prefix); ::strcat(attribute_buffer, a); }

// ���ù��캯��
LongUI::CUIEditaleText::CUIEditaleText(UIControl* host, pugi::xml_node node,
    const char* prefix) noexcept : m_pHost(host) {
    m_dragRange = { 0, 0 };
    // ������
    assert(node && prefix && "bad arguments");
    ZeroMemory(&m_recentMedium, sizeof(m_recentMedium));
    m_pFactory = ::SafeAcquire(UIManager_DWriteFactory);
    register const char* str = nullptr;
    char attribute_buffer[256];
    // �������
    {
        uint32_t tmptype = Type_None;
        // ���ı�
        UIEditaleText_NewAttribute("rich");
        if (node.attribute(attribute_buffer).as_bool(false)) {
            tmptype |= Type_Riched;
        }
        // ������ʾ
        UIEditaleText_NewAttribute("multiline");
        if (node.attribute(attribute_buffer).as_bool(false)) {
            tmptype |= Type_MultiLine;
        }
        // ֻ��
        UIEditaleText_NewAttribute("readonly");
        if (node.attribute(attribute_buffer).as_bool(false)) {
            tmptype |= Type_ReadOnly;
        }
        // ֻ��
        UIEditaleText_NewAttribute("accelerator");
        if (node.attribute(attribute_buffer).as_bool(false)) {
            tmptype |= Type_Accelerator;
        }
        // ����
        UIEditaleText_NewAttribute("password");
        if (str = node.attribute(attribute_buffer).value()) {
            tmptype |= Type_Password;
            // TODO: UTF8 char(s) to UTF32 char;
            // this->password = 
        }
        this->type = static_cast<EditaleTextType>(tmptype);
    }
    // ��ȡ��Ⱦ��
    {
        int renderer_index = Type_NormalTextRenderer;
        UIEditaleText_NewAttribute("renderer");
        if ((str = node.attribute(attribute_buffer).value())) {
            renderer_index = LongUI::AtoI(str);
        }
        auto renderer = UIManager.GetTextRenderer(renderer_index);
        m_pTextRenderer = renderer;
        // ��֤������
        if (renderer) {
            auto length = renderer->GetContextSizeInByte();
            if (length) {
                UIEditaleText_NewAttribute("context");
                if ((str = node.attribute(attribute_buffer).value())) {
                    m_buffer.NewSize(length);
                    renderer->CreateContextFromString(m_buffer.data, str);
                }
            }
        }
    }
    // ��������ɫ
    {
        m_basicColor = D2D1::ColorF(D2D1::ColorF::Black);
        UIEditaleText_NewAttribute("color");
        UIControl::MakeColor(node.attribute(attribute_buffer).value(), m_basicColor);
    }
    // ����ʽ
    {
        uint32_t format_index = LongUIDefaultTextFormatIndex;
        UIEditaleText_NewAttribute("format");
        if ((str = node.attribute(attribute_buffer).value())) {
            format_index = static_cast<uint32_t>(LongUI::AtoI(str));
        }
        m_pBasicFormat = UIManager.GetTextFormat(format_index);
    }
    // ��ȡ�ı�
    {
        wchar_t buffer[LongUIStringBufferLength];
        if (str = node.attribute(prefix).value()) {
            try {
                m_text.assign(buffer, LongUI::UTF8toWideChar(str, buffer));
            }
            catch (...) {
                UIManager << DL_Error << L"FAILED" << LongUI::endl;
            }
        }
    }
    // ��������
    this->recreate_layout();
}

// ����ȫ������
void LongUI::CUIEditaleText::CopyGlobalProperties(
    IDWriteTextLayout* old_layout,
    IDWriteTextLayout* new_layout) noexcept {
    // ��������
    new_layout->SetTextAlignment(old_layout->GetTextAlignment());
    new_layout->SetParagraphAlignment(old_layout->GetParagraphAlignment());
    new_layout->SetWordWrapping(old_layout->GetWordWrapping());
    new_layout->SetReadingDirection(old_layout->GetReadingDirection());
    new_layout->SetFlowDirection(old_layout->GetFlowDirection());
    new_layout->SetIncrementalTabStop(old_layout->GetIncrementalTabStop());
    // ��������
#ifndef LONGUI_EDITCORE_COPYMAINPROP
    DWRITE_TRIMMING trimmingOptions = {};
    IDWriteInlineObject* inlineObject = nullptr;
    old_layout->GetTrimming(&trimmingOptions, &inlineObject);
    new_layout->SetTrimming(&trimmingOptions, inlineObject);
    SafeRelease(inlineObject);

    DWRITE_LINE_SPACING_METHOD lineSpacingMethod = DWRITE_LINE_SPACING_METHOD_DEFAULT;
    float lineSpacing = 0.f, baseline = 0.f;
    old_layout->GetLineSpacing(&lineSpacingMethod, &lineSpacing, &baseline);
    new_layout->SetLineSpacing(lineSpacingMethod, lineSpacing, baseline);
#endif
}

// �Է�Χ���Ƶ�������
void LongUI::CUIEditaleText::CopySinglePropertyRange(
    IDWriteTextLayout* old_layout, uint32_t old_start,
    IDWriteTextLayout* new_layout, uint32_t new_start, uint32_t length) noexcept {
    // ���㷶Χ
    DWRITE_TEXT_RANGE range = { new_start,  std::min(length, UINT32_MAX - new_start) };
    // ���弯
#ifndef LONGUI_EDITCORE_COPYMAINPROP
    IDWriteFontCollection* fontCollection = nullptr;
    old_layout->GetFontCollection(old_start, &fontCollection);
    new_layout->SetFontCollection(fontCollection, range);
    SafeRelease(fontCollection);
#endif
    {
        // ������
        wchar_t fontFamilyName[100];
        fontFamilyName[0] = L'\0';
        old_layout->GetFontFamilyName(old_start, &fontFamilyName[0], ARRAYSIZE(fontFamilyName));
        new_layout->SetFontFamilyName(fontFamilyName, range);
        // һ������
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
        old_layout->GetFontWeight(old_start, &weight);
        old_layout->GetFontStyle(old_start, &style);
        old_layout->GetFontStretch(old_start, &stretch);
        new_layout->SetFontWeight(weight, range);
        new_layout->SetFontStyle(style, range);
        new_layout->SetFontStretch(stretch, range);
        // �����С
        FLOAT fontSize = 12.0f;
        old_layout->GetFontSize(old_start, &fontSize);
        new_layout->SetFontSize(fontSize, range);
        // �»���. ɾ����
        BOOL value = FALSE;
        old_layout->GetUnderline(old_start, &value);
        new_layout->SetUnderline(value, range);
        old_layout->GetStrikethrough(old_start, &value);
        new_layout->SetStrikethrough(value, range);
#ifndef LONGUI_EDITCORE_COPYMAINPROP
        // ������
        wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
        localeName[0] = L'\0';
        old_layout->GetLocaleName(old_start, &localeName[0], ARRAYSIZE(localeName));
        new_layout->SetLocaleName(localeName, range);
#endif
    }
    // �̻�Ч��
    IUnknown* drawingEffect = nullptr;
    old_layout->GetDrawingEffect(old_start, &drawingEffect);
    new_layout->SetDrawingEffect(drawingEffect, range);
    SafeRelease(drawingEffect);
    // ��������
    IDWriteInlineObject* inlineObject = nullptr;
    old_layout->GetInlineObject(old_start, &inlineObject);
    new_layout->SetInlineObject(inlineObject, range);
    SafeRelease(inlineObject);
#ifndef LONGUI_EDITCORE_COPYMAINPROP
    // �Ű�
    IDWriteTypography* typography = nullptr;
    old_layout->GetTypography(old_start, &typography);
    new_layout->SetTypography(typography, range);
    SafeRelease(typography);
#endif
}

// ��Χ����AoE!
void LongUI::CUIEditaleText::CopyRangedProperties(
    IDWriteTextLayout* old_layout, IDWriteTextLayout* new_layout,
    uint32_t begin, uint32_t end, uint32_t new_offset, bool negative) noexcept {
    auto current = begin;
    // ����
    while (current < end) {
        // ���㷶Χ����
        DWRITE_TEXT_RANGE increment = { current, 1 };
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        old_layout->GetFontWeight(current, &weight, &increment);
        UINT32 rangeLength = increment.length - (current - increment.startPosition);
        // �����Ч��
        rangeLength = std::min(rangeLength, end - current);
        // ���Ƶ���
        CUIEditaleText::CopySinglePropertyRange(
            old_layout,
            current,
            new_layout,
            negative ? (current - new_offset) : (current + new_offset),
            rangeLength
            );
        // �ƽ�
        current += rangeLength;
    }
}



// -------------------------------------------------------------


// ����FALSE
HRESULT LongUI::UIBasicTextRenderer::IsPixelSnappingDisabled(void *, BOOL * isDisabled) noexcept {
    *isDisabled = false;
    return S_OK;
}

// ��Ŀ����Ⱦ��������ȡ
HRESULT LongUI::UIBasicTextRenderer::GetCurrentTransform(void *, DWRITE_MATRIX * mat) noexcept {
    assert(m_pRenderTarget);
    m_pRenderTarget->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(mat));
    return S_OK;
}

// ʼ����һ, ����ת��
HRESULT LongUI::UIBasicTextRenderer::GetPixelsPerDip(void *, FLOAT * bilibili) noexcept {
    *bilibili = 1.f;
    return S_OK;
}

// ��Ⱦ��������
HRESULT LongUI::UIBasicTextRenderer::DrawInlineObject(
    void * clientDrawingContext,
    FLOAT originX, FLOAT originY,
    IDWriteInlineObject * inlineObject,
    BOOL isSideways, BOOL isRightToLeft,
    IUnknown * clientDrawingEffect) noexcept {
    assert(inlineObject && "bad argument");
    // �������������LongUI��������
    // ��Ⱦ
    inlineObject->Draw(
        clientDrawingContext,
        this,
        originX, originY,
        false, false,
        clientDrawingEffect
        );
    return S_OK;
}

// ------------------UINormalTextRender-----------------------
// �̻�����
HRESULT LongUI::UINormalTextRender::DrawGlyphRun(
    void * clientDrawingContext,
    FLOAT baselineOriginX, FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    const DWRITE_GLYPH_RUN * glyphRun,
    const DWRITE_GLYPH_RUN_DESCRIPTION * glyphRunDescription,
    IUnknown * effect) noexcept {
    // ��ȡ��ɫ
    register D2D1_COLOR_F* color = nullptr;
    if (effect && LONGUISAMEVT(effect, &this->basic_color)) {
        color = &static_cast<UIColorEffect*>(effect)->color;
    }
    else {
        color = &this->basic_color.color;
    }
    // ������ɫ
    m_pBrush->SetColor(color);
    // ����D2D�ӿ�ֱ����Ⱦ����
    m_pRenderTarget->DrawGlyphRun(
        D2D1::Point2(baselineOriginX, baselineOriginY),
        glyphRun,
        m_pBrush,
        measuringMode
        );
    return S_OK;
}

// �̻��»���
HRESULT LongUI::UINormalTextRender::DrawUnderline(
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    const DWRITE_UNDERLINE* underline,
    IUnknown* effect
    ) noexcept {
    // ��ȡ��ɫ
    register D2D1_COLOR_F* color = nullptr;
    if (effect && LONGUISAMEVT(effect, &this->basic_color)) {
        color = &static_cast<UIColorEffect*>(effect)->color;
    }
    else {
        color = &this->basic_color.color;
    }
    // ������ɫ
    m_pBrush->SetColor(color);
    // �������
    D2D1_RECT_F rectangle = {
        baselineOriginX,
        baselineOriginY + underline->offset,
        baselineOriginX + underline->width,
        baselineOriginY + underline->offset + underline->thickness
    };
    // ������
    m_pRenderTarget->FillRectangle(&rectangle, m_pBrush);
    return S_OK;
}

// �̻�ɾ����
HRESULT LongUI::UINormalTextRender::DrawStrikethrough(
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    const DWRITE_STRIKETHROUGH* strikethrough,
    IUnknown* effect
    ) noexcept {
    // ��ȡ��ɫ
    register D2D1_COLOR_F* color = nullptr;
    if (effect && LONGUISAMEVT(effect, &this->basic_color)) {
        color = &static_cast<UIColorEffect*>(effect)->color;
    }
    else {
        color = &this->basic_color.color;
    }
    // ������ɫ
    m_pBrush->SetColor(color);
    // �������
    D2D1_RECT_F rectangle = {
        baselineOriginX,
        baselineOriginY + strikethrough->offset,
        baselineOriginX + strikethrough->width,
        baselineOriginY + strikethrough->offset + strikethrough->thickness
    };
    // ������
    m_pRenderTarget->FillRectangle(&rectangle, m_pBrush);
    return S_OK;
}