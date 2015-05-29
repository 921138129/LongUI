
#include "LongUI.h"


// λͼ�滮:
/*
-----
|1|2|
-----
|3|4|
-----
1-2     �������
3       �����
*/

#define MakeAsUnit(a) (((a) + (LongUITargetBitmapUnitSize-1)) / LongUITargetBitmapUnitSize * LongUITargetBitmapUnitSize)

// UIWindow ���캯��
LongUI::UIWindow::UIWindow(pugi::xml_node node,
    UIWindow* parent) noexcept : Super(node) {
    assert(node && "<LongUI::UIWindow::UIWindow> window_node null");
    uint32_t flag = this->flags;
    // ���DComposition���
    if (node.attribute("dcomp").as_bool(true)) {
        flag |= Flag_Window_DComposition;
    }
    // Ŀǰ��֧�ַ�Flag_Window_DComposition
    // ���ܻᷢ��δ֪����
    assert(flag & Flag_Window_DComposition && "NOT IMPL");
    // ���FullRendering���
    if (node.attribute("fullrender").as_bool(false)) {
        flag |= Flag_Window_FullRendering;
    }
    else {
        this->reset_renderqueue();
    }
    // ���RenderedOnParentWindow���
    if (node.attribute("renderonparent").as_bool(false)) {
        flag |= Flag_Window_RenderedOnParentWindow;
    }
    // ���alwaysrendering
    if (node.attribute("alwaysrendering").as_bool(false)) {
        flag |= Flag_Window_AlwaysRendering;
    }
    force_cast(this->flags) = static_cast<LongUIFlag>(flag);
    // Ĭ����ʽ
    DWORD window_style = WS_OVERLAPPEDWINDOW;
    const char* str_get = nullptr;
    // ���ô��ڴ�С
    RECT window_rect = { 0, 0, LongUIDefaultWindowWidth, LongUIDefaultWindowHeight };
    if (str_get = node.attribute("size").value()) {
        UIControl::MakeFloats(str_get, &draw_zone.width, 2);
        window_rect.right = static_cast<decltype(window_rect.right)>(draw_zone.width);
        window_rect.bottom = static_cast<decltype(window_rect.bottom)>(draw_zone.height);
    }
    else {
        this->draw_zone.width = static_cast<float>(LongUIDefaultWindowWidth);
        this->draw_zone.height = static_cast<float>(LongUIDefaultWindowHeight);
    }
    m_clientSize.width = this->draw_zone.width;
    m_clientSize.height = this->draw_zone.height;
    // ������С
    ::AdjustWindowRect(&window_rect, window_style, FALSE);
    // ����
    window_rect.right -= window_rect.left;
    window_rect.bottom -= window_rect.top;
    window_rect.left = (::GetSystemMetrics(SM_CXFULLSCREEN) - window_rect.right) / 2;
    window_rect.top = (::GetSystemMetrics(SM_CYFULLSCREEN) - window_rect.bottom) / 2;
    // ��������
    m_hwnd = ::CreateWindowExW(
        //WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        (this->flags & Flag_Window_DComposition) ? WS_EX_NOREDIRECTIONBITMAP : 0,
        L"LongUIWindow", L"Nameless",
        WS_OVERLAPPEDWINDOW,
        window_rect.left, window_rect.top, window_rect.right, window_rect.bottom,
        parent ? parent->GetHwnd() : nullptr, nullptr, 
        ::GetModuleHandleW(nullptr), 
        this
        );
    //SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    // ����Hover
    m_csTME.cbSize = sizeof(m_csTME);
    m_csTME.dwFlags = TME_HOVER | TME_LEAVE;
    m_csTME.hwndTrack = m_hwnd;
    m_csTME.dwHoverTime = LongUIDefaultHoverTime;
    // ������˸��ʱ��
    m_idBlinkTimer = ::SetTimer(m_hwnd, 0, ::GetCaretBlinkTime(), nullptr);
    // ��Ӵ���
    UIManager.AddWindow(this);
    this->clear_color.a = 0.95f;
    // ����������
    ::CoCreateInstance(
        CLSID_DragDropHelper, 
        nullptr, 
        CLSCTX_INPROC_SERVER,
        LongUI_IID_PV_ARGS(m_pDropTargetHelper)
        );
    // ע����קĿ��
    ::RegisterDragDrop(m_hwnd, this);
    // ��ʾ����
    ::ShowWindow(m_hwnd, SW_SHOW);
    // Set Timer
    m_timer.Start();
    // ���ڴ��ھ����Լ�
    m_pWindow = this;
    // ���ֲ���
    ZeroMemory(&m_rcScroll, sizeof(m_dirtyRects));
    ZeroMemory(m_dirtyRects, sizeof(m_dirtyRects));
    m_present = { 0, m_dirtyRects, nullptr, nullptr };
}

// UIWindow ��������
LongUI::UIWindow::~UIWindow() noexcept {
    // ���
    if (m_pRenderQueue) {
        LongUICtrlFree(m_pRenderQueue);
        m_pRenderQueue = nullptr;
    }
    // ȡ��ע��
    ::RevokeDragDrop(m_hwnd);
    // �ݻٴ���
    ::DestroyWindow(m_hwnd);
    // �Ƴ�����
    UIManager.RemoveWindow(this);
    // ɱ��!
    ::KillTimer(m_hwnd, m_idBlinkTimer);
    // �ͷ���Դ
    this->release_data();
    // �ͷ�����
    ::SafeRelease(m_pDropTargetHelper);
    ::SafeRelease(m_pCurDataObject);
}


// ע��
void LongUI::UIWindow::RegisterPreRender(UIControl* c, bool is3d) noexcept {
    // ���
#ifdef _DEBUG
    auto itr = std::find(m_vRegisteredControl.begin(), m_vRegisteredControl.end(), c);
    if (itr != m_vRegisteredControl.end()) {
        UIManager << DL_Warning << L"control: [" << c->GetNameStr() << L"] existed" << LongUI::endl;
        return;
    }
#endif
    try {
        if (is3d) {
            m_vRegisteredControl.insert(m_vRegisteredControl.begin(), c);
        }
        else {
            m_vRegisteredControl.push_back(c);
        }
    }
    catch (...) {
        UIManager << DL_Warning << L"insert failed" << LongUI::endl;
    }
}

// ��ע��
void LongUI::UIWindow::UnRegisterPreRender(UIControl* c) noexcept {
    auto itr = std::find(m_vRegisteredControl.begin(), m_vRegisteredControl.end(), c);
    if (itr != m_vRegisteredControl.end()) {
        m_vRegisteredControl.erase(itr);
    }
#ifdef _DEBUG
    else {
        UIManager << DL_Warning << L"control: [" << c->GetNameStr() << L"] not found" << LongUI::endl;
    }
#endif
}


// ���ò������
void LongUI::UIWindow::SetCaretPos(UIControl* c, float x, float y) noexcept {
    if (!m_cShowCaret) return;
    // ת��Ϊ��������
    auto pt = D2D1::Point2F(x, y);
    if (c && c->parent) {
        pt = LongUI::TransformPoint(c->parent->world, pt);
    }
    m_bCaretIn = true;
    const register int intx = static_cast<int>(pt.x);
    const register int inty = static_cast<int>(pt.y);
    const register int oldx = static_cast<int>(m_rcCaretPx.left);
    const register int oldy = static_cast<int>(m_rcCaretPx.top);
    if (oldx != intx || oldy != inty) {
        this->refresh_caret();
        m_rcCaretPx.left = intx; m_rcCaretPx.top = inty;
        ::SetCaretPos(intx, inty);
#if 0
        if (!m_pd2dBitmapAE) return;
        m_pTargetBimtap->CopyFromBitmap(nullptr, m_pd2dBitmapAE, nullptr);
        this->draw_caret();
        /*const register int intw = static_cast<int>(m_rcCaret.width) + 1;
        const register int inth = static_cast<int>(m_rcCaret.height) + 1;
        RECT rects[] = {
            { oldx, oldy, oldx + intw,oldy + inth },
            { intx, inty, intx + intw,inty + inth },
        };*/
        /*::(L"rects: {%d, %d, %d, %d} {%d, %d, %d, %d}\n",
            rects[0].left, rects[0].top, rects[0].right, rects[0].bottom,
            rects[1].left, rects[1].top, rects[1].right, rects[1].bottom
            );*/
        DXGI_PRESENT_PARAMETERS para = { 0, nullptr, nullptr, nullptr };
        // �����ˢ��
        m_pSwapChain->Present1(0, 0, &para);
#endif
    }
}

// �����������
void LongUI::UIWindow::CreateCaret(float width, float height) noexcept {
    this->refresh_caret();
    // TODO: ת��Ϊ���ص�λ
    m_rcCaretPx.width = static_cast<decltype(m_rcCaretPx.height)>(width);
    m_rcCaretPx.height = static_cast<decltype(m_rcCaretPx.width)>(height);
    if (!m_rcCaretPx.width) m_rcCaretPx.width = 1;
    if (!m_rcCaretPx.height) m_rcCaretPx.height = 1;
}

// ��ʾ�������
void LongUI::UIWindow::ShowCaret() noexcept {
    ++m_cShowCaret;
    // ����AEλͼ
    //if (!m_pd2dBitmapAE) {
        //this->recreate_ae_bitmap();
    //}
}

// �쳣�������
void LongUI::UIWindow::HideCaret() noexcept { 
    if (m_cShowCaret) {
        --m_cShowCaret;
    }
#ifdef _DEBUG
    else {
        UIManager << DL_Warning << L"m_cShowCaret alread to 0" << LongUI::endl;
    }
#endif
    if (!m_cShowCaret) m_bCaretIn = false;
}

// release data
void LongUIMethodCall LongUI::UIWindow::release_data() noexcept {
    // �ͷ���Դ
    ::SafeRelease(m_pTargetBimtap);
    ::SafeRelease(m_pSwapChain);
    ::SafeRelease(m_pDcompDevice);
    ::SafeRelease(m_pDcompTarget);
    ::SafeRelease(m_pDcompVisual);
    ::SafeRelease(m_pBitmapPlanning);
}


namespace LongUI {
    // ��Ⱦ��λ
    struct RenderingUnit {
        // length of this unit
        size_t      length;
        // main data of unit
        UIControl*  units[LongUIDirtyControlSize];
    };
    // ʵ�� RenderingQueue
    class UIWindow::RenderingQueue {
    public:
        // ���캯��
        RenderingQueue(uint32_t i) noexcept : display_frequency(i) { }
        // ����
        void Update() noexcept {
            this->current_time = ::timeGetTime();
            // ���
            auto dur = this->current_time - this->start_time;
            if (dur >= 1000ui32 * LongUIPlanRenderingTotalTime) {
                this->start_time += 1000ui32 * LongUIPlanRenderingTotalTime;
                dur %= 1000ui32 * LongUIPlanRenderingTotalTime;
            }
            // ��λ
            auto offset = dur * display_frequency / 1000;
            assert(offset >= 0 && offset < display_frequency * LongUIPlanRenderingTotalTime);
            current_unit = this->units + offset;
        }
        // ��ȡ
        auto Get(float time_in_the_feature) {
            assert(time_in_the_feature < float(LongUIPlanRenderingTotalTime));
            // ��ȡĿ��ֵ
            auto offset = uint32_t(time_in_the_feature * float(display_frequency));
            auto got = this->current_unit + offset;
            // Խ����
            if (got >= units + queue_length) {
                got -= queue_length;
            }
            return got;
        }
    public:
        // ���ε���Ļˢ����
        uint32_t        display_frequency = 0;
        // �����г���
        uint32_t        queue_length = 0;
        // ���п�ʼʱ��
        uint32_t        start_time = ::timeGetTime();
        // ���е�ǰʱ��
        uint32_t        current_time = start_time;
        // ��ǰ��λ
        RenderingUnit*  current_unit = this->units;
        // �ϴε�λ
        RenderingUnit*  last_uint = this->units;
#ifdef _MSC_VER
#pragma warning(disable: 4200)
#endif
        // ��λ����
        RenderingUnit   units[0];
    };

}


// ������Ⱦ����
void LongUIMethodCall LongUI::UIWindow::reset_renderqueue() noexcept {
    assert(!(this->flags & Flag_Window_FullRendering));
    DEVMODEW mode;
    ::EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode);
    size_t size_alloc = mode.dmDisplayFrequency * LongUIPlanRenderingTotalTime 
        * sizeof(RenderingUnit);
    RenderingQueue* new_queue = reinterpret_cast<RenderingQueue*>(
        LongUICtrlAlloc(sizeof(RenderingQueue) + size_alloc)
        );
    if (new_queue) {
        new(new_queue) RenderingQueue(mode.dmDisplayFrequency);
        ZeroMemory(new_queue->units, size_alloc);
        // ����
        new_queue->queue_length = mode.dmDisplayFrequency * LongUIPlanRenderingTotalTime;
        // �����Ч
        if (m_pRenderQueue) {
            // TODO: ���ת��
            // ת��
            LongUICtrlFree(m_pRenderQueue);
        }
        // ȫˢ��һ֡
        new_queue->current_unit->length = 1;
        new_queue->current_unit->units[0] = this;
        m_pRenderQueue = new_queue;
    }
}



// �ƻ���Ⱦ�ؼ�
void LongUI::UIWindow::PlanToRender(
    float waiting_time, float rendering_time, UIControl* control) noexcept {
    assert((waiting_time + rendering_time) < float(LongUIPlanRenderingTotalTime) && "time overflow");
    if (m_pRenderQueue) {
        // ���
        auto begin_unit = m_pRenderQueue->Get(waiting_time);
        if (rendering_time > 0.f) rendering_time += 0.02f;
        auto end_unit = m_pRenderQueue->Get(waiting_time + rendering_time);
#ifdef _DEBUG
        auto sssssssslength = end_unit - begin_unit + 1;
#endif
        for (auto current_unit = begin_unit;;) {
            // Խ��
            if (current_unit >= m_pRenderQueue->units + m_pRenderQueue->queue_length) {
                current_unit = m_pRenderQueue->units;
            }
            // �Ѿ�����
            bool doit = true;
            if (current_unit->length) {
                if (current_unit->units[0] == this)
                    doit = false;
                if (current_unit->length == LongUIPlanRenderingTotalTime) {
                    current_unit->length = 1;
                    current_unit->units[0] = this;
                    doit = false;
                }
            }
            // ����!
            if (doit) {
                // ��ȡ����
                while (control != this) {
                    if (control->flags & Flag_RenderParent) {
                        control = control->parent;
                    }
                    else {
                        break;
                    }
                }
            }
            // ���Լ���д�ڵ�һ��
            if (control == this) {
                current_unit->units[0] = this;
                current_unit->length = 1;
            }
            // ���ھ����
            else if (std::none_of(
                current_unit->units,
                current_unit->units + current_unit->length,
                [=](LongUI::UIControl* unit) noexcept -> bool { return unit == control; }
                )) {
                current_unit->units[current_unit->length] = control;
                ++current_unit->length;
            }
            // ����
            if (current_unit == end_unit) {
                break;
            }
            ++current_unit;
        }
    }
    else {
        // TODO: �ӳ�ˢ��
        if (m_fConRenderTime < rendering_time) m_fConRenderTime = rendering_time;
        //m_fDeltaTime = m_timer.Delta_s<decltype(m_fDeltaTime)>();
        //m_timer.MovStartEnd();
    }
}


// �̻��������
void LongUIMethodCall LongUI::UIWindow::draw_caret() noexcept {
    // ������BeginDraw/EndDraw֮�����
    D2D1_POINT_2U pt = { m_rcCaretPx.left, m_rcCaretPx.top };
    D2D1_RECT_U src_rect;
    src_rect.top = LongUIWindowPlanningBitmap / 2;
    src_rect.left = m_bCaretIn ? 0 : LongUIWindowPlanningBitmap / 4;
    src_rect.right = src_rect.left + m_rcCaretPx.width;
    src_rect.bottom = src_rect.top + m_rcCaretPx.height;
    m_pTargetBimtap->CopyFromBitmap(
        &pt, m_pBitmapPlanning, &src_rect
        );
}

// ���²������
void LongUIMethodCall LongUI::UIWindow::refresh_caret() noexcept {
    // ������BeginDraw/EndDraw֮�����
    // TODO: ���λͼ����
}

// ���ó���
void LongUIMethodCall LongUI::UIWindow::set_present() noexcept {
    // ��ʾ���?
    if (m_cShowCaret) {
        this->draw_caret();
        m_dirtyRects[m_present.DirtyRectsCount].left = m_rcCaretPx.left;
        m_dirtyRects[m_present.DirtyRectsCount].top = m_rcCaretPx.top;
        m_dirtyRects[m_present.DirtyRectsCount].right = m_rcCaretPx.left + m_rcCaretPx.width;
        m_dirtyRects[m_present.DirtyRectsCount].bottom = m_rcCaretPx.top + m_rcCaretPx.height;
        ++m_present.DirtyRectsCount;
    }
    // ���������?
    if (m_present.DirtyRectsCount) {
        m_present.pScrollRect = &m_rcScroll;
#ifdef _DEBUG
        static RECT s_rects[LongUIDirtyControlSize + 2];
        ::memcpy(s_rects, m_dirtyRects, m_present.DirtyRectsCount * sizeof(RECT));
        m_present.pDirtyRects = s_rects;
        s_rects[m_present.DirtyRectsCount] = { 0, 0, 128, 35 };
        ++m_present.DirtyRectsCount;
#endif
    }
    else {
        m_present.pScrollRect = nullptr;
    }
}

// begin draw
void LongUIMethodCall LongUI::UIWindow::BeginDraw() noexcept {
    // ��ȡ��ǰ��С
    reinterpret_cast<D2D1_SIZE_F&>(this->show_zone.width) = m_clientSize;
    // ˢ���ӿؼ�����
    Super::UpdateChildLayout();
    // Ԥ��Ⱦ
    if (!m_vRegisteredControl.empty()) {
        for (auto i : m_vRegisteredControl) {
            auto ctrl = reinterpret_cast<UIControl*>(i);
            assert(ctrl->parent && "check it");
            m_pRenderTarget->SetTransform(&ctrl->parent->world);
            ctrl->PreRender();
        }
    }
    // ��Ϊ��ǰ��Ⱦ����
    m_pRenderTarget->SetTarget(m_pTargetBimtap);
    // ��ʼ��Ⱦ
    m_pRenderTarget->BeginDraw();
    // ����ת������
    m_pRenderTarget->SetTransform(&this->transform);
}

// end draw
auto LongUIMethodCall LongUI::UIWindow::EndDraw(uint32_t vsyc) noexcept -> HRESULT {
    // ������Ⱦ
    m_pRenderTarget->EndDraw();
    // ����
    this->set_present();
    HRESULT hr = m_pSwapChain->Present1(vsyc, 0, &m_present);
    // ��ȡˢ��ʱ��
    if (this->flags & Flag_Window_FullRendering) {
        if (m_fConRenderTime == 0.0f) m_fConRenderTime = -1.f;
        else m_fConRenderTime -= this->GetDeltaTime();
    }
    // �յ��ؽ���Ϣʱ �ؽ�UI
#ifdef _DEBUG
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || test_D2DERR_RECREATE_TARGET) {
        test_D2DERR_RECREATE_TARGET = false;
        UIManager << DL_Hint << L"D2DERR_RECREATE_TARGET!" << LongUI::endl;
#else
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#endif
        UIManager.RecreateResources();
#ifdef _DEBUG
        if (FAILED(hr)) {
            UIManager << DL_Hint << L"But, Recreate Failed!!!" << LongUI::endl;
            UIManager << DL_Error << L"Recreate Failed!!!" << LongUI::endl;
        }
#endif
    }
    // ���
    AssertHR(hr);
    return hr;
}

// UIWindow ��Ⱦ 
auto LongUIMethodCall LongUI::UIWindow::Render() noexcept ->HRESULT {
    // ���ü��ʱ��
    m_fDeltaTime = m_timer.Delta_s<decltype(m_fDeltaTime)>();
    //UIManager << DL_Log << long(m_fDeltaTime * 1000.f) << LongUI::endl;
    m_timer.MovStartEnd();
    // ��ձ���  clear_color
    m_pRenderTarget->Clear(this->clear_color);
    //
    LongUI::RenderingUnit* current_unit = nullptr;
    bool full = false;
    if (this->flags & Flag_Window_FullRendering) {
        full = true;
    }
    else if(m_pRenderQueue){
        current_unit = m_pRenderQueue->last_uint;
        if (current_unit->length && current_unit->units[0] == this) {
            full = true;
        }
    }
    // ȫˢ��: �̳и���
    if (full) {
        m_present.DirtyRectsCount = 0;
        Super::Render();
    }
    // ����ˢ��:
    else {
        m_present.DirtyRectsCount = current_unit->length;
        for (uint32_t i = 0ui32; i < current_unit->length; ++i) {
            auto ctrl = current_unit->units[i];
            assert(ctrl->parent && "check it");
            // ����ת������
            m_pRenderTarget->SetTransform(&ctrl->parent->world);
            ctrl->Render();
            D2D1_RECT_F draw_rect = GetDrawRect(ctrl);
            // ��������
            auto lefttop = LongUI::TransformPoint(
                ctrl->parent->world,
                D2D1::Point2F(ctrl->show_zone.left, ctrl->show_zone.top)
                );
            auto rightbuttom = LongUI::TransformPoint(
                ctrl->parent->world,
                D2D1::Point2F(
                    ctrl->show_zone.left + ctrl->show_zone.width,
                    ctrl->show_zone.top + ctrl->show_zone.height)
                );
            // ת��
            m_dirtyRects[i].left = static_cast<LONG>(lefttop.x);
            m_dirtyRects[i].top = static_cast<LONG>(lefttop.y);
            m_dirtyRects[i].right = static_cast<LONG>(rightbuttom.x);
            m_dirtyRects[i].bottom = static_cast<LONG>(rightbuttom.y);
        }
    }
    // ��Ч -> ����
    if (current_unit) {
        current_unit->length = 0;
    }
#ifdef _DEBUG
    // �������
    {
        D2D1_MATRIX_3X2_F nowMatrix, iMatrix = D2D1::Matrix3x2F::Scale(0.5f, 0.5f);
        m_pRenderTarget->GetTransform(&nowMatrix);
        m_pRenderTarget->SetTransform(&iMatrix);
        if (full) {
            ++full_render_counter;
        }
        else {
            ++dirty_render_counter;
        }
        wchar_t buffer[1024];
        auto length = ::swprintf(
            buffer, 1024,
            L"Full Render Count: %d\nDirty Render Count: %d\nThis DirtyRectsCount:%d",
            full_render_counter,
            dirty_render_counter,
            m_present.DirtyRectsCount
            );
        auto tf = UIManager.GetTextFormat(LongUIDefaultTextFormatIndex);
        auto ta = tf->GetTextAlignment();
        m_pBrush_SetBeforeUse->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_pRenderTarget->DrawText(
            buffer, length, tf,
            D2D1::RectF(0.f, 0.f, 1000.f, 70.f),
            m_pBrush_SetBeforeUse
            );
        tf->SetTextAlignment(ta);
        ::SafeRelease(tf);
        m_pRenderTarget->SetTransform(&nowMatrix);
    }
#endif
    return S_OK;
}

// UIWindow �¼�����
bool LongUIMethodCall LongUI::UIWindow::
DoEvent(LongUI::EventArgument& _arg) noexcept {
    // �Լ�������LongUI�¼�
    if (_arg.sender) return Super::DoEvent(_arg);
    LongUI::EventArgument new_arg;
    bool handled = false; UIControl* control_got = nullptr;
    // �����¼�
    switch (_arg.msg)
    {
    case WM_SETCURSOR:
        // ���ù��
        ::SetCursor(now_cursor);
        break;
    /*case WM_DWMCOLORIZATIONCOLORCHANGED:
    {
        DWORD buffer = sizeof(color_a);
        ::RegGetValueA(
            HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\DWM",
            "ColorizationColor",
            RRF_RT_DWORD,
            nullptr,
            &color_a,
            &buffer
            );
        this->Invalidate(this);
    }
    break;*/
    case WM_MOUSEMOVE:
        ::TrackMouseEvent(&m_csTME);
        if (m_normalLParam != _arg.lParam_sys) {
            m_normalLParam = _arg.lParam_sys;
        }
        else {
            handled = true;
            break;
        }
        // �д�����ؼ�
        if (m_pCapturedControl) {
            m_pCapturedControl->DoEventEx(_arg);
            handled = true;
            break;
        }
        // �����ӿؼ�
        control_got = this->FindControl(_arg.pt);
        // �ؼ���Ч
        if (control_got){
            if (control_got != m_pPointedControl ) {
                new_arg = _arg;
                new_arg.sender = this;
                if (m_pPointedControl) {
                    new_arg.event = LongUI::Event::Event_MouseLeave;
                    m_pPointedControl->DoEventEx(new_arg);
                }
                m_pPointedControl = control_got;
                new_arg.event = LongUI::Event::Event_MouseEnter;
                m_pPointedControl->DoEventEx(new_arg);
            }
            // ��ͬ
            else {
                control_got->DoEventEx(_arg);
            }
        }
        handled = true;
        break;
    case WM_TIMER:
        // ��˸?
        if (_arg.wParam_sys == 0 && m_cShowCaret) {
            // ��˸
            m_present.DirtyRectsCount = 0;
            this->set_present();
            m_pSwapChain->Present1(0, 0, &m_present);
            m_bCaretIn = !m_bCaretIn;
        }
        handled = true;
        break;
    case WM_LBUTTONDOWN:    // ����������
        // �����ӿؼ�
        control_got = this->FindControl(_arg.pt);
        // �ؼ���Ч
        if (control_got && control_got != m_pFocusedControl){
            new_arg = _arg;
            new_arg.sender = this;
            if (m_pFocusedControl){
                new_arg.event = LongUI::Event::Event_KillFocus;
                m_pFocusedControl->DoEventEx(new_arg);
            }
            new_arg.event = LongUI::Event::Event_SetFocus;
            // �ؼ���Ӧ��?
            m_pFocusedControl = control_got->DoEventEx(new_arg) ? control_got : nullptr;
        }
        break;
    /*case WM_NCHITTEST:
        _arg.lr = HTCAPTION;
        handled = true;
        break;*/
    case WM_SETFOCUS:
        ::CreateCaret(m_hwnd, nullptr, 1, 1);
        handled = true;
        break;
    case WM_KILLFOCUS:
        // ���ڽ���ؼ�
        if (m_pFocusedControl){
            new_arg = _arg;
            new_arg.sender = this;
            new_arg.event = LongUI::Event::Event_KillFocus;
            m_pFocusedControl->DoEvent(new_arg);
            m_pFocusedControl = nullptr;
        }
        ::DestroyCaret();
        handled = true;
        break;
    case WM_MOUSELEAVE:     // ����Ƴ�����
        if (m_pPointedControl){
            new_arg = _arg;
            new_arg.sender = this;
            new_arg.event = LongUI::Event::Event_MouseLeave;
            m_pPointedControl->DoEvent(new_arg);
            m_pPointedControl = nullptr;
        }
        handled = true;
        break;
    case WM_SIZE:           // �ı��С
        this->DrawSizeChanged();
        if (_arg.lParam_sys && m_pSwapChain){
            this->OnResize();
            // ǿ��ˢ��һ֡
            this->Invalidate(this);
            this->UpdateRendering();
            this->BeginDraw();
            this->Render();
            this->EndDraw(0);
            handled = true;
        }
        break;
    case WM_GETMINMAXINFO:  // ��ȡ���ƴ�С
        reinterpret_cast<MINMAXINFO*>(_arg.lParam_sys)->ptMinTrackSize.x = 128;
        reinterpret_cast<MINMAXINFO*>(_arg.lParam_sys)->ptMinTrackSize.y = 128;
        break;
    case WM_DISPLAYCHANGE:
        // ���·ֱ���
        if(!(this->flags & Flag_Window_FullRendering))   this->reset_renderqueue();
        // TODO: OOM����
        break;
    case WM_CLOSE:          // �رմ���
        // ���ڹر�
        this->Close();
        handled = true;
        break;
    }
    // ����
    if (handled) return true;
    // ����ؼ�
    register UIControl* processor = nullptr;
    // ����¼����ɲ���ؼ�(����)�������ָ��ؼ�����
    if (_arg.msg >= WM_MOUSEFIRST && _arg.msg <= WM_MOUSELAST) {
        processor = m_pCapturedControl ? m_pCapturedControl : m_pPointedControl;
    }
    // �����¼����ɽ���ؼ�����
    else {
        processor = m_pFocusedControl;
    }
    // �оʹ���
    // �����˾�ֱ�ӷ���
    if (processor && processor->DoEventEx(_arg)) {
        return true;
    }
    // ����û�д���ͽ������ദ��
    return Super::DoEvent(_arg);
}


// ˢ����Ⱦ״̬
bool LongUI::UIWindow::UpdateRendering() noexcept {
    bool isrender = false;
    // ������Ⱦ����
    if (m_pRenderQueue) {
        if (m_pRenderQueue->current_unit->length) {
            isrender = true;
            m_pRenderQueue->last_uint = m_pRenderQueue->current_unit;
        }
        m_pRenderQueue->Update();
    }
    // ȫˢ��
    else {
        isrender = m_fConRenderTime >= 0.0f;
    }
    return isrender;
}

// ���ô��ڴ�С
void LongUI::UIWindow::OnResize(bool force) noexcept {
    // �޸Ĵ�С
    this->DrawSizeChanged(); m_pRenderTarget->SetTarget(nullptr);
    RECT rect; ::GetClientRect(m_hwnd, &rect);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    // ����
    m_rcScroll.right = rect.right;
    m_rcScroll.bottom = rect.bottom;

    m_clientSize.width = static_cast<float>(rect.right);
    m_clientSize.height = static_cast<float>(rect.bottom);
    rect.right = MakeAsUnit(rect.right);
    rect.bottom = MakeAsUnit(rect.bottom);
    auto old_size = m_pTargetBimtap->GetPixelSize();
    register HRESULT hr = S_OK;
    // ǿ�� ���� С�ڲ�Resize
    if (force || old_size.width < uint32_t(rect.right) || old_size.height < uint32_t(rect.bottom)) {
        UIManager << DL_Hint << L"Window: [" << this->GetNameStr() << L"] \n\t\tTarget Bitmap Resize to " 
            << long(rect.right) << ", " << long(rect.bottom) << LongUI::endl;
        IDXGISurface* pDxgiBackBuffer = nullptr;
        ::SafeRelease(m_pTargetBimtap);
        hr = m_pSwapChain->ResizeBuffers(2, rect.right, rect.bottom, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        // ���
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            UIManager.RecreateResources();
            UIManager << DL_Hint << L"Recreate device" << LongUI::endl;
        }
        // ���ý�������ȡDxgi����
        if (SUCCEEDED(hr)) {
            hr = m_pSwapChain->GetBuffer(0, LongUI_IID_PV_ARGS(pDxgiBackBuffer));
        }
        // ����Dxgi���洴��λͼ
        if (SUCCEEDED(hr)) {
            D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.0f,
                96.0f
                );
            hr = m_pRenderTarget->CreateBitmapFromDxgiSurface(
                pDxgiBackBuffer,
                &bitmapProperties,
                &m_pTargetBimtap
                );
        }
        if (FAILED(hr)) {
            UIManager << DL_Error << L" Recreate FAILED!" << LongUI::endl;
            AssertHR(hr);
        }
        ::SafeRelease(pDxgiBackBuffer);
    }
    // ǿ��ˢ��һ֡
    this->Invalidate(this);
}


// UIWindow �ؽ�
auto LongUIMethodCall LongUI::UIWindow::Recreate(LongUIRenderTarget* newRT) noexcept ->HRESULT {
    // UIWindow::Recreate��������Ϊnullptr
    assert(newRT && "bad argument");
    // DXGI Surface ��̨����
    IDXGISurface*                        pDxgiBackBuffer = nullptr;
    this->release_data();
    // ����������
    IDXGIFactory2* pDxgiFactory = UIManager;
    assert(pDxgiFactory);
    HRESULT hr = S_OK;
    // ����������
    if (SUCCEEDED(hr)) {
        RECT rect = { 0 }; ::GetClientRect(m_hwnd, &rect);
        // ��������Ϣ
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = MakeAsUnit(rect.right - rect.left);
        swapChainDesc.Height = MakeAsUnit(rect.bottom - rect.top);
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.Flags = 0;
        // ����
        m_rcScroll.right = rect.right - rect.left;
        m_rcScroll.bottom = rect.bottom - rect.top;
        if (this->flags & Flag_Window_DComposition) {
            // DirectComposition����Ӧ�ó���
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            // ����DirectComposition������
            hr = pDxgiFactory->CreateSwapChainForComposition(
                UIManager_DXGIDevice,
                &swapChainDesc,
                nullptr,
                &m_pSwapChain
                );
        }
        else {
            // һ������Ӧ�ó���
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            // ���ô��ھ������������
            hr = pDxgiFactory->CreateSwapChainForHwnd(
                UIManager_D3DDevice,
                m_hwnd,
                &swapChainDesc,
                nullptr,
                nullptr,
                &m_pSwapChain
                );
        }
    }
    // ȷ��DXGI������߲��ᳬ��һ֡
    if (SUCCEEDED(hr)) {
        hr = UIManager_DXGIDevice->SetMaximumFrameLatency(1);
    }
    // ���ý�������ȡDxgi����
    if (SUCCEEDED(hr)) {
        hr = m_pSwapChain->GetBuffer(0, LongUI_IID_PV_ARGS(pDxgiBackBuffer));
    }
    // ����Dxgi���洴��λͼ
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f
            );
        hr = newRT->CreateBitmapFromDxgiSurface(
            pDxgiBackBuffer,
            &bitmapProperties,
            &m_pTargetBimtap
            );
    }
    // �����ƻ�λͼ
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f
            );
        hr = newRT->CreateBitmap(
            D2D1::SizeU(LongUIWindowPlanningBitmap, LongUIWindowPlanningBitmap),
            nullptr, 0,
            &bitmapProperties,
            &m_pBitmapPlanning
            );
    }
    // ʹ��DComp
    if (this->flags & Flag_Window_DComposition) {
        // ����ֱ�����(Direct Composition)�豸
        if (SUCCEEDED(hr)) {
            hr = LongUI::Dll::DCompositionCreateDevice(
                UIManager_DXGIDevice,
                LongUI_IID_PV_ARGS(m_pDcompDevice)
                );
        }
        // ����ֱ�����(Direct Composition)Ŀ��
        if (SUCCEEDED(hr)) {
            hr = m_pDcompDevice->CreateTargetForHwnd(
                m_hwnd, true, &m_pDcompTarget
                );
        }
        // ����ֱ�����(Direct Composition)�Ӿ�
        if (SUCCEEDED(hr)) {
            hr = m_pDcompDevice->CreateVisual(&m_pDcompVisual);
        }
        // ���õ�ǰ������Ϊ�Ӿ�����
        if (SUCCEEDED(hr)) {
            hr = m_pDcompVisual->SetContent(m_pSwapChain);
        }
        // ���õ�ǰ�Ӿ�Ϊ����Ŀ��
        if (SUCCEEDED(hr)) {
            hr = m_pDcompTarget->SetRoot(m_pDcompVisual);
        }
        // ��ϵͳ�ύ
        if (SUCCEEDED(hr)) {
            hr = m_pDcompDevice->Commit();
        }
    }
    // ����
    if (FAILED(hr)){
        UIManager << L"Recreate Failed!" << LongUI::endl;
        AssertHR(hr);
    }
    ::SafeRelease(pDxgiBackBuffer);
    // ���ù滮λͼ
    if (SUCCEEDED(hr)) {
        constexpr float PBS = float(LongUIWindowPlanningBitmap);
        // �������Ϳ��
        newRT->SetTarget(m_pBitmapPlanning);
        newRT->BeginDraw();
        newRT->PushAxisAlignedClip(
            D2D1::RectF(0.f, PBS*0.5f, PBS * 0.25f, PBS),
            D2D1_ANTIALIAS_MODE_ALIASED
            );
        newRT->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        newRT->PopAxisAlignedClip();
        newRT->PushAxisAlignedClip(
            D2D1::RectF(PBS * 0.25f, PBS*0.5f, PBS * 0.5f, PBS),
            D2D1_ANTIALIAS_MODE_ALIASED
            );
        newRT->Clear(D2D1::ColorF(D2D1::ColorF::White));
        newRT->PopAxisAlignedClip();
        newRT->EndDraw();
    }
    // �ؽ� �ӿؼ�UI
    return Super::Recreate(newRT);
}

// UIWindow �رտؼ�
void LongUI::UIWindow::Close() noexcept {
    // ɾ������
    delete this;
    // �˳�
    UIManager.Exit();
}


// ----------------- IDropTarget!!!! Yooooooooooo~-----

// ���ò���
void __fastcall SetLongUIEventArgument(LongUI::EventArgument& arg, HWND hwnd, POINTL pt) {
    // ��ȡ����λ��
    RECT rc = { 0 }; ::GetWindowRect(hwnd, &rc);
    // ӳ�䵽��������
    POINT ppt = { pt.x, pt.y };  ::ScreenToClient(hwnd, &ppt);
    // ���Ҷ�Ӧ�ؼ�
    arg = { 0 };
    arg.pt.x = static_cast<float>(ppt.x);
    arg.pt.y = static_cast<float>(ppt.y);

}

// ��ȡ�Ϸ�Ч��
DWORD __fastcall GetDropEffect(DWORD grfKeyState, DWORD dwAllowed) {
    register DWORD dwEffect = 0;
    // 1. ���pt�����Ƿ�����drop������ĳ��λ��
    // 2. ���������grfKeyState��dropЧ��
    if (grfKeyState & MK_CONTROL) {
        dwEffect = dwAllowed & DROPEFFECT_COPY;
    }
    else if (grfKeyState & MK_SHIFT) {
        dwEffect = dwAllowed & DROPEFFECT_MOVE;
    }
    // 3. �Ǽ������η�ָ��(��dropЧ��������), ��˻���dropԴ��Ч��
    if (dwEffect == 0) {
        if (dwAllowed & DROPEFFECT_COPY) dwEffect = DROPEFFECT_COPY;
        if (dwAllowed & DROPEFFECT_MOVE) dwEffect = DROPEFFECT_MOVE;
    }
    return dwEffect;
}

// IDropTarget::DragEnter ʵ��
HRESULT STDMETHODCALLTYPE LongUI::UIWindow::DragEnter(IDataObject* pDataObj,
    DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) noexcept {
    m_bInDraging = true;
    // ������
    if (!pDataObj) return E_INVALIDARG;
    // ȡ���۽�����
    if(m_pFocusedControl){
        LongUI::EventArgument arg = { 0 };
        arg.sender = this;
        arg.event = LongUI::Event::Event_KillFocus;
        m_pFocusedControl->DoEventEx(arg);
        m_pFocusedControl = nullptr;
    }
    // ��������
    ::SafeRelease(m_pCurDataObject);
    m_pCurDataObject = SafeAcquire(pDataObj);
    // �ɰ���������
    POINT ppt = { pt.x, pt.y };
    if (m_pDropTargetHelper) {
        m_pDropTargetHelper->DragEnter(m_hwnd, pDataObj, &ppt, *pdwEffect);
    }
    return S_OK;
}


// IDropTarget::DragOver ʵ��
HRESULT STDMETHODCALLTYPE LongUI::UIWindow::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) noexcept {
    LongUI::EventArgument arg;
    ::SetLongUIEventArgument(arg, m_hwnd, pt);
    arg.sender = this;
    arg.event = LongUI::Event::Event_FindControl;
    // ���ؼ�֧��
    if (Super::DoEventEx(arg) && arg.ctrl) {
        // ��һ���ؼ�?
        if (m_pDragDropControl == arg.ctrl) {
            // һ������Over
            arg.event = LongUI::Event::Event_DragOver;
        }
        else {
            // ���Ͽؼ������뿪�¼�
            if (m_pDragDropControl) {
                arg.event = LongUI::Event::Event_DragLeave;
                m_pDragDropControl->DoEventEx(arg);
            }
            // �¿ؼ����ͽ���
            arg.event = LongUI::Event::Event_DragEnter;
            m_pDragDropControl = arg.ctrl;
        }
        arg.dataobj_cf = m_pCurDataObject;
        arg.outeffect_cf = pdwEffect;
        if (!arg.ctrl->DoEventEx(arg)) *pdwEffect = DROPEFFECT_NONE;
    }
    else {
        // ��֧��
        *pdwEffect = DROPEFFECT_NONE;
    }
    // �ɰ���������
    if (m_pDropTargetHelper) {
        POINT ppt = { pt.x, pt.y };
        m_pDropTargetHelper->DragOver(&ppt, *pdwEffect);
    }
    return S_OK;
}

// IDropTarget::DragLeave ʵ��
HRESULT LongUI::UIWindow::DragLeave(void) noexcept {
    // �����¼�
    if (m_pDragDropControl) {
        LongUI::EventArgument arg = { 0 };
        arg.sender = this;
        arg.event = LongUI::Event::Event_DragLeave;
        m_pDragDropControl->DoEventEx(arg);
        m_pDragDropControl = nullptr;
        // ���ڲ���ؼ�?
        /*if (m_pCapturedControl) {
            this->ReleaseCapture();
            /*arg.sender = nullptr;
            arg.msg = WM_LBUTTONUP;
            m_pCapturedControl->DoEventEx(arg);
        }*/
    }
    /*OnDragLeave(m_hTargetWnd);*/
    m_pDragDropControl = nullptr;
    //m_isDataAvailable = TRUE;
    if (m_pDropTargetHelper) {
        m_pDropTargetHelper->DragLeave();
    }
    m_bInDraging = false;
    return S_OK;
}

// IDropTarget::Drop ʵ��
HRESULT LongUI::UIWindow::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) noexcept {
    // �����¼�
    if (m_pDragDropControl) {
        LongUI::EventArgument arg;
        ::SetLongUIEventArgument(arg, m_hwnd, pt);
        arg.sender = this;
        arg.event = LongUI::Event::Event_Drop;
        arg.dataobj_cf = m_pCurDataObject;
        arg.outeffect_cf = pdwEffect;
        // �����¼�
        m_pDragDropControl->DoEventEx(arg);
        m_pDragDropControl = nullptr;
        
    }
    // ������
    if (!pDataObj) return E_INVALIDARG;
    if (m_pDropTargetHelper){
        POINT ppt = { pt.x, pt.y };
        m_pDropTargetHelper->Drop(pDataObj, &ppt, *pdwEffect);
    }
    *pdwEffect = ::GetDropEffect(grfKeyState, *pdwEffect);
    return S_OK;
}
