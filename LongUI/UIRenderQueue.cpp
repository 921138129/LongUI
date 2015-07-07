#include "LongUI.h"


// ��Ⱦ���� ���캯��
LongUI::CUIRenderQueue::CUIRenderQueue(UIWindow* window) noexcept {
    m_unitLike.length = 0; m_unitLike.window = window;
    // get window
    // static_cast<UIWindow*>(m_unitLike.window)
}

// ��Ⱦ���� ��������
LongUI::CUIRenderQueue::~CUIRenderQueue() noexcept {

}

// ����
void LongUI::CUIRenderQueue::Reset(uint32_t freq) noexcept {
    // һ���Ͳ�����
    if (m_dwDisplayFrequency == freq) return;
    // �޸�
    m_dwDisplayFrequency = freq;
    // ����
    CUIRenderQueue::UNIT* data = nullptr;
    if (freq) {
        data = LongUI::CtrlAllocT(data, LongUIPlanRenderingTotalTime * freq);
        if (data) {
            for (auto i = 0u; i < LongUIPlanRenderingTotalTime * freq; ++i) {
                data[i].length = 0;
            }
        }
    }
    // TODO: ���ת��
    if (m_pUnitsDataBegin && data) {

    }
    // �ͷ�
    if (m_pUnitsDataBegin) LongUI::CtrlFree(m_pUnitsDataBegin);
    // ת��
    if (data) {
        m_pUnitsDataBegin = data;
        m_pUnitsDataEnd = data + LongUIPlanRenderingTotalTime * freq;
        // XXX
        m_pCurrentUnit = data;
    }
    // ��ʼ��Ⱦ
    m_dwStartTime = ::timeGetTime();
}

// ++ ������
void LongUI::CUIRenderQueue::operator++() noexcept {
    // ��Ⱦ����ģʽ
    if (m_pCurrentUnit) {
        ++m_pCurrentUnit;
        if (m_pCurrentUnit == m_pUnitsDataEnd) {
            m_pCurrentUnit = m_pUnitsDataBegin;
            // ������
            register auto time = m_dwStartTime;
            m_dwStartTime = ::timeGetTime();
            time = m_dwStartTime - time;
            UIManager << DL_Hint << "Time Deviation: "
                << long(time) - long(LongUIPlanRenderingTotalTime * 1000)
                << " ms" << endl;
            // TODO: ʱ��У��
        }
    }
    // ��ģʽ
    else {
        assert(!"error");
    }
}

// �ƻ���Ⱦ
void LongUI::CUIRenderQueue::PlanToRender(float wait, float render, UIControl* ctrl) noexcept {
    // ����ˢ��
    if (render != 0.0f) render += 0.1f;
    assert((wait + render) < float(LongUIPlanRenderingTotalTime) && "time overflow");
    // ��ǰ����
    auto window = m_unitLike.window;
    // ���õ�Ԫ
    auto set_unit = [window](UNIT* unit, UIControl* ctrl) noexcept {
        // �Ѿ�ȫ��Ⱦ�˾Ͳ���
        if (unit->length && unit->units[0] == window) {
            return;
        }
        // ��Ԫ���˾�����Ϊȫ��Ⱦ
        if (unit->length == LongUIDirtyControlSize) {
            unit->length = 1;
            unit->units[0] = window;
            return;
        }
        // ��ȡ��������
        auto get_real_render_control = [window](UIControl* control) noexcept {
            // ��ȡ����
            while (control != window) {
                if (control->flags & Flag_RenderParent) control = control->parent;
                else break;
            }
            return control;
        };
        // ��Ⱦ����Ҳ����Ϊȫ��Ⱦ
        ctrl = get_real_render_control(ctrl);
        if (ctrl == window) {
            unit->length = 1;
            unit->units[0] = window;
            return;
        }
#if 0
        // ����Ƿ��ڵ�Ԫ����
        register bool not_in = true;
        for (auto unit_ctrl = unit->units; unit_ctrl < unit->units + unit->length; ++unit_ctrl) {
            if (*unit_ctrl == ctrl) {
                not_in = false;
                break;
            }
        }
        // ���ڵ�Ԫ����ͼ���
        if (not_in) {
            unit->units[unit->length] = ctrl;
            ++unit->length;
        }
#else
        // ���ڵ�Ԫ����ͼ���
        if (std::none_of(unit->units, unit->units + unit->length, [ctrl](UIControl* unit) {
            return unit == ctrl;
        })) {
            unit->units[unit->length] = ctrl;
            ++unit->length;
        }
#endif
    };
    // ��Ⱦ����ģʽ
    if (m_pCurrentUnit) {
        // ʱ��Ƭ����
        auto frame_offset = long(wait * float(m_dwDisplayFrequency));
        auto frame_count = long(render * float(m_dwDisplayFrequency)) + 1;
        auto start = m_pCurrentUnit + frame_offset;
        for (long i = 0; i < frame_count; ++i) {
            if (start >= m_pUnitsDataEnd) {
                start -= LongUIPlanRenderingTotalTime * m_dwDisplayFrequency;
            }
            set_unit(start, ctrl);
            ++start;
        }
    }
    // ��ģʽ
    else {
        assert(!"error");
    }
}