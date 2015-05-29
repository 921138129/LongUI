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
    // UI String -- compatible with std library string interface(part of) but static
    class LongUIAPI CUIString {
    public:
        // Ĭ�Ϲ��캯��
        LongUIInline CUIString() noexcept { *m_aDataStatic = 0; }
        // �ַ������캯��
        CUIString(const wchar_t* str, uint32_t l = 0) noexcept { this->Set(str, l); }
        // ��������
        ~CUIString() noexcept;
        // ���ƹ��캯��
        CUIString(const CUIString&) noexcept;
        // move���캯��
        CUIString(CUIString&&) noexcept;
    public:
        // ����
        void __fastcall Set(const wchar_t* str, uint32_t = 0) noexcept;
        // ת��Ϊ wchar_t*
        LongUIInline  operator const wchar_t*()const noexcept{ return m_pString; }
    public: // std::string compatibled interface/method
        // get data
        LongUIInline const auto* data() const noexcept{ return m_pString; }
        // c style string
        LongUIInline const auto* c_str() const noexcept{ return m_pString; }
        // get dlengthata
        LongUIInline auto length() const noexcept{ return m_cLength; }
        // get size
        LongUIInline auto size() const noexcept{ return m_cLength; }
        // = ����
        const CUIString& __fastcall operator=(const wchar_t* s) noexcept{ this->Set(s); }
        // += ����
        //const CUIString& __fastcall operator+=(const wchar_t*) noexcept;
        // == ����
        bool __fastcall operator == (const wchar_t* str) const noexcept{ return (::wcscmp(m_pString, str) == 0); };
        // != ����
        bool __fastcall operator != (const wchar_t* str) const noexcept{ return (::wcscmp(m_pString, str) != 0); };
        // <= ����
        bool __fastcall operator <= (const wchar_t* str) const noexcept{ return (::wcscmp(m_pString, str) <= 0); };
        // < ����
        bool __fastcall operator <  (const wchar_t* str) const noexcept{ return (::wcscmp(m_pString, str) < 0); };
        // >= ����
        bool __fastcall operator >= (const wchar_t* str) const noexcept{ return (::wcscmp(m_pString, str) >= 0); };
        // > ����
        bool __fastcall operator >  (const wchar_t* str) const noexcept{ return (::wcscmp(m_pString, str) > 0); };
    private:
        // �ַ���
        wchar_t*            m_pString = m_aDataStatic;
        // �ַ�������
        uint32_t            m_cLength = 0;
        // ����������
        uint32_t            m_cBufferLength = LongUIStringLength;
        // ��̬����
        wchar_t             m_aDataStatic[LongUIStringLength];
    };
}