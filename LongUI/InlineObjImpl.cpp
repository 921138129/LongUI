#include "LongUI.h"


// CUIRubyCharacter ���캯��
LongUI::CUIRubyCharacter::CUIRubyCharacter(const CtorContext& ctx) 
noexcept : Super(UIInlineObject::Type_Ruby) {

}

// CUIRubyCharacter ��������
LongUI::CUIRubyCharacter::~CUIRubyCharacter() noexcept {
    ::SafeRelease(m_pBaseLayout);
    ::SafeRelease(m_pRubyLayout);
}

// CUIRubyCharacter �̻�
auto LongUI::CUIRubyCharacter::Draw(
    void* clientDrawingContext,
    IDWriteTextRenderer* renderer,
    FLOAT originX,
    FLOAT originY,
    BOOL isSideways,
    BOOL isRightToLeft,
    IUnknown* clientDrawingEffect
    ) noexcept ->HRESULT {
    return E_NOTIMPL;
}

// ��ȡ Metrics
auto LongUI::CUIRubyCharacter::GetMetrics(
    DWRITE_INLINE_OBJECT_METRICS* metrics) noexcept ->HRESULT {
    DWRITE_INLINE_OBJECT_METRICS inlineMetrics = { 0 };
    inlineMetrics;

    *metrics = inlineMetrics;
    return S_OK;
}


// ��ȡ Overhang Metrics
auto LongUI::CUIRubyCharacter::GetOverhangMetrics(
     DWRITE_OVERHANG_METRICS* overhangs) noexcept ->HRESULT {
    overhangs->left = 0;
    overhangs->top = 0;
    overhangs->right = 0;
    overhangs->bottom = 0;
    return S_OK;
}


auto LongUI::CUIRubyCharacter::GetBreakConditions(
    DWRITE_BREAK_CONDITION* breakConditionBefore,
    DWRITE_BREAK_CONDITION* breakConditionAfter
    ) noexcept ->HRESULT {
    *breakConditionBefore = DWRITE_BREAK_CONDITION_NEUTRAL;
    *breakConditionAfter = DWRITE_BREAK_CONDITION_NEUTRAL;
    return S_OK;
}
