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


// longui namespace
namespace LongUI {
    // UIDropSource �� 
    class CUIDropSource final: public ComBase<QiListSelf<IUnknown, QiList<IDropSource>>> {
    public:
        // ��������
        static CUIDropSource* New() noexcept;
    private:
        // ���캯��
        CUIDropSource() noexcept {};
        // ��������
        ~CUIDropSource() noexcept {};
        // ��ֹ��̬����
        void* operator new(size_t) = delete;
        // ��ֹ��̬����
        void* operator new[](size_t) = delete;
        // ��ֹ��̬����
        void operator delete(void* p) noexcept { LongUISmallFree(p); };
        // ��ֹ��̬����
        void operator delete[](void*) = delete;
    public: // IDropSource �ӿ� ʵ��
        // IDropSource::QueryContinueDrag ʵ��
        HRESULT STDMETHODCALLTYPE   QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) noexcept override;
        // IDropSource::GiveFeedback ʵ��
        HRESULT STDMETHODCALLTYPE   GiveFeedback(DWORD dwEffect) noexcept override;
    private:
    };
}