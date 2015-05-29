
#include "LongUI.h"
/*
        // Render ��Ⱦ --- ���ڵ�һλ!
        virtual auto LongUIMethodCall Render() noexcept ->HRESULT override;
        // do event �¼�����
        virtual bool LongUIMethodCall DoEvent(LongUI::EventArgument&) noexcept override;
        // Ԥ��Ⱦ
        virtual void LongUIMethodCall PreRender() noexcept override {};
        // recreate �ؽ�
        virtual auto LongUIMethodCall Recreate(LongUIRenderTarget*) noexcept ->HRESULT override;
        // close this control �رտؼ�
        virtual void LongUIMethodCall Close() noexcept override;
*/

// Ҫ��:
// 1. ���¿ռ���ʾ����С
//      -> �й�����, ��������������
//      -> û��, �����Լ�����


// TODO: ������пؼ�Render, ��Ҫ����UIControl::Render;

// UIControl ���캯��
LongUI::UIControl::UIControl(pugi::xml_node node) noexcept {
    // ����Ĭ��
    int flag = LongUIFlag::Flag_None | LongUIFlag::Flag_Visible;
    // ��Ч?
    if (node) {
        m_pScript = UIManager.script;
        const char* data = nullptr;
        // ���ű�
        if ((data = node.attribute("script").value()) && m_pScript) {
            m_script = m_pScript->AllocScript(data);
        }
        else {
            m_script.data = nullptr;
            m_script.size = 0;
        }
        // �����Ⱦ���ؼ�
        if (node.attribute("renderparent").as_bool(false)) {
            flag |= LongUI::Flag_RenderParent;
        }
        // �������
        UIControl::MakeString(node.attribute("name").value(), m_strControlName);
        // ���λ��
        UIControl::MakeFloats(node.attribute("pos").value(), const_cast<float*>(&show_zone.left), 4);
        // ��ȹ̶�
        if (show_zone.width > 0.f) {
            flag |= LongUI::Flag_WidthFixed;
        }
        // �߶ȹ̶�
        if (show_zone.height > 0.f) {
            flag |= LongUI::Flag_HeightFixed;
        }
    }
    else  {
        // ����
        //UIManager << DL_Warning << L"given a null xml node" << LongUI::endl;
    }
    // �޸�flag
    force_cast(this->flags) = static_cast<LongUIFlag>(this->flags | (flag));
}

// ��������
LongUI::UIControl::~UIControl() noexcept {
    ::SafeRelease(m_pRenderTarget);
    ::SafeRelease(m_pBrush_SetBeforeUse);
    // �ͷŽű�ռ�ÿռ�
    if (m_script.data) {
        assert(m_pScript && "no script interface but data");
        m_pScript->FreeScript(m_script);
    }
    // ��ע��
    if (this->flags & Flag_NeedPreRender) {
        m_pWindow->UnRegisterPreRender(this);
    }
}


// ��Ⱦ�ؼ�
auto LongUIMethodCall LongUI::UIControl::Render() noexcept -> HRESULT {
    m_bDrawPosChanged = false;
    m_bDrawSizeChanged = false;
    return S_OK;
}


// �ؽ�
HRESULT LongUIMethodCall LongUI::UIControl::Recreate(LongUIRenderTarget* target) noexcept {
    ::SafeRelease(m_pRenderTarget);
    ::SafeRelease(m_pBrush_SetBeforeUse);
    m_pRenderTarget = ::SafeAcquire(target);
    m_pBrush_SetBeforeUse = static_cast<decltype(m_pBrush_SetBeforeUse)>(
        UIManager.GetBrush(LongUICommonSolidColorBrushIndex)
        );
    return target ? S_OK : E_INVALIDARG;
}

// ת������DoEvent
bool LongUIMethodCall LongUI::UIControl::DoEventEx(LongUI::EventArgument& arg) noexcept {
    auto old = arg.pt;
    D2D1_MATRIX_3X2_F* transform;
    if (this->parent) {
        transform = &this->parent->world;
    }
    else {
        assert(this->flags & Flag_UIContainer);
        transform = &static_cast<UIContainer*>(this)->transform;
    }
    // ת��
    arg.pt = LongUI::TransformPointInverse(*transform, arg.pt);
    auto code = this->DoEvent(arg);
    arg.pt = old;
    return code;
}

// �����ַ���
bool LongUI::UIControl::MakeString(const char* data, CUIString& str) noexcept {
    if (!data || !*data) return false;
    wchar_t buffer[LongUIStringBufferLength];
    // ת��
    auto length = LongUI::UTF8toWideChar(data, buffer);
    buffer[length] = L'\0';
    // �����ַ���
    str.Set(buffer, length);
    return true;
}

// ��������
bool LongUI::UIControl::MakeFloats(const char* sdata, float* fdata, int size) noexcept {
    if (!sdata || !*sdata) return false;
    // ����
    assert(fdata && size && "bad argument");
    // ��������
    char buffer[LongUIStringBufferLength];
    ::strcpy_s(buffer, sdata);
    char* index = buffer;
    const char* to_parse = buffer;
    // �������
    bool new_float = true;
    while (size) {
        char ch = *index;
        // �ֶη�?
        if (ch == ',' || ch == ' ' || !ch) {
            if (new_float) {
                *index = 0;
                *fdata = ::LongUI::AtoF(to_parse);
                ++fdata;
                --size;
                new_float = false;
            }
        }
        else if (!new_float) {
            to_parse = index;
            new_float = true;
        }
        // �˳�
        if (!ch) break;
        ++index;
    }
    return true;
}


// 16����
unsigned int __fastcall Hex2Int(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    else {
        return c - '0';
    }
}

#define white_space(c) ((c) == ' ' || (c) == '\t')


// ��ȡ��ɫ��ʾ
bool LongUI::UIControl::MakeColor(const char* data, D2D1_COLOR_F& color) noexcept {
    if (!data || !*data) return false;
    // ��ȡ��Чֵ
    while (white_space(*data)) ++data;
    // ��#��ͷ?
    if (*data == '#') {
        color.a = 1.f;
        // #RGB
        if (data[4] == ' ' || !data[4]) {
            color.r = static_cast<float>(::Hex2Int(*++data)) / 15.f;
            color.g = static_cast<float>(::Hex2Int(*++data)) / 15.f;
            color.b = static_cast<float>(::Hex2Int(*++data)) / 15.f;
        }
        // #RRGGBB
        else if (data[7] == ' ' || !data[7]) {
            color.r = static_cast<float>((::Hex2Int(*++data) << 4) | (::Hex2Int(*++data))) / 255.f;
            color.g = static_cast<float>((::Hex2Int(*++data) << 4) | (::Hex2Int(*++data))) / 255.f;
            color.b = static_cast<float>((::Hex2Int(*++data) << 4) | (::Hex2Int(*++data))) / 255.f;
        }
        // #AARRGGBB
        else {
            color.a = static_cast<float>((::Hex2Int(*++data) << 4) | (::Hex2Int(*++data))) / 255.f;
            color.r = static_cast<float>((::Hex2Int(*++data) << 4) | (::Hex2Int(*++data))) / 255.f;
            color.g = static_cast<float>((::Hex2Int(*++data) << 4) | (::Hex2Int(*++data))) / 255.f;
            color.b = static_cast<float>((::Hex2Int(*++data) << 4) | (::Hex2Int(*++data))) / 255.f;
        }
        return true;
    }
    // ��������
    else {
        return UIControl::MakeFloats(data, reinterpret_cast<float*>(&color), 4);
    }
}

// LongUI::UIControl ע��ص��¼�
void LongUIMethodCall  LongUI::UIControl::SetEventCallBack(
    const wchar_t* control_name, LongUI::Event event, LongUICallBack call) noexcept {
    assert(control_name && call&&  "bad argument");
    UIControl* control = UIManager.FindControlW(control_name);
    assert(control && " no control found");
    if (!control) return;
    // �Զ�����Ϣ?
    if (event >= LongUI::Event::Event_CustomEvent) {
        UIManager.configure->SetEventCallBack(
            event, call, control, this
            );
        return;
    }
    switch (event)
    {
    case LongUI::Event::Event_ButtoClicked:
        static_cast<UIButton*>(control)->RegisterClickEvent(call, this);
        break;
    case LongUI::Event::Event_EditReturn:
        //static_cast<UIEdit*>(control)->RegisterReturnEvent(call, this);
        break;
    case LongUI::Event::Event_SliderValueChanged:
        static_cast<UISlider*>(control)->RegisterValueChangedEvent(call, this);
        break;
    }
}



// -------------------------------------------------------
// UILabel
// -------------------------------------------------------


// Render ��Ⱦ 
auto LongUIMethodCall LongUI::UILabel::Render() noexcept ->HRESULT {
    //
    if (m_bDrawSizeChanged) {
        this->draw_zone = this->show_zone;
        // ���ô�С
        m_text.SetNewSize(this->draw_zone.width, this->draw_zone.height);
        // super will do it
        //m_bDrawSizeChanged = false;
    }
    // ��Ⱦ����
    m_text.Render(this->draw_zone.left, this->draw_zone.top);
    return Super::Render();
}


// UILabel ���캯��
/*LongUI::UILabel::UILabel(pugi::xml_node node) noexcept: Super(node), m_text(node) {
    //m_bInitZoneChanged = true;
}*/


// UILabel::CreateControl ����
LongUI::UIControl* LongUI::UILabel::CreateControl(pugi::xml_node node) noexcept {
    if (!node) {
        UIManager << DL_Warning << L"node null" << LongUI::endl;
    }
    // ����ռ�
    auto pControl = LongUI::UIControl::AllocRealControl<LongUI::UILabel>(
        node,
        [=](void* p) noexcept { new(p) UILabel(node);}
    );
    if (!pControl) {
        UIManager << DL_Error << L"alloc null" << LongUI::endl;
    }
    return pControl;
}



// do event �¼�����
bool LongUIMethodCall LongUI::UILabel::DoEvent(LongUI::EventArgument& arg) noexcept {
    if (arg.sender) {
        if (arg.event == LongUI::Event::Event_FindControl &&
            IsPointInRect(this->show_zone, arg.pt)) {
            arg.ctrl = this;
        }
    }
    return false;
}

// recreate �ؽ�
/*HRESULT LongUIMethodCall LongUI::UILabel::Recreate(LongUIRenderTarget* newRT) noexcept {
// ����
return Super::Recreate(newRT);
}*/

// close this control �رտؼ�
void LongUIMethodCall LongUI::UILabel::Close() noexcept {
    delete this;
}


// -------------------------------------------------------
// UIButton
// -------------------------------------------------------

// Render ��Ⱦ 
auto LongUI::UIButton::Render() noexcept ->HRESULT {
    // ���¿̻�����
    if (m_bDrawSizeChanged) {
        this->draw_zone = this->show_zone;
        // super will do it
        // m_bDrawSizeChanged = false;
    }
    D2D1_RECT_F draw_rect = GetDrawRect(this);
    m_uiElement.Render(&draw_rect);
    // ���¼�ʱ��
    UIElement_Update(m_uiElement);
    return Super::Render();
}


// UIButton ���캯��
LongUI::UIButton::UIButton(pugi::xml_node node) noexcept :
Super(node), m_uiElement(node, nullptr) {

}

// UIButton ��������
LongUI::UIButton::~UIButton() noexcept {
    ::SafeRelease(m_pBGBrush);
}


// UIButton::CreateControl ����
auto LongUI::UIButton::CreateControl(pugi::xml_node node) noexcept ->UIControl* {
    if (!node) {
        UIManager << DL_Warning << L"node null" << LongUI::endl;
    }
    // ����ռ�
    auto pControl = LongUI::UIControl::AllocRealControl<LongUI::UIButton>(
        node,
        [=](void* p) noexcept { new(p) UIButton(node);}
    );
    if (!pControl) {
        UIManager << DL_Error << L"alloc null" << LongUI::endl;
    }
    return pControl;
}


// do event �¼�����
bool LongUI::UIButton::DoEvent(LongUI::EventArgument& arg) noexcept {
    if (arg.sender) {
        switch (arg.event)
        {
        case LongUI::Event::Event_FindControl:
            if (IsPointInRect(this->show_zone, arg.pt)) {
                arg.ctrl = this;
            }
            __fallthrough;
        case LongUI::Event::Event_SetFocus:
            return true;
        case LongUI::Event::Event_KillFocus:
            m_tarStatusClick = LongUI::Status_Normal;
            return true;
        case LongUI::Event::Event_MouseEnter:
            //m_bEffective = true;
            UIElement_SetNewStatus(m_uiElement, LongUI::Status_Hover);
            break;
        case LongUI::Event::Event_MouseLeave:
            //m_bEffective = false;
            UIElement_SetNewStatus(m_uiElement, LongUI::Status_Normal);
            break;
        }
    }
    else {
        bool rec = false;
        arg.sender = this;   auto tempmsg = arg.msg;
        switch (arg.msg)
        {
        case WM_LBUTTONDOWN:
            m_pWindow->SetCapture(this);
            UIElement_SetNewStatus(m_uiElement, LongUI::Status_Pushed);
            break;
        case WM_LBUTTONUP:
            arg.event = LongUI::Event::Event_ButtoClicked;
            m_tarStatusClick = LongUI::Status_Hover;
            // ���ű�
            if (m_pScript && m_script.data) {
                m_pScript->Evaluation(m_script, arg);
            }
            // ����Ƿ����¼��ص�
            if (m_eventClick) {
                rec = (m_pClickTarget->*m_eventClick)(this);
            }
            else {
                // �������¼�������
                rec = m_pWindow->DoEvent(arg);
            }
            arg.msg = tempmsg;
            UIElement_SetNewStatus(m_uiElement, m_tarStatusClick);
            m_pWindow->ReleaseCapture();
            break;
        }
        arg.sender = nullptr;
    }
    return Super::DoEvent(arg);
}

// recreate �ؽ�
auto LongUI::UIButton::Recreate(LongUIRenderTarget* newRT) noexcept ->HRESULT {
    SafeRelease(m_pBGBrush);
    newRT->CreateSolidColorBrush(m_uiElement.colors, nullptr, &m_pBGBrush);
    m_uiElement.target = newRT;
    m_uiElement.brush = m_pBGBrush;
    m_uiElement.InitStatus(LongUI::Status_Normal);
    // ���ദ��
    return Super::Recreate(newRT);
}

// �رտؼ�
void LongUI::UIButton::Close() noexcept {
    delete this;
}


// -------------------------------------------------------
// UIEdit
// -------------------------------------------------------


HRESULT LongUIMethodCall LongUI::UIEditBasic::Render() noexcept {
    // ���¿̻�����
    if (m_bDrawSizeChanged) {
        this->draw_zone = this->show_zone;
        // super will do it
        // m_bDrawSizeChanged = false;
        m_text.SetNewSize(this->draw_zone.width, this->draw_zone.height);
    }
    m_text.Render(this->draw_zone.left, this->draw_zone.top);
    return Super::Render();
}

// do event 
bool    LongUIMethodCall LongUI::UIEditBasic::DoEvent(LongUI::EventArgument& arg) noexcept {
    if (arg.sender) {
        switch (arg.event)
        {
        case LongUI::Event::Event_FindControl: // ���ұ��ؼ�
            if (IsPointInRect(this->show_zone, arg.pt)) {
                arg.ctrl = this;
            }
            return true;
        case LongUI::Event::Event_FinishedTreeBuliding:
            return true;
        case LongUI::Event::Event_DragEnter:
            return m_text.OnDragEnter(arg.dataobj_cf, arg.outeffect_cf);
        case LongUI::Event::Event_DragOver:
            return m_text.OnDragOver(
                arg.pt.x - this->show_zone.left,
                arg.pt.y - this->show_zone.top
                );
        case LongUI::Event::Event_DragLeave:
            return m_text.OnDragLeave();
        case LongUI::Event::Event_Drop:
            return m_text.OnDrop(arg.dataobj_cf, arg.outeffect_cf);
        case LongUI::Event::Event_MouseEnter:
            m_pWindow->now_cursor = m_hCursorI;
            return true;
        case LongUI::Event::Event_MouseLeave:
            m_pWindow->now_cursor = m_pWindow->default_cursor;
            return true;
        case LongUI::Event::Event_SetFocus:
            m_text.OnSetFocus();
            return true;
        case LongUI::Event::Event_KillFocus:
            m_text.OnKillFocus();
            return true;
        }
    }
    else {
        switch (arg.msg)
        {
        default:
            return false;
        case WM_KEYDOWN:
            m_text.OnKey(static_cast<uint32_t>(arg.wParam_sys));
            break;
        case WM_CHAR:
            m_text.OnChar(static_cast<char32_t>(arg.wParam_sys));
            break;
        case WM_MOUSEMOVE:
            // ��ק?
            if (arg.wParam_sys & MK_LBUTTON) {
                m_text.OnLButtonHold(
                    arg.pt.x - this->show_zone.left,
                    arg.pt.y - this->show_zone.top
                    );
            }
            break;
        case WM_LBUTTONDOWN:
            m_text.OnLButtonDown(
                arg.pt.x - this->show_zone.left,
                arg.pt.y - this->show_zone.top,
                (arg.wParam_sys & MK_SHIFT) > 0
                );
            break;
        case WM_LBUTTONUP:
            m_text.OnLButtonUp(
                arg.pt.x - this->show_zone.left,
                arg.pt.y - this->show_zone.top
                );
            break;
        }
    }
    return false;
}

// close this control �رտؼ�
HRESULT    LongUIMethodCall LongUI::UIEditBasic::Recreate(LongUIRenderTarget* target) noexcept {
    m_text.Recreate(target);
    return Super::Recreate(target);
}

// close this control �رտؼ�
void    LongUIMethodCall LongUI::UIEditBasic::Close() noexcept {
    delete this;
}


// UIEditBasic::CreateControl ����
LongUI::UIControl* LongUI::UIEditBasic::CreateControl(pugi::xml_node node) noexcept {
    if (!node) {
        UIManager << DL_Warning << L"node null" << LongUI::endl;
    }
    // ����ռ�
    auto pControl = LongUI::UIControl::AllocRealControl<LongUI::UIEditBasic>(
        node,
        [=](void* p) noexcept { new(p) UIEditBasic(node);}
    );
    if (!pControl) {
        UIManager << DL_Error << L"alloc null" << LongUI::endl;
    }
    return pControl;
}


// ���캯��
LongUI::UIEditBasic::UIEditBasic(pugi::xml_node node) noexcept
    :  Super(node), m_text(this, node) {
}



