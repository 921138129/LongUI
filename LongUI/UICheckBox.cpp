
#include "LongUI.h"



// Render ��Ⱦ 
HRESULT LongUIMethodCall LongUI::UICheckBox::Render() noexcept {
    if (m_bDrawSizeChanged) {
        this->draw_zone = this->show_zone;
    }
    D2D1_RECT_F draw_rect = GetDrawRect(this);
    draw_rect.left += 1.f;
    // ������Ⱦ��
    draw_rect.top = (draw_rect.bottom + draw_rect.top - m_szCheckBox.height) * 0.5f;
    draw_rect.bottom = draw_rect.top + m_szCheckBox.height;
    draw_rect.right = draw_rect.left + m_szCheckBox.width;
    // ��Ⱦ��
    m_pRenderTarget->DrawRectangle(
        draw_rect, m_pBrush, 1.5f
        );
    // ��Ⱦ��ͷ
    switch (this->state)
    {
    case CheckBoxState::State_Checked:
        D2D1_MATRIX_3X2_F matrix;
        m_pRenderTarget->GetTransform(&matrix);
        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(draw_rect.left, draw_rect.bottom) * matrix);
        m_pRenderTarget->FillGeometry(m_pCheckedGeometry, m_pBrush);
        m_pRenderTarget->SetTransform(&matrix);
        break;
    case CheckBoxState::State_Indeterminate:
        // ������Χ
        draw_rect.left += m_szCheckBox.width * 0.2f;
        draw_rect.right -= m_szCheckBox.width * 0.2f;
        draw_rect.top += m_szCheckBox.height * 0.2f;
        draw_rect.bottom -= m_szCheckBox.height * 0.2f;
        // ��Ⱦ��
        m_pRenderTarget->FillRectangle(draw_rect, m_pBrush);
        break;
    }
    // �����ı���Χ +
    this->show_zone.left += m_szCheckBox.width;
    // �̻��ı�
    Super::Render();
    // �����ı���Χ -
    this->show_zone.left -= m_szCheckBox.width;
    return S_OK;
}


// UICheckBox ���캯��
LongUI::UICheckBox::UICheckBox(pugi::xml_node node) noexcept: Super(node) {
    // �� symbol
    auto format = UIManager.GetTextFormat(LongUIDefaultTextFormatIndex);
    char32_t chars = U'��';
    CUIManager::CreateTextPathGeometry(
        &chars, 1,
        format,
        UIManager_D2DFactory,
        nullptr,
        &m_pCheckedGeometry
        );
    ::SafeRelease(format);
}

// UICheckBox ��������
LongUI::UICheckBox::~UICheckBox() noexcept {
    ::SafeRelease(m_pCheckedGeometry);
    ::SafeRelease(m_pBrush);
}


// UICheckBox::CreateControl ����
LongUI::UIControl* LongUI::UICheckBox::CreateControl(pugi::xml_node node) noexcept {
    if (!node) {
        UIManager << DL_Warning << L"node null" << LongUI::endl;
    }
    // ����ռ�
    auto pControl = LongUI::UIControl::AllocRealControl<LongUI::UICheckBox>(
        node,
        [=](void* p) noexcept { new(p) UICheckBox(node);}
    );
    if (!pControl) {
        UIManager << DL_Error << L"alloc null" << LongUI::endl;
    }
    return pControl;
}



// do event �¼�����
bool LongUIMethodCall LongUI::UICheckBox::DoEvent(LongUI::EventArgument& arg) noexcept {
    D2D1_COLOR_F* color = nullptr;
    if (arg.sender) {
        switch (arg.event)
        {
        case LongUI::Event::Event_FindControl:
            if (IsPointInRect(this->show_zone, arg.pt)) {
                arg.ctrl = this;
            }
            __fallthrough;
        case LongUI::Event::Event_SetFocus:
            __fallthrough;
        case LongUI::Event::Event_KillFocus:
            return true;
        case LongUI::Event::Event_MouseEnter:
            m_pWindow->now_cursor = m_hCursorHand;
            break;
        case LongUI::Event::Event_MouseLeave:
            m_pWindow->now_cursor = m_pWindow->default_cursor;
            break;
        }
    }
    else {
        auto new_arg = arg;
        switch (arg.msg)
        {
        case WM_LBUTTONUP:
            // ��Ч
            if (IsPointInRect(this->show_zone, arg.pt)) {
                // ���flag
                if (this->flags & Flag_CheckBox_WithIndeterminate) {
                    if (this->state == CheckBoxState::State_UnChecked) {
                        force_cast(this->state) = CheckBoxState::State_Checked;
                    }
                    else if (this->state == CheckBoxState::State_Checked) {
                        force_cast(this->state) = CheckBoxState::State_Indeterminate;
                    }
                    else {
                        force_cast(this->state) = CheckBoxState::State_UnChecked;
                    }
                }
                else {
                    force_cast(this->state) = static_cast<decltype(this->state)>
                        (!static_cast<uint32_t>(this->state));
                }
                m_pWindow->Invalidate(this);
            }
            break;
        }
    }
    return Super::DoEvent(arg);
}

// recreate �ؽ�
HRESULT LongUIMethodCall LongUI::UICheckBox::Recreate(LongUIRenderTarget* newRT) noexcept {
    ::SafeRelease(m_pBrush);
    m_pBrush = UIManager.GetBrush(LongUIDefaultTextFormatIndex);
    // ���ദ��
    return Super::Recreate(newRT);
}

// �رտؼ�
void LongUIMethodCall LongUI::UICheckBox::Close() noexcept {
    delete this;
}