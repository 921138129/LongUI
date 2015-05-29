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
    // CUIFileLoader��
    class CUIFileLoader{
    private:
        // ˽�л����캯��
        CUIFileLoader() noexcept{}
        // ˽�л���������
        ~CUIFileLoader() noexcept{
            if (m_pData){
                free(m_pData);
                m_pData = nullptr;
            }
        }
    public:
        // ��ȡ�ļ�
        bool LongUIMethodCall ReadFile(WCHAR* file_name) noexcept;
        // ��ȡ����ָ��
        auto LongUIMethodCall GetData() const  noexcept{ return m_pData; }
        // ��ȡ���ݳ���
        auto LongUIMethodCall GetLength()const noexcept{ return m_cLength; }
    private:
        // ����ָ��
        void*           m_pData = nullptr;
        // ���ݳ���
        size_t          m_cLength = 0;
        // ʵ�ʳ���
        size_t          m_cLengthReal = 0;
    public:
        // ���߳��̵߳���
        static CUIFileLoader s_instanceForMainThread;
    };
}
#define UIFileLoader (LongUI::CUIFileLoader::s_instanceForMainThread)