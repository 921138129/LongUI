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
    // ���ݴ���
    struct DATASTORAGE {
        FORMATETC formatEtc;
        STGMEDIUM stgMedium;
    };
    // UIDataObject ��: ʵ��IDataObject
    class CUIDataObject final  :public ComBase<QiListSelf<IUnknown, QiList<IDataObject>>> {
    public:
        // ��������
        static CUIDataObject* New() noexcept;
    private:
        // delete new operator
        void* operator new(size_t ) = delete;
        // delete new operator
        void* operator new[](size_t ) = delete;
        // delete operator
        void  operator delete(void* p) noexcept { LongUISmallFree(p); };
        // delete new operator
        void  operator delete[](void*) = delete;
        // ���캯��
        CUIDataObject() noexcept;
        // ɾ�����ƹ��캯��
        CUIDataObject(const CUIDataObject&) = delete;
        // ɾ��=
        CUIDataObject& operator =(const CUIDataObject&) = delete;
        // ��������
        ~CUIDataObject() noexcept;
    public:
        // ����Unicode
        HRESULT LongUIMethodCall SetUnicodeText(const wchar_t*, size_t =0) noexcept;
        // ����Unicode
        HRESULT LongUIMethodCall SetUnicodeText(HGLOBAL) noexcept;
    public: // IDataObject �ӿ� ʵ��
        // IDataObject::GetData ʵ��
        HRESULT STDMETHODCALLTYPE GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium) noexcept override;
        // IDataObject::GetDataHere ʵ��
        HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium) noexcept override;
        // IDataObject::QueryGetData ʵ��
        HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *pformatetc) noexcept override;
        // IDataObject::GetCanonicalFormatEtc ʵ��
        HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *pformatetcIn, FORMATETC *pformatetcOut) noexcept override;
        // IDataObject::SetData ʵ��
        HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) noexcept override;
        // IDataObject::EnumFormatEtc ʵ��
        HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) noexcept override;
        // IDataObject::DAdvise ʵ��
        HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSnk, DWORD *pdwConnection) noexcept override;
        // IDataObject::DUnadvise ʵ��
        HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) noexcept override;
        // IDataObject::EnumDAdvise ʵ��
        HRESULT STDMETHODCALLTYPE  EnumDAdvise(IEnumSTATDATA **ppenumAdvise) noexcept override;
    private:
        // ����ý������
        HRESULT CopyMedium(STGMEDIUM* pMedDest, STGMEDIUM* pMedSrc, FORMATETC* pFmtSrc) noexcept;
        // ����Blob
        HRESULT SetBlob(CLIPFORMAT cf, const void *pvBlob, UINT cbBlob) noexcept;
    private:
        // ��ǰ����
        DATASTORAGE             m_dataStorage;
    };
}