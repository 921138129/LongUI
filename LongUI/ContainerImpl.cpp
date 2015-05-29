
#include "LongUI.h"




// -------------------------- UIContainer -------------------------
// UIContainer ���캯��
LongUI::UIContainer::UIContainer(pugi::xml_node node) noexcept :
Super(node),
scrollbar_h(UIScrollBar::CreateDesc(nullptr, ScrollBarType::Type_Horizontal), this),
scrollbar_v(UIScrollBar::CreateDesc(nullptr, ScrollBarType::Type_Vertical), this) {
    assert(node && "bad argument.");
    uint32_t flag = this->flags | Flag_UIContainer;
    if ((this->flags & Flag_RenderParent) || node.attribute("rendercd").as_bool(false)) {
        flag |= LongUI::Flag_Container_AlwaysRenderChildrenDirectly;
    }
    force_cast(this->flags) = LongUIFlag(flag);
}

// UIContainer ��������
LongUI::UIContainer::~UIContainer() noexcept {
    for (auto itr = this->begin(); itr != this->end(); ) {
        auto itr_next = itr; ++itr_next;
        itr->Close();
        itr = itr_next;
    }
}

// �������
void LongUI::UIContainer::AfterInsert(UIControl* child) noexcept {
    assert(child && "bad argument");
    // check flag
    if (this->flags & Flag_Container_AlwaysRenderChildrenDirectly) {
        force_cast(child->flags) = LongUIFlag(child->flags | Flag_RenderParent);
    }
    // recreate resource
    if (m_pRenderTarget) child->Recreate(m_pRenderTarget);
    // set parent
    force_cast(child->parent) = this;
    // change draw zone
    child->DrawPosChanged();
    child->DrawSizeChanged();
    this->DrawSizeChanged();
}

// ���²���
void LongUIMethodCall LongUI::UIContainer::RefreshChildLayout(bool refresh_scroll) noexcept {
    if (!refresh_scroll) return;
    // �����
    this->scrollbar_h.show_zone.left = 0.f;
    this->scrollbar_h.show_zone.top = this->show_zone.height - this->scrollbar_h.desc.size;
    this->scrollbar_h.show_zone.width = this->show_zone.width;
    this->scrollbar_h.show_zone.height = this->scrollbar_h.desc.size;
    this->scrollbar_h.draw_zone = this->scrollbar_h.show_zone;
    this->scrollbar_h.Refresh();
    // ���߶�
    this->scrollbar_v.show_zone.left = this->show_zone.width - this->scrollbar_v.desc.size;
    this->scrollbar_v.show_zone.top = 0.f;
    this->scrollbar_v.show_zone.width = this->scrollbar_v.desc.size;
    this->scrollbar_v.show_zone.height = this->show_zone.height;
    this->scrollbar_v.draw_zone = this->scrollbar_v.show_zone;
    this->scrollbar_v.Refresh();
    // �ڶ��μ��
    if (this->scrollbar_h) {

    }
    if (this->scrollbar_v) {

    }
}

// do event �¼�����
bool LongUIMethodCall LongUI::UIContainer::DoEvent(LongUI::EventArgument& arg) noexcept {
    // TODO: ����EventArgument��Ϊconst
    bool done = false;
    // ת������
    auto pt_old = arg.pt;
    auto pt4self = LongUI::TransformPointInverse(this->transform, pt_old);
    arg.pt = pt4self;
    // �������¼�
    if (arg.sender) {
        switch (arg.event)
        {
        case LongUI::Event::Event_FindControl:
            // ��������
            if (scrollbar_h) {

            }
            if (scrollbar_v) {

            }
            // ����ӿؼ�
            if (!done) {
                // XXX: �Ż�
                for (auto ctrl : (*this)) {
                    if (IsPointInRect(ctrl->show_zone, pt4self)) {
                        done = ctrl->DoEvent(arg);
                        break;
                    }
                }
            }
            done = true;
            break;
        case LongUI::Event::Event_FinishedTreeBuliding:
            // ������ɿռ�������
            for (auto ctrl : (*this)) {
                ctrl->DoEvent(arg);
            }
            done = true;
            break;
        }
    }
    // �����
    arg.pt = pt_old;
    return done;
}

// UIContainer ��Ⱦ����
auto LongUIMethodCall LongUI::UIContainer::Render() noexcept -> HRESULT {
    // ���
    if (m_bDrawPosChanged || m_bDrawSizeChanged) {
        this->transform = D2D1::Matrix3x2F::Translation(
            this->show_zone.left - this->margin_rect.left,
            this->show_zone.top - this->margin_rect.top
            );
    }
    // ����ת��
    D2D1_MATRIX_3X2_F old_transform;
    m_pRenderTarget->GetTransform(&old_transform);
    // ��������ת��
    this->world = this->transform * old_transform;
    m_pRenderTarget->SetTransform(&this->world);
    // ��Ⱦ�����Ӳ���
    for (auto ctrl : (*this)) {
        D2D1_RECT_F clip_rect;
        clip_rect.left = ctrl->show_zone.left - ctrl->margin_rect.left;
        clip_rect.top = ctrl->show_zone.top - ctrl->margin_rect.top;
        clip_rect.right = clip_rect.left + ctrl->show_zone.width + ctrl->margin_rect.right;
        clip_rect.bottom = clip_rect.top + ctrl->show_zone.height + ctrl->margin_rect.bottom;
        m_pRenderTarget->PushAxisAlignedClip(&clip_rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        ctrl->Render();
        m_pRenderTarget->PopAxisAlignedClip();
    }
    // ��Ⱦ������
    if (this->scrollbar_h) {
        this->scrollbar_h.Render();
    }
    if (this->scrollbar_v) {
        this->scrollbar_v.Render();
    }
    // ����
    auto hr = Super::Render();
    // ����ת��
    m_pRenderTarget->SetTransform(&old_transform);
    return hr;
}

// UIContainer �ؽ�
auto LongUIMethodCall LongUI::UIContainer::Recreate(
    LongUIRenderTarget* newRT) noexcept ->HRESULT {
    this->scrollbar_h.Recreate(newRT);
    this->scrollbar_v.Recreate(newRT);
    return Super::Recreate(newRT);
}

// ��ȡָ���ؼ�
auto LongUIMethodCall LongUI::UIContainer::
at(uint32_t i) const noexcept -> UIControl * {
    // ���ܾ���
    UIManager << DL_Warning << L"Performance Warning!"
        L"random accessig is not fine for list" << LongUI::endl;
    // ��鷶Χ
    if (i >= this->size()) {
        UIManager << DL_Error << L"out of range" << LongUI::endl;
        return nullptr;
    }
    // ֻ��һ��?
    if (this->size() == 1) return m_pHead;
    // ǰ�벿��?
    UIControl * control;
    if (i < this->size() / 2) {
        control = m_pHead;
        while (i) {
            assert(control && "null pointer");
            control = control->next;
            --i;
        }
    }
    // ��벿��?
    else {
        control = m_pTail;
        i = this->size() - i - 1;
        while (i) {
            assert(control && "null pointer");
            control = control->prev;
            --i;
        }
    }
    return control;
}
// ����ؼ�
void LongUIMethodCall LongUI::UIContainer::
insert(Iterator itr, UIControl* ctrl) noexcept {
    assert(ctrl && "bad arguments");
    if (ctrl->prev) {
        UIManager << DL_Warning
            << L"the 'prev' attr of the control: ["
            << ctrl->GetNameStr()
            << "] that to insert is not null"
            << LongUI::endl;
    }
    if (ctrl->next) {
        UIManager << DL_Warning
            << L"the 'next' attr of the control: ["
            << ctrl->GetNameStr()
            << "] that to insert is not null"
            << LongUI::endl;
    }
    // ����β��?
    if (itr == this->end()) {
        // ����
        force_cast(ctrl->prev) = m_pTail;
        // ��β?
        if(m_pTail) force_cast(m_pTail->next) = ctrl;
        // ��ͷ?
        if (!m_pHead) m_pHead = ctrl;
        // ����β
        m_pTail = ctrl;
    }
    else {
        // ctrl->next = itr;
        // ctrl->prev = ǰ��;
        force_cast(ctrl->next) = itr.Ptr();
        force_cast(ctrl->prev) = itr->prev;
        // ǰ��->next = ctrl
        // itr->prev = ctrl
        if (itr->prev) {
            force_cast(itr->prev) = ctrl;
        }
        force_cast(itr->prev) = ctrl;
    }
    ++m_cChildrenCount;
    // ���֮��Ĵ���
    this->AfterInsert(ctrl);
}


// �Ƴ��ؼ�
bool LongUIMethodCall LongUI::UIContainer::remove(Iterator itr) noexcept {
    // ����Ƿ����ڱ�����
#ifdef _DEBUG
    bool ok = false;
    for (auto i : (*this)) {
        if (itr == i) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        UIManager << DL_Error << "control:[" << itr->GetNameStr()
            << "] not in this container: " << this->GetNameStr() << LongUI::endl;
        return false;
    }
#endif
    // ����ǰ��ڵ�
    register auto prev = itr->prev;
    register auto next = itr->next;
    // ���
    if (prev) {
        force_cast(prev->next) = next;
    }
    // �ײ�
    else {
        m_pHead = next;
    }
    // ���
    if (next) {
        force_cast(next->prev) = prev;
    }
    // β��
    else {
        m_pTail = prev;
    }
    // ����
    force_cast(itr->prev) = nullptr;
    force_cast(itr->next) = nullptr;
    --m_cChildrenCount;
    // �޸�
    this->DrawSizeChanged();
    return true;
}


// -------------------------- UIVerticalLayout -------------------------
// UIVerticalLayout ����
auto LongUI::UIVerticalLayout::CreateControl(pugi::xml_node node) noexcept ->UIControl* {
    if (!node) {
        UIManager << DL_Warning << L"node null" << LongUI::endl;
    }
    // ����ռ�
    auto pControl = LongUI::UIControl::AllocRealControl<LongUI::UIVerticalLayout>(
        node,
        [=](void* p) noexcept { new(p) UIVerticalLayout(node);}
    );
    if (!pControl) {
        UIManager << DL_Error << L"alloc null" << LongUI::endl;
    }
    return pControl;
}

// �����ӿؼ�����
void LongUIMethodCall LongUI::UIVerticalLayout::RefreshChildLayout(bool refresh_scroll) noexcept {
    // �����㷨:
    // 1. ȥ�������ؼ�Ӱ��
    // 2. һ�α���, ���ָ���߶ȵĿؼ�, ��������߶�/���
    // 3. ����ʵ�ʸ߶�/���, �޸�show_zone, ���¹�����, ������С���Ķ�
    float base_width = 0.f, base_height = 0.f;
    float counter = 0.0f;
    // ��һ��
    for (auto ctrl : (*this)) {
        // �Ǹ���ؼ�
        if (!(ctrl->flags & Flag_Floating)) {
            // ��ȹ̶�?
            if (ctrl->flags & Flag_WidthFixed) {
                base_width = std::max(base_width, ctrl->GetTakingUpWidth());
            }
            // �߶ȹ̶�?
            if (ctrl->flags & Flag_HeightFixed) {
                base_height += ctrl->GetTakingUpHeight();
            }
            // δָ���߶�?
            else {
                counter += 1.f;
            }
        }
    }
    // ����
    base_width = std::max(base_width, this->show_zone.width);
    // ��ֱ������?
    if (this->scrollbar_v) {
        base_width -= this->scrollbar_v.desc.size;
    }    // ˮƽ������?
    if (this->scrollbar_h) {
        base_height -= this->scrollbar_h.desc.size;
    }
    // �߶Ȳ���
    float height_step = counter > 0.f ? (this->show_zone.height - base_height) / counter : 0.f;
    float position_y = 0.f;
    // �ڶ���
    for (auto ctrl : (*this)) {
        // �����
        if (ctrl->flags & Flag_Floating) continue;
        ctrl->show_zone.left = 0.f;
        // ���ÿؼ����
        if (!(ctrl->flags & Flag_WidthFixed)) {
            register auto old_width = ctrl->show_zone.width;
            ctrl->show_zone.width = base_width - ctrl->margin_rect.left - ctrl->margin_rect.right;
        }
        // ���ÿؼ��߶�
        if (!(ctrl->flags & Flag_HeightFixed)) {
            ctrl->show_zone.height = height_step - ctrl->margin_rect.top - ctrl->margin_rect.bottom;
        }
        // �޸�
        ctrl->DrawSizeChanged();
        ctrl->DrawPosChanged();
        ctrl->show_zone.top = position_y;
        position_y += ctrl->GetTakingUpHeight();
        // �ӿؼ�Ҳ���������������
        if (ctrl->flags & Flag_UIContainer) {
            static_cast<UIContainer*>(ctrl)->UpdateChildLayout();
        }
    }
    // �޸�
    force_cast(this->end_of_right) = base_width;
    force_cast(this->end_of_bottom) = position_y;
    // ����
    return Super::RefreshChildLayout(refresh_scroll);
}


// UIVerticalLayout �ؽ�
auto LongUIMethodCall LongUI::UIVerticalLayout::Recreate(LongUIRenderTarget* newRT) noexcept ->HRESULT {
    HRESULT hr = S_OK;
    for (auto ctrl : (*this)) {
        hr = ctrl->Recreate(newRT);
        AssertHR(hr);
    }
    return Super::Recreate(newRT);
}

// UIVerticalLayout �رտؼ�
void LongUIMethodCall LongUI::UIVerticalLayout::Close() noexcept {
    delete this;
}

// -------------------------- UIHorizontalLayout -------------------------
// UIHorizontalLayout ����
auto LongUI::UIHorizontalLayout::CreateControl(pugi::xml_node node) noexcept ->UIControl* {
    if (!node) {
        UIManager << DL_Warning << L"node null" << LongUI::endl;
    }
    // ����ռ�
    auto pControl = LongUI::UIControl::AllocRealControl<LongUI::UIHorizontalLayout>(
        node,
        [=](void* p) noexcept { new(p) UIHorizontalLayout(node);}
    );
    if (!pControl) {
        UIManager << DL_Error << L"alloc null" << LongUI::endl;
    }
    return pControl;
}


// �����ӿؼ�����
void LongUIMethodCall LongUI::UIHorizontalLayout::RefreshChildLayout(bool refresh_scroll) noexcept {
    // �����㷨:
    // 1. ȥ�������ؼ�Ӱ��
    // 2. һ�α���, ���ָ���߶ȵĿؼ�, ��������߶�/���
    // 3. ����ʵ�ʸ߶�/���, �޸�show_zone, ���¹�����, ������С���Ķ�
    float base_width = 0.f, base_height = 0.f;
    float counter = 0.0f;
    // ��һ��
    for (auto ctrl : (*this)) {
        // �Ǹ���ؼ�
        if (!(ctrl->flags & Flag_Floating)) {
            // �߶ȹ̶�?
            if (ctrl->flags & Flag_HeightFixed) {
                base_height = std::max(base_height, ctrl->GetTakingUpHeight());
            }
            // ��ȹ̶�?
            if (ctrl->flags & Flag_WidthFixed) {
                base_width += ctrl->GetTakingUpWidth();
            }
            // δָ�����?
            else {
                counter += 1.f;
            }
        }
    }
    // ����
    base_height = std::max(base_height, this->show_zone.height);
    // ��ֱ������?
    if (this->scrollbar_v) {
        base_width -= this->scrollbar_v.desc.size;
    }    // ˮƽ������?
    if (this->scrollbar_h) {
        base_height -= this->scrollbar_h.desc.size;
    }
    // ��Ȳ���
    float width_step = counter > 0.f ? (this->show_zone.width - base_width) / counter : 0.f;
    float position_x = 0.f;
    // �ڶ���
    for (auto ctrl : (*this)) {
        // �����
        if (ctrl->flags & Flag_Floating) continue;
        ctrl->show_zone.top = 0.f;
        // ���ÿؼ��߶�
        if (!(ctrl->flags & Flag_HeightFixed)) {
            register auto old_height = ctrl->show_zone.height;
            ctrl->show_zone.height = base_height - ctrl->margin_rect.left - ctrl->margin_rect.right;
        }
        // ���ÿؼ����
        if (!(ctrl->flags & Flag_WidthFixed)) {
            ctrl->show_zone.width = width_step - ctrl->margin_rect.top - ctrl->margin_rect.bottom;
        }
        // �޸�
        ctrl->DrawSizeChanged();
        ctrl->DrawPosChanged();
        ctrl->show_zone.left = position_x;
        position_x += ctrl->GetTakingUpWidth();
        // �ӿؼ�Ҳ���������������
        if (ctrl->flags & Flag_UIContainer) {
            static_cast<UIContainer*>(ctrl)->UpdateChildLayout();
        }
    }
    // �޸�
    force_cast(this->end_of_right) = position_x;
    force_cast(this->end_of_bottom) = base_height;
    // ����
    return Super::RefreshChildLayout(refresh_scroll);
}

// UIHorizontalLayout �ؽ�
auto LongUIMethodCall LongUI::UIHorizontalLayout::Recreate(LongUIRenderTarget* newRT) noexcept ->HRESULT {
    HRESULT hr = S_OK;
    if (newRT) {
        for (auto ctrl : (*this)) {
            hr = ctrl->Recreate(newRT);
            AssertHR(hr);
        }
    }
    return Super::Recreate(newRT);
}

// UIHorizontalLayout �رտؼ�
void LongUIMethodCall LongUI::UIHorizontalLayout::Close() noexcept {
    delete this;
}

