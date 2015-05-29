#pragma once
/**
* Copyright (c) 2014-2015 dustpg   mailto:dustpg@gmail.com
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use,
* copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following
* conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/

// LongUI namespace
namespace LongUI{
    // ��ѡ��״̬
    // the disabled state is one lowest bit of state (state & 1)
    // "CheckBoxState" is state right shift a bit (state>>1)
    enum class CheckBoxState : uint32_t {
        // δ֪
        State_Unknown = ControlStatus::Status_Disabled,
        // δѡ��
        State_UnChecked = ControlStatus::Status_Normal,
        // ��ȷ��
        State_Indeterminate = ControlStatus::Status_Hover,
        // ѡ��
        State_Checked = ControlStatus::Status_Pushed,
    };
    // default CheckBox control Ĭ�ϸ�ѡ��ؼ�
    class LongUIAPI UICheckBox final : public UILabel {
        // ��������
        using Super = UILabel ;
    public:
        // Render ��Ⱦ --- ���ڵ�һλ!
        virtual auto LongUIMethodCall Render() noexcept->HRESULT override;
        // do event �¼�����
        virtual bool LongUIMethodCall DoEvent(LongUI::EventArgument&) noexcept override;
        // Ԥ��Ⱦ
        virtual void LongUIMethodCall PreRender() noexcept override {};
        // recreate �ؽ�
        virtual auto LongUIMethodCall Recreate(LongUIRenderTarget*) noexcept->HRESULT override;
        // close this control �رտؼ�
        virtual void LongUIMethodCall Close() noexcept override;
    public:
        // create ����
        static UIControl* WINAPI CreateControl(pugi::xml_node) noexcept;
    protected:
        // constructor ���캯��
        UICheckBox(pugi::xml_node) noexcept;
        // destructor ��������
        ~UICheckBox() noexcept;
        // deleted function
        UICheckBox(const UICheckBox&) = delete;
    protected:
        // default brush
        ID2D1Brush*             m_pBrush = nullptr;
        // geometry for "��"
        ID2D1PathGeometry*      m_pCheckedGeometry = nullptr;
        // hand cursor
        HCURSOR                 m_hCursorHand = ::LoadCursorW(nullptr, IDC_HAND);
        // check box's box size
        D2D1_SIZE_F             m_szCheckBox = D2D1::SizeF(LongUIDefaultCheckBoxWidth, LongUIDefaultCheckBoxWidth);
    public:
        // set new state
        void SetNewState(CheckBoxState new_result){ force_cast(state) = new_result; m_pWindow->Invalidate(this); }
        // now state
        CheckBoxState const   state = CheckBoxState::State_UnChecked;
    };
}