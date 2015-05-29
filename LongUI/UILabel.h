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
    // default label control Ĭ�ϱ�ǩ�ؼ�
    class LongUIAPI UILabel : public UIControl{
    private:
        // ��������
        using Super = UIControl ;
    public:
        // Render ��Ⱦ --- ���ڵ�һλ!
        virtual auto LongUIMethodCall Render() noexcept->HRESULT override;
        // do event �¼�����
        virtual bool LongUIMethodCall DoEvent(LongUI::EventArgument&) noexcept override;
        // Ԥ��Ⱦ
        virtual void LongUIMethodCall PreRender() noexcept override {};
        // recreate �ؽ�
        //virtual auto LongUIMethodCall Recreate(LongUIRenderTarget*) noexcept->HRESULT override;
        // close this control �رտؼ�
        virtual void LongUIMethodCall Close() noexcept override;
    public:
        // create ����
        static UIControl* WINAPI CreateControl(pugi::xml_node) noexcept;
        // control text �ؼ��ı�
        const auto LongUIMethodCall GetText() const noexcept{ return m_text.c_str(); }
        // set control text
        const void LongUIMethodCall SetText(const wchar_t* t) noexcept { m_text = t; }
        // set control text
        const void LongUIMethodCall SetText(const char* t) noexcept { m_text = t;  }
    protected:
        // constructor ���캯��
        UILabel(pugi::xml_node node) noexcept : Super(node), m_text(node) {}
        // destructor ��������
        ~UILabel() noexcept { }
        // deleted function
        UILabel(const UILabel&) = delete;
    protected:
        // the text of control
        UIText                  m_text;
    };
}