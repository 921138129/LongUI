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
namespace LongUI {
    // endl for longUI
    static struct EndL { } endl;
    // ui manager ui ������
    class LongUIAlignas LongUIAPI CUIManager {
    public: 
        // Windows Version
        enum WindowsVersion : size_t {
            // Win8,
            Style_Win8 = 0, 
            //  Win8.1
            Style_Win8_1,
            // Win10
            Style_Win10
        };
    public: // handle zone ������
        // initialize ��ʼ��
        auto LongUIMethodCall Initialize(IUIConfigure* =nullptr) noexcept->HRESULT;
        // uninitialize ����ʼ��
        void LongUIMethodCall UnInitialize() noexcept;
        // run ����
        void LongUIMethodCall Run() noexcept;
        // add "string to create funtion" map ��Ӻ���ӳ���ϵ
        auto LongUIMethodCall AddS2CPair(const wchar_t*, CreateControlFunction) noexcept->HRESULT;
        // get control by wchar_t pointer ��ȡ�ؼ�
        auto LongUIMethodCall FindControlW(const wchar_t*) noexcept ->UIControl*;
        // get control by CUIString ��ȡ�ؼ�
        auto LongUIMethodCall FindControl(const CUIString&) noexcept ->UIControl*;
        // ShowError with HRESULT code
        void LongUIMethodCall ShowError(HRESULT, const wchar_t* str_b =nullptr) noexcept;
    public: // ����
        // load bitmap
        static auto __cdecl LoadBitmapFromFile(
            LongUIRenderTarget*, IWICImagingFactory *, PCWSTR, UINT, UINT, ID2D1Bitmap1 **
            ) noexcept ->HRESULT;
        // get default LongUI imp IDWriteFontCollection
        static auto __cdecl CreateLongUIFontCollection(
            IDWriteFactory*, const wchar_t* filename=L"*.*tf", const wchar_t* folder=L"Fonts"
            ) noexcept->IDWriteFontCollection*;
        // create path-geometry from utf-32 char array using text format
        // fontface: (you can see <LongUI::UIScrollBar::UIScrollBar>)
        //          fontface == nullptr, ok but a bit slow
        //          *fontface == nullptr, ok, a bit slow, and out a IDWriteFontFace*, you can use it in next time(same format)
        //          *fontface != nullptr, ok
        static auto __cdecl CreateTextPathGeometry(
            IN const char32_t* utf32_string,  
            IN size_t string_length, 
            IN IDWriteTextFormat* format, 
            IN ID2D1Factory* factory,
            IN OUT OPTIONAL IDWriteFontFace** fontface,
            OUT ID2D1PathGeometry** geometry
            ) noexcept->HRESULT;
        // create mesh from geometry
        static auto __cdecl CreateMeshFromGeometry(ID2D1Geometry* geometry, ID2D1Mesh** mesh) noexcept->HRESULT;
        // format the text into core-mode with xml string: ��������
        static auto __cdecl XMLToCoreFormat(const char*, wchar_t*) noexcept->bool;
        // format the text into textlayout with format: ����C/C++
        static auto __cdecl FormatTextCore(FormatTextConfig&, const wchar_t*, ...) noexcept->IDWriteTextLayout*;
        // format the text into textlayout with format: ����C/C++
        static auto __cdecl FormatTextCore(FormatTextConfig&, const wchar_t*, va_list) noexcept->IDWriteTextLayout*;
        // create ui window via xml string ��������
        template<typename T = UIWindow>
        LongUINoinline auto CreateUIWindow(const char*, void* = nullptr, UIWindow* = nullptr) noexcept->T*;
        // create ui window via pugixml node ��������
        template<typename T = UIWindow>
        LongUINoinline auto CreateUIWindow(const pugi::xml_node, void* = nullptr, UIWindow* = nullptr) noexcept->T*;
    public: // UAC About
        // is run as admin?
        static bool WINAPI IsRunAsAdministrator() noexcept;
        // try to elevate now,  will lauch a new elevated instance and
        // exit this instance if success. be careful about your app if
        // only can be in one instance
        static bool WINAPI TryElevateUACNow(const wchar_t* parameters = nullptr, bool exit = true) noexcept;
    public: // inline ��
        // ShowError with string
        LongUIInline auto ShowError(const wchar_t * str, const wchar_t* str_b = nullptr) { this->configure->ShowError(str, str_b); }
        // ��ȡ�ı���Ⱦ�� GetXXX method will call AddRef if it is a COM object
        LongUIInline auto GetTextRenderer(int i) const { return ::SafeAcquire(m_apTextRenderer[i]); }
        // Exit �˳�
        LongUIInline auto Exit() { m_exitFlag = true; }
        // �ؽ���Դ
        LongUIInline auto RecreateResources() { this->discard_resources(); return this->create_resources(); }
    public: // ����ת����
        // ת��Ϊ LongUIRenderTarget
#define UIManager_RenderTaget (static_cast<ID2D1DeviceContext*>(UIManager))
        LongUIInline operator ID2D1DeviceContext*()const noexcept { return m_pd2dDeviceContext; };
        // ת��Ϊ DXGI Factory2
#define UIManager_DXGIFactory (static_cast<IDXGIFactory2*>(UIManager))
        LongUIInline operator IDXGIFactory2*()const noexcept { return m_pDxgiFactory; };
        // ת��Ϊ D3D11 Device
#define UIManager_D3DDevice  (static_cast<ID3D11Device*>(UIManager))
        LongUIInline operator ID3D11Device*()const noexcept { return m_pd3dDevice; };
        // ת��Ϊ D3D11 Device Context
#define UIManager_D3DContext (static_cast<ID3D11DeviceContext*>(UIManager))
        LongUIInline operator ID3D11DeviceContext*()const noexcept { return m_pd3dDeviceContext; };
        // ת��Ϊ D2D1 Device
#define UIManager_D2DDevice  (static_cast<ID2D1Device*>(UIManager))
        LongUIInline operator ID2D1Device*()const noexcept { return m_pd2dDevice; };
        // ת��Ϊ DXGI Device1
#define UIManager_DXGIDevice (static_cast<IDXGIDevice1*>(UIManager))
        LongUIInline operator IDXGIDevice1*()const noexcept { return m_pDxgiDevice; };
        // ת��Ϊ DXGI Adapter
#define UIManager_DXGIAdapter (static_cast<IDXGIAdapter*>(UIManager))
        LongUIInline operator IDXGIAdapter*()const noexcept { return m_pDxgiAdapter; };
        // ת��Ϊ DWrite Factory1
#define UIManager_DWriteFactory (static_cast<IDWriteFactory1*>(UIManager))
        LongUIInline operator IDWriteFactory1*()const noexcept { return m_pDWriteFactory; };
        // ת��Ϊ D2D Factory1
#define UIManager_D2DFactory (static_cast<ID2D1Factory1*>(UIManager))
        LongUIInline operator ID2D1Factory1*()const noexcept { return m_pd2dFactory; };
        // ת��Ϊ IWICImagingFactory2
#define UIManager_WICImagingFactory (static_cast<IWICImagingFactory2*>(UIManager))
        LongUIInline operator IWICImagingFactory2*()const noexcept { return m_pWICFactory; };
#ifdef LONGUI_VIDEO_IN_MF
        // ת��Ϊ  IMFDXGIDeviceManager
#   define UIManager_MFDXGIDeviceManager (static_cast<IMFDXGIDeviceManager*>(UIManager))
        LongUIInline operator IMFDXGIDeviceManager*()const noexcept { return m_pDXGIManager; };
        // ת��Ϊ  IMFMediaEngineClassFactory
#   define UIManager_MFMediaEngineClassFactory (static_cast<IMFMediaEngineClassFactory*>(UIManager))
        LongUIInline operator IMFMediaEngineClassFactory*()const noexcept { return m_pMediaEngineFactory; };
        // MF Dxgi�豸������
        IMFDXGIDeviceManager*           m_pDXGIManager = nullptr;
        // MF ý������
        IMFMediaEngineClassFactory*     m_pMediaEngineFactory = nullptr;
#endif
    public:
        // script �ű�
        IUIScript*           const      script = nullptr;
        // the handler
        InlineParamHandler   const      inline_handler = nullptr;
        // config
        IUIConfigure*        const      configure = nullptr;
        // windows version
        WindowsVersion       const      version = WindowsVersion::Style_Win8;
        // user context size �û������Ĵ�С
        size_t               const      user_context_size = 0;
        // now mouse states
        DIMOUSESTATE         const      now_mouse_states = DIMOUSESTATE();
    private:
        // last mouse states
        DIMOUSESTATE                    m_lastMouseStates;
        // DirectInput
        IDirectInput8W*                 m_pDirectInput = nullptr;
        // DInput Mouse
        IDirectInputDevice8W*           m_pDInputMouse = nullptr;
        // D2D ����
        ID2D1Factory1*                  m_pd2dFactory = nullptr;
        // WIC ����
        IWICImagingFactory2*            m_pWICFactory = nullptr;
        // DWrite����
        IDWriteFactory1*                m_pDWriteFactory = nullptr;
        // DWrite ���弯
        IDWriteFontCollection*          m_pFontCollection = nullptr;
        // D3D �豸
        ID3D11Device*                   m_pd3dDevice = nullptr;
        // D3D �豸������
        ID3D11DeviceContext*            m_pd3dDeviceContext = nullptr;
        // D2D �豸
        ID2D1Device*                    m_pd2dDevice = nullptr;
        // D2D �豸������
        ID2D1DeviceContext*             m_pd2dDeviceContext = nullptr;
        // DXGI ����
        IDXGIFactory2*                  m_pDxgiFactory = nullptr;
        // DXGI �豸
        IDXGIDevice1*                   m_pDxgiDevice = nullptr;
        // DXGI ������
        IDXGIAdapter*                   m_pDxgiAdapter = nullptr;
#ifdef _DEBUG
        // ���Զ���
        ID3D11Debug*                    m_pd3dDebug = nullptr;
#endif
        // �ı���Ⱦ��
        UIBasicTextRenderer*            m_apTextRenderer[LongUIMaxTextRenderer];
        // ��������Դ��ȡ��
        IUIBinaryResourceLoader*        m_pBinResLoader = nullptr;
        // default bitmap buffer
        uint8_t*                        m_pBitmap0Buffer = nullptr;
        // map ����ӳ��
        StringMap                       m_mapString2CreateFunction;
        // map ����ӳ��
        StringMap                       m_mapString2Control;
        // �����豸���Եȼ�
        D3D_FEATURE_LEVEL               m_featureLevel;
        // �˳��ź�
        BOOL                            m_exitFlag = false;
        // ��Ⱦ������
        uint32_t                        m_uTextRenderCount = 0;
        // TF �ֿ�
        BasicContainer                  m_textFormats;
        // ��ˢ����
        BasicContainer                  m_brushes;
        // ��������
        BasicContainer                  m_windows;
        // λͼ����
        BasicContainer                  m_bitmaps;
        // Metaͼ������
        BasicContainer                  m_metaicons;
        // Meta����
        LongUI::Vector<Meta>            m_metas;
        // ��������
        wchar_t                         m_szLocaleName[LOCALE_NAME_MAX_LENGTH / 4 * 4 + 4];
#ifdef LONGUI_WITH_DEFAULT_CONFIG
        // Ĭ������
        CUIDefaultConfigure             m_config;
#endif
        // ��ͨ�ı���Ⱦ��
        UINormalTextRender              m_normalTRenderer;
        // tinyxml2 ��Դ
        pugi::xml_document              m_docResource;
        // tinyxml2 ����
        pugi::xml_document              m_docWindow;
    public:
        // add window
        void LongUIMethodCall AddWindow(UIWindow* wnd) noexcept;
        // remove window
        void LongUIMethodCall RemoveWindow(UIWindow* wnd) noexcept;
        // register, return -1 for error(out of renderer space), return other for index
        auto LongUIMethodCall RegisterTextRenderer(UIBasicTextRenderer*) noexcept-> int32_t;
        // get text format, "Get" method will call IUnknown::AddRef if it is a COM object
        auto LongUIMethodCall GetTextFormat(uint32_t i) noexcept->IDWriteTextFormat*;
        // get bitmap by index, "Get" method will call IUnknown::AddRef if it is a COM object
        auto LongUIMethodCall GetBitmap(uint32_t index) noexcept->ID2D1Bitmap1*;
        // get brush by index, "Get" method will call IUnknown::AddRef if it is a COM object
        auto LongUIMethodCall GetBrush(uint32_t index) noexcept->ID2D1Brush*;
        // get meta by index, "Get" method will call IUnknown::AddRef if it is a COM object
        // Meta isn't a IUnknown object, so, won't call Meta::bitmap->AddRef
        void LongUIMethodCall GetMeta(uint32_t index, LongUI::Meta&) noexcept;
        // get meta's icon handle by index, will do runtime-converting if first call the
        //  same index. "Get" method will call IUnknown::AddRef if it is a COM object
        // HICON isn't a IUnknown object. Meta HICON managed by this manager
        auto LongUIMethodCall GetMetaHICON(uint32_t index) noexcept->HICON;
    public:
        // constructor ���캯��
        CUIManager() noexcept;
        // destructor ��������
        ~CUIManager() noexcept;
        // delte this method ɾ�����ƹ��캯��
        CUIManager(const CUIManager&) = delete;
    private:
        // create programs resources
        auto LongUIMethodCall create_programs_resources() /*throw(std::bad_alloc&)*/ ->void;
        // create all resources
        auto LongUIMethodCall create_resources() noexcept ->HRESULT;
        // discard resources
        void LongUIMethodCall discard_resources() noexcept;
        // create bitmap
        void LongUIMethodCall add_bitmap(const pugi::xml_node) noexcept;
        // create brush
        void LongUIMethodCall add_brush(const pugi::xml_node) noexcept;
        // create text format
        void LongUIMethodCall add_textformat(const pugi::xml_node) noexcept;
        // create meta
        void LongUIMethodCall add_meta(const pugi::xml_node) noexcept;
        // �����ؼ�
        auto LongUIMethodCall create_control(pugi::xml_node) noexcept->UIControl*;
        // �����ؼ���
        void LongUIMethodCall make_control_tree(UIWindow*, pugi::xml_node) noexcept;
        // ��ӿؼ�
        void LongUIMethodCall add_control(UIControl*, pugi::xml_node) noexcept;
    private:
        // main window proc ���ڹ��̺���
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
    public:
        // ���� CUIRenderer
        static      CUIManager      s_instance;
    public: // DEBUG ZONE ������
#ifdef _DEBUG
        // last DebugStringLevel
        DebugStringLevel        m_lastLevel = DebugStringLevel::DLevel_Log;
        // overload << operator ���� << �����
        template<typename T> CUIManager& operator<< (T t) noexcept;
        // overload << operator for DebugStringLevel
        template<> CUIManager& operator<< (DebugStringLevel l) noexcept { m_lastLevel = l; return *this; }
        // overload << operator for float
        template<> CUIManager& operator<< (float f) noexcept;
        // overload << operator for long
        template<> CUIManager& operator<< (long l) noexcept;
        // overload << operator for endl
        template<> CUIManager& operator<< (LongUI::EndL) noexcept;
        // overload << operator for DXGI_ADAPTER_DESC*
        template<> CUIManager& operator<< (DXGI_ADAPTER_DESC* d) noexcept;
        // overload << operator for const wchar_t*
        template<> CUIManager& operator<< (const wchar_t* s) noexcept { this->OutputNoFlush(m_lastLevel, s); return *this; }
        // overload << operator for wchar_t*
        template<> CUIManager& operator<< (wchar_t* s) noexcept { this->OutputNoFlush(m_lastLevel, s); return *this; }
        // overload << operator for const char*
        template<> CUIManager& operator<< (const char* s) noexcept { this->OutputNoFlush(m_lastLevel, s); return *this; }
        // overload << operator for char*
        template<> CUIManager& operator<< (char* s) noexcept { this->OutputNoFlush(m_lastLevel, s); return *this; }
        // overload << operator for wchar_t
        template<> CUIManager& operator<< (wchar_t ch) noexcept { wchar_t chs[2] = { ch, 0 }; this->OutputNoFlush(m_lastLevel, chs); return *this; }
        // output debug string with flush
        inline void Output(DebugStringLevel l, const wchar_t* s) noexcept { this->configure->OutputDebugStringW(l, s, true); }
        // output debug string with flush
        void Output(DebugStringLevel l, const char* s) noexcept;
        // Output with format for None
        void _cdecl OutputN(const wchar_t*, ...) noexcept;
        // Output with format for Log
        void _cdecl OutputL(const wchar_t*, ...) noexcept;
        // Output with format for Hint
        void _cdecl OutputH(const wchar_t*, ...) noexcept;
        // Output with format for Warning
        void _cdecl OutputW(const wchar_t*, ...) noexcept;
        // Output with format for Error
        void _cdecl OutputE(const wchar_t*, ...) noexcept;
        // Output with format for Fatal
        void _cdecl OutputF(const wchar_t*, ...) noexcept;
    private:
        // output debug (utf-8) string without flush
        void OutputNoFlush(DebugStringLevel l, const char* s) noexcept;
        // output debug string without flush
        inline void OutputNoFlush(DebugStringLevel l, const wchar_t* s) noexcept { this->configure->OutputDebugStringW(l, s, false); }
    public:
#else
        // overload << operator ���� << �����
        template<typename T> CUIManager& operator<< (T t) noexcept { return *this; }
        // output with wide char
        inline void Output(DebugStringLevel l, const wchar_t* s) noexcept {  }
        // output with utf-8
        inline void Output(DebugStringLevel l, const char* s) noexcept {  }
        // Output with format for None
        inline void _cdecl OutputN(const wchar_t*, ...) noexcept {  }
        // Output with format for Log
        inline void _cdecl OutputL(const wchar_t*, ...) noexcept {  }
        // Output with format for Hint
        inline void _cdecl OutputH(const wchar_t*, ...) noexcept {  }
        // Output with format for Warning
        inline void _cdecl OutputW(const wchar_t*, ...) noexcept {  }
        // Output with format for Error
        inline void _cdecl OutputE(const wchar_t*, ...) noexcept {  }
        // Output with format for Fatal
        inline void _cdecl OutputF(const wchar_t*, ...) noexcept {  }
#endif

    };
    // ��������
    template<typename T>
    LongUINoinline auto CUIManager::CreateUIWindow(const char* xml, void* buffer_sent, UIWindow* parent) noexcept->T* {
        pugi::xml_node root_node(nullptr); T* wnd = nullptr; auto buffer = buffer_sent;
        pugi::xml_parse_result result;
        // get buffer of window
        if (!buffer) buffer = LongUICtrlAlloc(sizeof(T));
        // parse the xml and check error
        if (buffer && (result = this->m_docWindow.load_string(xml)) &&
            (root_node = this->m_docWindow.first_child())) {
            // create the window
            wnd = new(buffer) T(root_node, parent);
            // recreate res'
            wnd->Recreate(this->m_pd2dDeviceContext);
            // make control tree
            this->make_control_tree(wnd, root_node);
            // finished
            LongUI::EventArgument arg; arg.sender = wnd;
            arg.event = LongUI::Event::Event_FinishedTreeBuliding;
            wnd->DoEvent(arg);
        }
        else if(!buffer_sent && buffer) {
            LongUICtrlFree(buffer);
        }
        // ������
        if (!result) {
            UIManager << DL_Error << L"XML Parse Error: " << result.description() << LongUI::endl;
        }
        assert(wnd && "no window created");
        return wnd;
    }
    // ��������
    template<typename T>
    LongUINoinline auto CUIManager::CreateUIWindow(const pugi::xml_node node, void* buffer_sent, UIWindow* parent) noexcept->T* {
        pugi::xml_node root_node(nullptr); T* wnd = nullptr; auto buffer = buffer_sent;
        // get buffer of window
        if (!buffer) buffer = LongUICtrlAlloc(sizeof(T));
        // check no error
        if (buffer && node) {
            // create the window
            wnd = new(buffer) T(root_node, parent);
            // recreate res'
            wnd->Recreate(this->m_pd2dDeviceContext);
            // make control tree
            this->make_control_tree(wnd, root_node);
            // finished
            LongUI::EventArgument arg; arg.sender = wnd;
            arg.event = LongUI::Event::Event_FinishedTreeBuliding;
            wnd->DoEvent(arg);
        }
        else if(!buffer_sent && buffer) {
            LongUICtrlFree(buffer);
        }
        assert(wnd && "no window created");
        return wnd;
    }
}