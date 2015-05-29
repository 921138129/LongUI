
#include "LongUI.h"

// node->Attribute\((.+?)\)
// node.attribute($1).value()


// CUIManager ��ʼ��
auto LongUIMethodCall LongUI::CUIManager::Initialize(IUIConfigure* config) noexcept->HRESULT {
    if (!config) {
#ifdef LONGUI_WITH_DEFAULT_CONFIG
        config = &m_config;
#else
        return E_INVALIDARG;
#endif
    }
    // ������Դ�ű�
    auto res_xml = config->GetResourceXML();
    if (res_xml) {
        auto re = m_docResource.load_string(res_xml);
        if (re.status) {
            assert(!"failed");
            ::MessageBoxA(nullptr, re.description(), "<LongUI::CUIManager::Initialize>: Failed to Parse XML", MB_ICONERROR);
        }
    }
    // ��ȡ����ϵͳ��Ϣ
#if 0
    if (IsWindows10OrGreater()) {
        force_cast(this->version) = WindowsVersion::Style_Win10;
    }
    /*else*/
#endif
    if (IsWindows8Point1OrGreater()) {
        force_cast(this->version) = WindowsVersion::Style_Win8_1;
    }
    // ��ȡ��Ϣ
    force_cast(this->configure) = config;
    force_cast(this->script) = config->GetScript();
    force_cast(this->inline_handler) = config->GetInlineParamHandler();
    *m_szLocaleName = 0;
    config->GetLocaleName(m_szLocaleName);
    // ��ʼ������
    ZeroMemory(m_apTextRenderer, sizeof(m_apTextRenderer));
    // ���Ĭ�ϴ�������
    this->AddS2CPair(L"Label", LongUI::UILabel::CreateControl);
    this->AddS2CPair(L"Button", LongUI::UIButton::CreateControl);
    this->AddS2CPair(L"VerticalLayout", LongUI::UIVerticalLayout::CreateControl);
    this->AddS2CPair(L"HorizontalLayout", LongUI::UIHorizontalLayout::CreateControl);
    this->AddS2CPair(L"Slider", LongUI::UISlider::CreateControl);
    this->AddS2CPair(L"CheckBox", LongUI::UICheckBox::CreateControl);
    this->AddS2CPair(L"RichEdit", LongUI::UIRichEdit::CreateControl);
    ///
    this->AddS2CPair(L"EditBasic", LongUI::UIEditBasic::CreateControl);
    this->AddS2CPair(L"Edit", LongUI::UIEditBasic::CreateControl);
    ///
    // ����Զ���ؼ�
    config->AddCustomControl(*this);
    // ��ȡʵ�����
    auto hInstance = ::GetModuleHandleW(nullptr);
    // ע�ᴰ���� | CS_DBLCLKS
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW ;
    wcex.lpfnWndProc = CUIManager::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(void*);
    wcex.hInstance = hInstance;
    wcex.hCursor = nullptr;
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"LongUIWindow"; 
    auto hicon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wcex.hIcon = hicon;
    // ע�ᴰ��
    ::RegisterClassExW(&wcex);
    m_pBitmap0Buffer = reinterpret_cast<uint8_t*>(malloc(
        sizeof(RGBQUAD)* LongUIDefaultBitmapSize * LongUIDefaultBitmapSize)
        );
    // �ؽ���Դ
    register HRESULT hr = m_pBitmap0Buffer ? S_OK : E_OUTOFMEMORY;
    // ����DirectInput����
    if (SUCCEEDED(hr)) {
        hr = ::DirectInput8Create(
            hInstance, 
            DIRECTINPUT_VERSION,
            LongUI_IID_PV_ARGS(m_pDirectInput),
            0
            );
    }
    // ��������豸
    if (SUCCEEDED(hr)) {
        hr = m_pDirectInput->CreateDevice(GUID_SysMouse, &m_pDInputMouse, 0);
    }
    // �������ݸ�ʽ :���
    if SUCCEEDED(hr) {
        hr = m_pDInputMouse->SetDataFormat(&c_dfDIMouse);
    }
    // ����Э���ȼ� ����ռ
    if SUCCEEDED(hr) {
        hr = m_pDInputMouse->SetCooperativeLevel(nullptr, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    }
    // �����������豸 ֪ͨ����ϵͳ�Ѿ�׼�����
    if SUCCEEDED(hr) {
        hr = m_pDInputMouse->Acquire();
    }
    // ����D2D����
    if (SUCCEEDED(hr)) {
        D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };
#ifdef _DEBUG
        options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
        hr = LongUI::Dll::D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            IID_ID2D1Factory1,
            &options,
            reinterpret_cast<void**>(&m_pd2dFactory)
            );
    }
    // ���� WIC ����.
    if (SUCCEEDED(hr)) {
        hr = ::CoCreateInstance(
            CLSID_WICImagingFactory2,
            nullptr,
            CLSCTX_INPROC_SERVER,
            LongUI_IID_PV_ARGS(m_pWICFactory)
            );
    }
    // ���� DirectWrite ����.
    IDWriteFactory1;
    if (SUCCEEDED(hr)) {
        hr = LongUI::Dll::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            LongUI_IID_PV_ARGS_Ex(m_pDWriteFactory)
            );
    }
    // �������弯
    if (SUCCEEDED(hr)) {
        m_pFontCollection = config->CreateFontCollection(*this);
        // ʧ�ܻ�ȡϵͳ���弯
        if (!m_pFontCollection) {
            hr = m_pDWriteFactory->GetSystemFontCollection(&m_pFontCollection);
        }
    }
    // ׼��������
    if (SUCCEEDED(hr)) {
        try {
            m_textFormats.reserve(64);
            m_brushes.reserve(64);
            m_windows.reserve(LongUIMaxWindow);
            m_bitmaps.reserve(64);
            m_metas.reserve(64);
            m_metaicons.reserve(64);
        }
        CATCH_HRESULT(hr)
    }
    // ע����Ⱦ��
    if (SUCCEEDED(hr)) {
        // ��ͨ��Ⱦ��
        if (this->RegisterTextRenderer(&m_normalTRenderer) != Type_NormalTextRenderer) {
            hr = E_FAIL;
        }
    }
    // ��ʼ���ű�
    if (this->script && !(this->script->Initialize(this))) {
        hr = E_FAIL;
    }
    // ������Դ
    if (SUCCEEDED(hr)) {
        hr = this->RecreateResources();
    }
    return hr;
}

// CUIManager  ����ʼ��
void LongUIMethodCall LongUI::CUIManager::UnInitialize() noexcept {
    // �����豸
    if (m_pDInputMouse) {
        m_pDInputMouse->Unacquire();
        m_pDInputMouse->Release();
        m_pDInputMouse = nullptr;
    }
    SafeRelease(m_pDirectInput);
    // �ͷŶ�ȡ��
    ::SafeRelease(m_pBinResLoader);
    // �ͷ��ı���Ⱦ��
    for (uint32_t i = 0; i < m_uTextRenderCount; ++i) {
        ::SafeRelease(m_apTextRenderer[i]);
    }
    // �ͷ���Դ
    this->discard_resources();
    ::SafeRelease(m_pFontCollection);
    ::SafeRelease(m_pDWriteFactory);
    ::SafeRelease(m_pWICFactory);
    ::SafeRelease(m_pd2dFactory);
    // �ͷ��ڴ�
    if (m_pBitmap0Buffer) {
        free(m_pBitmap0Buffer);
        m_pBitmap0Buffer = nullptr;
    }
    // ����ʼ���ű�
    if (this->script) {
        this->script->UnInitialize();
        this->script->Release();
        force_cast(script) = nullptr;
    }
    // �ͷ�����
    ::SafeRelease(force_cast(this->configure));
}


// CUIManager �����ؼ���
// Ĭ������ 64kb+, ����ջ(Ĭ��1~2M)������ʽϵ�
void LongUIMethodCall LongUI::CUIManager::make_control_tree(
    LongUI::UIWindow* window,
    pugi::xml_node node) noexcept {
    // ����
    assert(window && node && "bad argument");
    // ��Ӵ���
    add_control(window, node);
    // ���� -- ˳�������
    LongUI::FixedCirQueue<pugi::xml_node, LongUIMaxControlInited> xml_queue;
    LongUI::FixedCirQueue<UIContainer*, LongUIMaxControlInited> parents_queue;
    // 
    UIControl* now_control = nullptr;
    UIContainer* parent_node = window;
    // Ψһ����
    std::pair<CUIString, void*> control_name;
    // �����㷨: 1.ѹ�������ӽڵ� 2.���ε��� 3.�ظ�1
    while (true) {
        // ѹ��/��� �����ӽڵ�
        node = node.first_child();
        while (node) {
            xml_queue.push(node);
            parents_queue.push(parent_node);
            node = node.next_sibling();
        }
    recheck:
        // Ϊ�����˳�
        if (xml_queue.empty()) break;
        // ����/���� ��һ���ڵ�
        node = *xml_queue.front;  xml_queue.pop();
        parent_node = *parents_queue.front; parents_queue.pop();
        // �������ƴ����ؼ�
        if (!(now_control = this->create_control(node))) {
            parent_node = nullptr;
#ifdef _DEBUG
            const char* node_name = node.name();
            UIManager << DL_Error << L" Control Class Not Found: " << node_name << LongUI::endl;
#endif
            continue;
        }
        // ��ӵ���
        if (UIControl::MakeString(node.attribute("name").value(), control_name.first)) {
            control_name.second = now_control;
            try {
                m_mapString2Control.insert(control_name);
            }
            catch (...) {
                ::MessageBoxW(
                    window->GetHwnd(),
                    L"<LongUI::CUIManager::make_control_tree>"
                    L"try: m_mapString2Control.insert(control_name); -> Error",
                    L"LongUI Error!",
                    MB_ICONERROR
                    );
            }
        }
        // ���ô��ڽڵ�
        now_control->m_pWindow = window;
        // ����ӽڵ�
        parent_node->insert(parent_node->end(), now_control);
        // ���ýڵ�Ϊ�´θ��ڵ�
        parent_node = static_cast<decltype(parent_node)>(now_control);
        // ��鱾�ؼ��Ƿ���ҪXML�ӽڵ���Ϣ
        if (now_control->flags & Flag_ControlNeedFullXMLNode) {
            goto recheck;
        }
    }
}

// UIManager ��ӿؼ�
void LongUIMethodCall LongUI::CUIManager::add_control(
    UIControl* ctrl, pugi::xml_node node) noexcept {
    // ����
    assert(ctrl && node && "bad argument");
    // ����Pair
    std::pair<CUIString, void*> paired;
    // ����
    UIControl::MakeString(node.attribute("name").value(), paired.first);
    paired.second = ctrl;
    // ����
    m_mapString2Control.insert(paired);
}

// �����ؼ�
auto LongUIMethodCall LongUI::CUIManager::create_control(
    pugi::xml_node node) noexcept -> UIControl* {
    assert(node && "bad argument");
    // ת��
    WCHAR buffer[LongUIStringBufferLength];
    auto length = LongUI::UTF8toWideChar(node.name(), buffer);
    buffer[length] = L'\0';
    // �����ַ���
    CUIString class_name(buffer, length);
    // ����
    const auto itr = m_mapString2CreateFunction.find(class_name);
    if (itr != m_mapString2CreateFunction.end()) {
        return reinterpret_cast<CreateControlFunction>(itr->second)(node);
    }
    return nullptr;
}


// ��Ϣѭ��
void LongUIMethodCall LongUI::CUIManager::Run() noexcept {
    MSG msg;
    //auto now_time = ::timeGetTime();
    while (!m_exitFlag) {
        // ��ȡ���״̬
        m_lastMouseStates = this->now_mouse_states;
        if (m_pDInputMouse->GetDeviceState(sizeof(DIMOUSESTATE), &force_cast(this->now_mouse_states))
            == DIERR_INPUTLOST){
            m_pDInputMouse->Acquire();
        }
        // ��Ϣѭ��
        if (::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            // ���ַ�ʽ�˳� ::PostQuitMessage(0) or UIManager.Exit()
            if (msg.message == WM_QUIT) {
                m_exitFlag = true;
                break;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        // ����Ϊ��Ⱦ
        else {
            msg.message = WM_PAINT;
        }
        // ��Ⱦ
        if(msg.message == WM_PAINT){
            // �д��ھ���Ⱦ
            UIWindow* windows[LongUIMaxWindow];
            UIWindow** window_end = windows;
            // ��鴰��
            for (auto itr = m_windows.begin(); itr != m_windows.end(); ++itr) {
                register auto wnd = reinterpret_cast<UIWindow*>(*itr);
                if (wnd->UpdateRendering()) {
                    *window_end = wnd;
                    ++window_end;
                }
            }
            // ��Ⱦ����
            if (window_end != windows){
                for (auto itr = windows; itr < window_end; ++itr) {
                    (*itr)->BeginDraw();
                    (*itr)->Render();
                    (*itr)->EndDraw(itr == window_end - 1);
                }
            }
            else {
                //std::this_thread::sleep_for
                ::Sleep(1);
                // ����ʱ��Ƭ
                //::Sleep(0);
            }
        }
    }
    // ����ǿ�йر�(ʹ�õ�������ʹ������ʧЧ)
    while (!m_windows.empty()) {
        reinterpret_cast<UIWindow*>(m_windows.back())->Close();
    }
}

// ���ڹ��̺���
LRESULT LongUI::CUIManager::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
    // ��д����
    LongUI::EventArgument arg;  arg.msg = message;  arg.sender = nullptr;
    POINT pt; ::GetCursorPos(&pt); ::ScreenToClient(hwnd, &pt);
    arg.pt.x = static_cast<float>(pt.x); arg.pt.y = static_cast<float>(pt.y);
    arg.wParam_sys = wParam; arg.lParam_sys = lParam;
    // ����
    arg.lr = 0;
    // ��������ʱ����ָ��
    if (message == WM_CREATE)    {
        // ��ȡָ��
        LongUI::UIWindow *pUIWindow = reinterpret_cast<LongUI::UIWindow*>(
            (reinterpret_cast<LPCREATESTRUCT>(lParam))->lpCreateParams
            );
        // ���ô���ָ��
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, PtrToUlong(pUIWindow));
        // ����1
        arg.lr = 1;
    }
    else {
        // ��ȡ�����ָ��
        LongUI::UIWindow *pUIWindow = reinterpret_cast<LongUI::UIWindow *>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(hwnd, GWLP_USERDATA))
            );
        // ����Ƿ�����
        bool wasHandled = false;
        //ָ����Ч�����
        if (pUIWindow) {
            wasHandled = pUIWindow->DoEvent(arg);
        }
        // ��ҪĬ�ϴ���
        if (!wasHandled) {
            arg.lr = ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
    return  arg.lr;
}

// ��ȡMeta��ͼ����
auto LongUIMethodCall LongUI::CUIManager::GetMetaHICON(uint32_t index) noexcept -> HICON {
    // TODO DO IT
    auto& data = m_metaicons[index];
    // û�оʹ���
    if (!data) {
        ID2D1Bitmap1* bitmap = this->GetBitmap(LongUIDefaultBitmapIndex);
        Meta meta; this->GetMeta(index, meta);
        D2D1_RECT_U rect = {
            static_cast<uint32_t>(meta.src_rect.left),
            static_cast<uint32_t>(meta.src_rect.top),
            static_cast<uint32_t>(meta.src_rect.right),
            static_cast<uint32_t>(meta.src_rect.bottom)
        };
        HRESULT hr = (bitmap && meta.bitmap) ? E_FAIL : S_OK;
        // ��������
        if (SUCCEEDED(hr)) {
            hr = bitmap->CopyFromBitmap(nullptr, meta.bitmap, &rect);
        }
        // ӳ������
        if (SUCCEEDED(hr)) {
            D2D1_MAPPED_RECT mapped_rect = { 
                LongUIDefaultBitmapSize * sizeof(RGBQUAD) ,
                m_pBitmap0Buffer
            };
            hr = bitmap->Map(D2D1_MAP_OPTIONS_READ, &mapped_rect);
        }
        // ȡ��ӳ��
        if (SUCCEEDED(hr)) {
            hr = bitmap->Unmap();
        }
        assert(SUCCEEDED(hr));
        ::SafeRelease(bitmap);
    }
    assert(data && "no icon got");
    return static_cast<HICON>(data);
}

// CUIManager ���캯��
LongUI::CUIManager::CUIManager() noexcept {

}

// CUIManager ��������
LongUI::CUIManager::~CUIManager() noexcept {
    this->discard_resources();
}



// ��ȡ�ؼ� wchar_tָ��
auto LongUIMethodCall LongUI::CUIManager::AddS2CPair(
    const wchar_t* name, CreateControlFunction func) noexcept ->HRESULT {
    if (!name || !(*name)) return S_FALSE;
    // ����pair
    std::pair<LongUI::CUIString, CreateControlFunction> pair(name, func);
    HRESULT hr = S_OK;
    // ����
    try {
        m_mapString2CreateFunction.insert(pair);
    }
    // ����ʧ��
    CATCH_HRESULT(hr);
    return hr;
}



// ��ȡ�ؼ� wchar_tָ��
auto LongUIMethodCall LongUI::CUIManager::FindControlW(
    const wchar_t* str) noexcept ->LongUI::UIControl* {
    LongUI::CUIString uistr(str);
    return FindControl(uistr);
}


// ��ȡ�ؼ� 
auto LongUIMethodCall LongUI::CUIManager::FindControl(
    const LongUI::CUIString& str) noexcept ->LongUI::UIControl* {
    // ���ҿؼ�
    const auto itr = m_mapString2Control.find(str);
    // δ�ҵ����ؿ�
    if (itr == m_mapString2Control.cend()){
        // ����
        UIManager << DL_Warning << L"Control Not Found:\n  " << str.c_str() << LongUI::endl;
        return nullptr;
    }
    // �ҵ��ͷ���ָ��
    else{
        return reinterpret_cast<LongUI::UIControl*>(itr->second);
    }
}

// ��ʾ�������
void LongUIMethodCall LongUI::CUIManager::ShowError(HRESULT hr, const wchar_t* str_b) noexcept {
    wchar_t buffer[LongUIStringBufferLength];
    if (!::FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,  hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        buffer,
        lengthof(buffer),
        nullptr)) {
        // ����
        ::swprintf(
            buffer, LongUIStringBufferLength,
            L"Error! HRESULT Code: 0x%08X",
            hr
            );
    }
    // ����
    this->ShowError(buffer, str_b);
}

// ����LongUI�����弯: �����������I/O, ���Գ���ʼ����һ�μ���
auto LongUI::CUIManager::CreateLongUIFontCollection(
    IDWriteFactory* factory, const wchar_t * filename, const wchar_t * folder)
    noexcept -> IDWriteFontCollection *{
    // �����ļ�ö��
    class LongUIFontFileEnumerator final : public ComStatic<QiList<IDWriteFontFileEnumerator>> {
    public:
        // ��ȡ��ǰ�����ļ�
        HRESULT STDMETHODCALLTYPE GetCurrentFontFile(IDWriteFontFile **ppFontFile) noexcept override  {
            if (!ppFontFile) return E_INVALIDARG;
            if (!m_pFilePath || !m_pFactory)  return E_FAIL;
            *ppFontFile = ::SafeAcquire(m_pCurFontFie);
            return m_pCurFontFie ? S_OK : E_FAIL;
        }
        // �ƶ�����һ���ļ�
        HRESULT STDMETHODCALLTYPE MoveNext(BOOL *pHasCurrentFile) noexcept override {
            if (!pHasCurrentFile)return E_INVALIDARG;
            if (!m_pFilePath || !m_pFactory) return E_FAIL;
            HRESULT hr = S_OK;
            if (*pHasCurrentFile = *m_pFilePathNow) {
                ::SafeRelease(m_pCurFontFie);
                hr = m_pFactory->CreateFontFileReference(m_pFilePathNow, nullptr, &m_pCurFontFie);
                if (*pHasCurrentFile = SUCCEEDED(hr)) {
                    m_pFilePathNow += ::wcslen(m_pFilePathNow);
                    ++m_pFilePathNow;
                }
            }
            return hr;
        }
    public:
        // ���캯��
        LongUIFontFileEnumerator(IDWriteFactory* f) :m_pFactory(::SafeAcquire(f)) {}
        // ��������
        ~LongUIFontFileEnumerator() { ::SafeRelease(m_pCurFontFie); ::SafeRelease(m_pFactory); }
        // ��ʼ��
        auto Initialize(const wchar_t* path) { m_pFilePathNow = m_pFilePath = path; };
    private:
        // �ļ�·�� �����ַ���
        const wchar_t*              m_pFilePath = nullptr;
        // ��ǰ�ļ�·��
        const wchar_t*              m_pFilePathNow = nullptr;
        // ��ǰDirect Write Font File
        IDWriteFontFile*            m_pCurFontFie = nullptr;
        // DWrite ����
        IDWriteFactory*             m_pFactory;
    };
    // �����ļ�������
    class LongUIFontCollectionLoader final : public ComStatic<QiList<IDWriteFontCollectionLoader>> {
    public:
        // ����ö����
        HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey(
            IDWriteFactory *pFactory,
            const void *collectionKey,
            UINT32 collectionKeySize,
            IDWriteFontFileEnumerator **ppFontFileEnumerator
            ) noexcept override {
            if (!pFactory || !ppFontFileEnumerator) return E_INVALIDARG;
            m_enumerator.LongUIFontFileEnumerator::~LongUIFontFileEnumerator();
            m_enumerator.LongUIFontFileEnumerator::LongUIFontFileEnumerator(pFactory);
            m_enumerator.Initialize(reinterpret_cast<const wchar_t*>(collectionKey));
            *ppFontFileEnumerator = &m_enumerator;
            return S_OK;
        }
    public:
        // ���캯��
        LongUIFontCollectionLoader() :m_enumerator(nullptr) {}
        // ��������
        ~LongUIFontCollectionLoader() = default;
    private:
        // ö����
        LongUIFontFileEnumerator        m_enumerator;
    };
    IDWriteFontCollection* collection = nullptr;
    constexpr size_t buffer_length = 256 * 256;
    // �����㹻�Ŀռ�
    wchar_t* const buffer(new(std::nothrow) wchar_t[buffer_length]);
    if (buffer) {
        wchar_t* index = buffer; *buffer = 0;
        WIN32_FIND_DATA fileinfo;
        wchar_t file_name_path[MAX_PATH]; ::swprintf(file_name_path, MAX_PATH, L"%ls\\%ls", folder, filename);
        HANDLE hFile = ::FindFirstFileW(file_name_path, &fileinfo);
        DWORD errorcode = ::GetLastError();
        // �����ļ�
        while (hFile != INVALID_HANDLE_VALUE && errorcode != ERROR_NO_MORE_FILES) {
            ::swprintf(index, MAX_PATH, L"%ls\\%ls", folder, fileinfo.cFileName);
            index += ::wcslen(index) + 1; *index = 0;
            if (index + MAX_PATH >= buffer + buffer_length) {
                break;
            }
            ::FindNextFileW(hFile, &fileinfo);
            errorcode = ::GetLastError();
        }
        ::FindClose(hFile);
        // �����ڷ��ϱ�׼���ļ�ʱ
        if (index != buffer) {
            LongUIFontCollectionLoader loader;
            factory->RegisterFontCollectionLoader(&loader);
            factory->CreateCustomFontCollection(
                &loader,
                buffer, reinterpret_cast<uint8_t*>(index) - reinterpret_cast<uint8_t*>(buffer),
                &collection
                );
            factory->UnregisterFontCollectionLoader(&loader);
        }
        delete[] buffer;
    }
    return collection;
}

// �� �ı���ʽ��������
auto LongUI::CUIManager::CreateTextPathGeometry(
    IN const char32_t* utf32_string,
    IN size_t string_length,
    IN IDWriteTextFormat* format,
    IN ID2D1Factory* factory,
    IN OUT OPTIONAL IDWriteFontFace** _fontface,
    OUT ID2D1PathGeometry** geometry
    ) noexcept -> HRESULT {
    // �������
    if (!utf32_string || !string_length || !format || !geometry || !factory) return E_INVALIDARG;
    // ���弯
    IDWriteFontCollection* collection = nullptr;
    IDWriteFontFamily* family = nullptr;
    IDWriteFont* font = nullptr;
    IDWriteFontFace* fontface = nullptr;
    ID2D1PathGeometry* pathgeometry = nullptr;
    if (_fontface) fontface = ::SafeAcquire(*_fontface);
    // �������ƻ���
    wchar_t fontname_buffer[MAX_PATH]; *fontname_buffer = 0;
    // ��Ҫ����
    uint16_t glyph_indices_buffer[1024];
    // ��֤�ռ�
    uint16_t* glyph_indices = string_length > lengthof(glyph_indices_buffer) ?
        new(std::nothrow) uint16_t[string_length * sizeof(uint16_t)] : glyph_indices_buffer;
    HRESULT hr = glyph_indices ? S_OK : E_OUTOFMEMORY;
    // ��������
    if (!fontface) {
        // ��ȡ��������
        if (SUCCEEDED(hr)) {
            hr = format->GetFontFamilyName(fontname_buffer, MAX_PATH);
        }
        // ��ȡ���弯
        if (SUCCEEDED(hr)) {
            hr = format->GetFontCollection(&collection);
        }
        // ��������
        uint32_t index = 0; BOOL exists = FALSE;
        if (SUCCEEDED(hr)) {
            hr = collection->FindFamilyName(fontname_buffer, &index, &exists);
        }
        // ��ȡ������
        if (SUCCEEDED(hr)) {
            if (exists) {
                hr = collection->GetFontFamily(index, &family);
            }
            else {
                hr = E_FAIL;
            }
        }
        // ��ȡ����
        if (SUCCEEDED(hr)) {
            hr = family->GetFirstMatchingFont(
                format->GetFontWeight(),
                format->GetFontStretch(),
                format->GetFontStyle(),
                &font
                );
        }
        // ��������
        if (SUCCEEDED(hr)) {
            hr = font->CreateFontFace(&fontface);
        }
    }
    // ��������
    if (SUCCEEDED(hr)) {
        hr = factory->CreatePathGeometry(&pathgeometry);
        ID2D1GeometrySink* sink = nullptr;
        // ��Sink
        if (SUCCEEDED(hr)) {
            hr = pathgeometry->Open(&sink);
        }
        // �����������
        if (SUCCEEDED(hr)) {
            static_assert(sizeof(uint32_t) == sizeof(char32_t), "32 != 32 ?!");
            hr = fontface->GetGlyphIndices(
                reinterpret_cast<const uint32_t*>(utf32_string), string_length, glyph_indices
                );
        }
        // ��������·������
        if (SUCCEEDED(hr)) {
            hr = fontface->GetGlyphRunOutline(
                format->GetFontSize(),
                glyph_indices,
                nullptr, nullptr,
                string_length,
                true, true, sink
                );
        }
        // �ر�·��
        if (SUCCEEDED(hr)) {
            sink->Close();
        }
        ::SafeRelease(sink);
    }
    // ɨβ
    ::SafeRelease(collection);
    ::SafeRelease(family);
    ::SafeRelease(font);
    if (_fontface && !(*_fontface)) {
        *_fontface = fontface;
    }
    else {
        ::SafeRelease(fontface);
    }
    if (glyph_indices && glyph_indices != glyph_indices_buffer) {
        delete[] glyph_indices;
        glyph_indices = nullptr;
    }
    *geometry = pathgeometry;
#ifdef _DEBUG
    if (pathgeometry) {
        float float_var = 0.f;
        pathgeometry->ComputeLength(nullptr, &float_var);
        pathgeometry->ComputeArea(nullptr, &float_var);
        float_var = 0.f;
    }
#endif
    return hr;
}

// ���ü����崴������
auto LongUI::CUIManager::CreateMeshFromGeometry(ID2D1Geometry * geometry, ID2D1Mesh ** mesh) noexcept -> HRESULT {
    return E_NOTIMPL;
}



// ת��ΪCore-Mode��ʽ
auto LongUI::CUIManager::XMLToCoreFormat(const char* xml, wchar_t* core) noexcept -> bool {
    if (!xml || !core) return false;
    wchar_t buffer[LongUIStringBufferLength];
    *buffer = 0;
    return true;
}


// ע���ı���Ⱦ��
auto LongUIMethodCall LongUI::CUIManager::RegisterTextRenderer(
    UIBasicTextRenderer* renderer) noexcept -> int32_t {
    if (m_uTextRenderCount == lengthof(m_apTextRenderer)) {
        return -1;
    }
    register const auto count = m_uTextRenderCount;
    m_apTextRenderer[count] = ::SafeAcquire(renderer);
    ++m_uTextRenderCount;
    return count;
}

// CUIManager ��ȡ�ı���ʽ
auto LongUIMethodCall LongUI::CUIManager::GetTextFormat(
    uint32_t index) noexcept ->IDWriteTextFormat* {
    IDWriteTextFormat* pTextFormat = nullptr;
    if (index >= m_textFormats.size()) {
        // Խ��
        UIManager << DL_Warning << L"index@" << long(index)
            << L" is out of range\n   Now set to 0" << LongUI::endl;
        index = 0;
    }
    pTextFormat = reinterpret_cast<IDWriteTextFormat*>(m_textFormats[index]);
    // δ�ҵ�
    if (!pTextFormat) {
        UIManager << DL_Error << L"index@" << long(index) << L" TF is null" << LongUI::endl;
    }
    return ::SafeAcquire(pTextFormat);

}

// ����������Դ
auto LongUIMethodCall LongUI::CUIManager::create_programs_resources() /*throw(std::bad_alloc &)*/-> void {
    // ���ڶ����ƶ�ȡ��?
    if (m_pBinResLoader) {
        // ��ȡλͼ����
        register auto count = m_pBinResLoader->GetBitmapCount();
        m_bitmaps.reserve(count);
        // ��ȡλͼ
        for (decltype(count) i = 0; i < count; ++i) {
            m_bitmaps.push_back(m_pBinResLoader->LoadBitmapAt(*this, i));
        }
        // ��ȡ��ˢ����
        count = m_pBinResLoader->GetBrushCount();
        m_brushes.reserve(count);
        // ��ȡ��ˢ
        for (decltype(count) i = 0; i < count; ++i) {
            m_brushes.push_back(m_pBinResLoader->LoadBrushAt(*this, i));
        }
        // ��ȡ�ı���ʽ����
        count = m_pBinResLoader->GetTextFormatCount();
        m_textFormats.reserve(count);
        // ��ȡ�ı���ʽ
        for (decltype(count) i = 0; i < count; ++i) {
            m_textFormats.push_back(m_pBinResLoader->LoadTextFormatAt(*this, i));
        }
        // ��ȡMeta����
        count = m_pBinResLoader->GetMetaCount();
        m_metas.reserve(count);
        // ͼ��
        m_metaicons.resize(count);
        // ��ȡ�ı���ʽ
        for (decltype(count) i = 0; i < count; ++i) {
            Meta meta; m_pBinResLoader->LoadMetaAt(*this, i, meta);
            m_metas.push_back(meta);
        }
    }
    else {
        // pugixml ʹ�õ��Ǿ��ʽ, ��������Ĵ����ǰ�ȫ��.
        // ���ǻ���΢��ʧ����(����cache���о�׬��һ����)
        // ����pugixml����������C++����, �Լ��ľ�ƫ����
        // C���, ȫ��ָ��, ��������ָ����
        register auto now_node = m_docResource.first_child().first_child();
        while (now_node) {
            // λͼ?
            if (!::strcmp(now_node.name(), "Bitmap")) {
                this->add_bitmap(now_node);
            }
            // ��ˢ?
            else if (!::strcmp(now_node.name(), "Brush")) {
                this->add_brush(now_node);
            }
            // �ı���ʽ?
            else if (!::strcmp(now_node.name(), "Font") ||
                !::strcmp(now_node.name(), "TextFormat")) {
                this->add_textformat(now_node);
            }
            // ͼԪ?
            else if (!::strcmp(now_node.name(), "Meta")) {
                this->add_meta(now_node);
            }
            // ����ͼԪ?
            else if (!::strcmp(now_node.name(), "MetaEx")) {
                this->add_meta(now_node);
            }
            // �ƽ�
            now_node = now_node.next_sibling();
        }
    }
}


// UIManager ����
auto LongUIMethodCall LongUI::CUIManager::create_resources() noexcept ->HRESULT {
    // �����Ⱦ����
    bool cpu_rendering = this->configure->IsRenderByCPU();
    // ����������
    IDXGIAdapter1* ready2use = nullptr;
    // ö����ʾ������
    if(!cpu_rendering) {
        IDXGIFactory1* temp_factory = nullptr;
        // ����һ����ʱ����
        register auto hr = LongUI::Dll::CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void**>(&temp_factory));
        if (SUCCEEDED(hr)) {
            IDXGIAdapter1* apAdapters[256]; size_t adnum;
            // ö��������
            for (adnum = 0; adnum < lengthof(apAdapters); ++adnum) {
                if (temp_factory->EnumAdapters1(adnum, apAdapters + adnum) == DXGI_ERROR_NOT_FOUND) {
                    break;
                }
            }
            // ѡ��������
            register auto index = this->configure->ChooseAdapter(apAdapters, adnum);
            if (index < adnum) {
                ready2use = ::SafeAcquire(apAdapters[index]);
            }
            // �ͷ�������
            for (size_t i = 0; i < adnum; ++i) {
                ::SafeRelease(apAdapters[i]);
            }
        }
        ::SafeRelease(temp_factory);
    }
    // �����豸��Դ
    register HRESULT hr /*= m_docResource.Error() ? E_FAIL :*/ S_OK;
    // ���� D3D11�豸���豸������ 
    if (SUCCEEDED(hr)) {
        // D3D11 ����flag 
        // һ��Ҫ��D3D11_CREATE_DEVICE_BGRA_SUPPORT
        // ���򴴽�D2D�豸�����Ļ�ʧ��
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
#ifdef D3D11_DEBUG
        // Debug״̬ ��D3D DebugLayer�Ϳ���ȡ��ע��
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
        auto tmpflag = D3D11_CREATE_DEVICE_FLAG(creationFlags);
#endif
#endif
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
        // �����豸
        hr = LongUI::Dll::D3D11CreateDevice(
            // ����Ϊ��Ⱦ
            ready2use,
            // �������ѡ������
            cpu_rendering ? D3D_DRIVER_TYPE_WARP : 
                (ready2use ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE),
            // û������ӿ�
            nullptr,
            // ����flag
            creationFlags,
            // ��ʹ�õ����Եȼ��б�
            featureLevels,
            // ���Եȼ��б���
            lengthof(featureLevels),
            // SDK �汾
            D3D11_SDK_VERSION,
            // ���ص�D3D11�豸ָ��
            &m_pd3dDevice,
            // ���ص����Եȼ�
            &m_featureLevel,
            // ���ص�D3D11�豸������ָ��
            &m_pd3dDeviceContext
            );
        // ���
        if (FAILED(hr)) {
            UIManager << DL_Hint << L"Create D3D11 Device Failed,"
                L" Now, Try to Create In WARP Mode" << LongUI::endl;
        }
        // ����ʧ������ʹ�����
        if (FAILED(hr)) {
            hr = LongUI::Dll::D3D11CreateDevice(
                // ����Ϊ��Ⱦ
                nullptr,
                // �������ѡ������
                D3D_DRIVER_TYPE_WARP,
                // û������ӿ�
                nullptr,
                // ����flag
                creationFlags,
                // ��ʹ�õ����Եȼ��б�
                featureLevels,
                // ���Եȼ��б���
                lengthof(featureLevels),
                // SDK �汾
                D3D11_SDK_VERSION,
                // ���ص�D3D11�豸ָ��
                &m_pd3dDevice,
                // ���ص����Եȼ�
                &m_featureLevel,
                // ���ص�D3D11�豸������ָ��
                &m_pd3dDeviceContext
                );
        }
    }
#ifdef _DEBUG
#ifdef D3D11_DEBUG
    // ���� ID3D11Debug
    if (SUCCEEDED(hr)) {
        hr = m_pd3dDevice->QueryInterface(LongUI_IID_PV_ARGS(m_pd3dDebug));
    }
#endif
#endif
    // ���� IDXGIDevice
    if (SUCCEEDED(hr)) {
        hr = m_pd3dDevice->QueryInterface(LongUI_IID_PV_ARGS(m_pDxgiDevice));
    }
    // ����D2D�豸
    if (SUCCEEDED(hr)) {
        hr = m_pd2dFactory->CreateDevice(m_pDxgiDevice, &m_pd2dDevice);
    }
    // ����D2D�豸������
    if (SUCCEEDED(hr)) {
        hr = m_pd2dDevice->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
            &m_pd2dDeviceContext
            );
    }
    // ��ȡDxgi������ ���Ի�ȡ����������Ϣ
    if (SUCCEEDED(hr)) {
        // ˳��ʹ��������Ϊ��λ
        m_pd2dDeviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
        hr = m_pDxgiDevice->GetAdapter(&m_pDxgiAdapter);
    }
#ifdef _DEBUG
    // ����Կ���Ϣ
    if (SUCCEEDED(hr)) {
        DXGI_ADAPTER_DESC desc;
        m_pDxgiAdapter->GetDesc(&desc);
        UIManager << DL_Hint << &desc << LongUI::endl;
    }
#endif
    // ��ȡDxgi����
    if (SUCCEEDED(hr)) {
        hr = m_pDxgiAdapter->GetParent(LongUI_IID_PV_ARGS(m_pDxgiFactory));
    }
#ifdef LONGUI_VIDEO_IN_MF
    uint32_t token = 0;
    // ���߳�
    if (SUCCEEDED(hr)) {
        ID3D10Multithread* mt = nullptr;
        hr = m_pd3dDevice->QueryInterface(IID_ID3D10Multithread, (void**)&mt);
        // ����
        if (SUCCEEDED(hr)) {
            mt->SetMultithreadProtected(TRUE);
        }
        ::SafeRelease(mt);
    }
    // ����MF
    if (SUCCEEDED(hr)) {
        hr = ::MFStartup(MF_VERSION);
    }
    // ����MF Dxgi �豸������
    if (SUCCEEDED(hr)) {
        hr = ::MFCreateDXGIDeviceManager(&token, &m_pDXGIManager);
    }
    // �����豸
    if (SUCCEEDED(hr)) {
        hr = m_pDXGIManager->ResetDevice(m_pd3dDevice, token);
    }
    // ����MFý���๤��
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(
            CLSID_MFMediaEngineClassFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            LongUI_IID_PV_ARGS(m_pMediaEngineFactory)
            );
    }
#endif
    /*// ��ֹ Alt + Enter ȫ��
    if (SUCCEEDED(hr)) {
        hr = m_pDxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    }*/
    // ��������0��Դ
    if (SUCCEEDED(hr)) {
        // ��Map��λͼ
        if (SUCCEEDED(hr)) {
            ID2D1Bitmap1* bitmap_index0 = nullptr;
            hr = m_pd2dDeviceContext->CreateBitmap(
                D2D1::SizeU(LongUIDefaultBitmapSize, LongUIDefaultBitmapSize),
                nullptr, LongUIDefaultBitmapSize * 4,
                D2D1::BitmapProperties1(
                static_cast<D2D1_BITMAP_OPTIONS>(LongUIDefaultBitmapOptions),
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                ),
                &bitmap_index0
                );
            m_bitmaps.push_back(bitmap_index0);
        }
        // ��ˢ
        if (SUCCEEDED(hr)) {
            ID2D1SolidColorBrush* scbrush = nullptr;
            D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Black);
            hr = m_pd2dDeviceContext->CreateSolidColorBrush(
                &color, nullptr, &scbrush
                );
            m_brushes.push_back(scbrush);
        }
        // �ı���ʽ
        if (SUCCEEDED(hr)) {
            IDWriteTextFormat* format = nullptr;
            hr = m_pDWriteFactory->CreateTextFormat(
                LongUIDefaultTextFontName,
                m_pFontCollection,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                LongUIDefaultTextFontSize,
                m_szLocaleName,
                &format
                );
            if (format) {
                format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                m_textFormats.push_back(format);
            }
        }
        // ͼԪ
        Meta meta = { 0 }; m_metas.push_back(meta);
    }
    // ������Դ������Դ
    if (SUCCEEDED(hr)) {
        try {
            this->create_programs_resources();
        }
        CATCH_HRESULT(hr);
    }
    ::SafeRelease(ready2use);
    // �����ı���Ⱦ������
    if (SUCCEEDED(hr)) {
        for (uint32_t i = 0u; i < m_uTextRenderCount; ++i) {
            m_apTextRenderer[i]->SetNewRT(m_pd2dDeviceContext);
            m_apTextRenderer[i]->SetNewBrush(static_cast<ID2D1SolidColorBrush*>(m_brushes[0]));
        }
        // �ؽ����д���
        for (auto i : m_windows) {
            reinterpret_cast<UIWindow*>(i)->Recreate(nullptr);
        }
    }
    // ���� HR
    AssertHR(hr);
    return hr;
}


#ifdef _DEBUG
template<typename T>
void AssertRelease(T* t) {
    auto count = t->Release();
    if (count) {
        UIManager << DL_Hint << L"[IUnknown *] count :" << long(count) << LongUI::endl;
        int breakpoint = 0;
    }
}

#define SafeReleaseContainer(c) \
    if (c.size()) {\
        for (auto itr = c.begin(); itr != c.end(); ++itr) {\
             register auto* interfacegot = static_cast<ID2D1Bitmap*>(*itr);\
             if (interfacegot) {\
                AssertRelease(interfacegot);\
                 *itr = nullptr;\
             }\
        }\
        c.clear();\
    }
#else
#define SafeReleaseContainer(c) \
    if (c.size()) {\
        for (auto itr = c.begin(); itr != c.end(); ++itr) {\
             register auto* interfacegot = static_cast<ID2D1Bitmap*>(*itr);\
             if (interfacegot) {\
                 interfacegot->Release();\
                 *itr = nullptr;\
             }\
        }\
        c.clear();\
    }
#endif
// UIManager ����
void LongUIMethodCall LongUI::CUIManager::discard_resources() noexcept {
    // �ͷ� λͼ
    SafeReleaseContainer(m_bitmaps);
    // �ͷ� ��ˢ
    SafeReleaseContainer(m_brushes);
    // �ͷ��ı���ʽ
    SafeReleaseContainer(m_textFormats);
    // metaֱ���ͷ�
    if (m_metas.size()) {
        m_metas.clear();
    }
    // ͼ��ݻ�
    if (m_metaicons.size()) {
        for (auto itr = m_metaicons.begin(); itr != m_metaicons.end(); ++itr) {
             register auto handle = static_cast<HICON>(*itr);
             if (handle) {
                 ::DestroyIcon(handle);
                 *itr = nullptr;
             }
        }
        m_metaicons.clear();
    }
    // �ͷ� �豸
    ::SafeRelease(m_pDxgiFactory);
    ::SafeRelease(m_pd2dDeviceContext);
    ::SafeRelease(m_pd2dDevice);
    ::SafeRelease(m_pDxgiAdapter);
    ::SafeRelease(m_pDxgiDevice);
    ::SafeRelease(m_pd3dDevice);
    ::SafeRelease(m_pd3dDeviceContext);
#ifdef LONGUI_VIDEO_IN_MF
    ::SafeRelease(m_pDXGIManager);
    ::SafeRelease(m_pMediaEngineFactory);
    ::MFShutdown();
#endif
#ifdef _DEBUG
    if (m_pd3dDebug) {
        m_pd3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    }
    ::SafeRelease(m_pd3dDebug);
#endif
}



// ��ȡ��ˢ
auto LongUIMethodCall LongUI::CUIManager::GetBrush(
    uint32_t index) noexcept -> ID2D1Brush* {
    ID2D1Brush* brush = nullptr;
    if (index >= m_brushes.size()) {
        // Խ��
        UIManager << DL_Warning << L"index@" << long(index)
            << L"is out of range\n   Now set to 0" << LongUI::endl;
        index = 0;
    }
    brush = reinterpret_cast<ID2D1Brush*>(m_brushes[index]);
    // ����
    if (!brush) {
        UIManager << DL_Error << L"index@" << long(index) << L"brush is null" << LongUI::endl;
    }
    return ::SafeAcquire(brush);
}

// ��ȡλͼ
auto LongUIMethodCall LongUI::CUIManager::GetBitmap(
    uint32_t index) noexcept ->ID2D1Bitmap1* {
    ID2D1Bitmap1* bitmap = nullptr;
    if (index >= m_bitmaps.size()) {
        // Խ��
        UIManager << DL_Warning << L"index@" << long(index)
            << L"is out of range\n   Now set to 0" << LongUI::endl;
        index = 0;
    }
    bitmap = static_cast<ID2D1Bitmap1*>(m_bitmaps[index]);
    // ����
    if (!bitmap) {
        UIManager << DL_Error << L"index@" << long(index) << L"bitmap is null" << LongUI::endl;
    }
    return ::SafeAcquire(bitmap);
}

// ��ȡͼԪ
void LongUIMethodCall LongUI::CUIManager::GetMeta(
    uint32_t index, LongUI::Meta& meta) noexcept {
    if (index >= m_metas.size()) {
        // Խ��
        UIManager << DL_Warning << L"index@" << long(index)
            << L"is out of range\n   Now set to 0" << LongUI::endl;
        index = 0;
    }
    meta = m_metas[index];
}

// ���λͼ
void LongUIMethodCall LongUI::CUIManager::add_bitmap(
    const pugi::xml_node node) noexcept {
    assert(node && "bad argument");
    // ��ȡ·��
    const char* uri = node.attribute("res").value();
    // ����λͼ
    auto bitmap = this->configure->LoadBitmapByRI(*this, uri);
    // û��
    if (!bitmap) {
        UIManager << DL_Error << L"Resource Identifier: [" << uri << L"], got a null pointer" << LongUI::endl;
    }
    m_bitmaps.push_back(bitmap);
}

// ��ӱ�ˢ
void LongUIMethodCall LongUI::CUIManager::add_brush(
    const pugi::xml_node node) noexcept {
    union {
        ID2D1SolidColorBrush*       scb;
        ID2D1LinearGradientBrush*   lgb;
        ID2D1RadialGradientBrush*   rgb;
        ID2D1BitmapBrush1*          b1b;
        ID2D1Brush*                 brush;
    };
    brush = nullptr;
    const char* str = nullptr;
    assert(node && "bad argument");
    // ��ˢ����
    D2D1_BRUSH_PROPERTIES brush_prop = D2D1::BrushProperties();
    if (str = node.attribute("opacity").value()) {
        brush_prop.opacity = static_cast<float>(::LongUI::AtoF(str));
    }
    if (str = node.attribute("transform").value()) {
        UIControl::MakeFloats(str, &brush_prop.transform._11, 6);
    }
    // �������
    auto type = BrushType::Type_SolidColor;
    if (str = node.attribute("type").value()) {
        type = static_cast<decltype(type)>(::LongUI::AtoI(str));
    }
    switch (type)
    {
    case LongUI::BrushType::Type_SolidColor:
    {
        D2D1_COLOR_F color;
        // ��ȡ��ɫ
        if (!UIControl::MakeColor(node.attribute( "color").value(), color)) {
            color = D2D1::ColorF(D2D1::ColorF::Black);
        }
        m_pd2dDeviceContext->CreateSolidColorBrush(&color, &brush_prop, &scb);
    }
    break;
    case LongUI::BrushType::Type_LinearGradient:
        __fallthrough;
    case LongUI::BrushType::Type_RadialGradient:
        if (str = node.attribute("stops").value()) {
            // �﷨ [pos0, color0] [pos1, color1] ....
            uint32_t stop_count = 0;
            ID2D1GradientStopCollection * collection = nullptr;
            D2D1_GRADIENT_STOP stops[LongUIMaxGradientStop];
            D2D1_GRADIENT_STOP* now_stop = stops;

            char buffer[LongUIStringBufferLength];
            // ���Ƶ�������
            strcpy(buffer, str);
            char* index = buffer;
            const char* paragraph = nullptr;
            register char ch = 0;
            bool ispos = false;
            // �������
            while (ch = *index) {
                // ���ҵ�һ����������Ϊλ��
                if (ispos) {
                    // ,��ʾλ�öν���, �ý�����
                    if (ch = ',') {
                        *index = 0;
                        now_stop->position = LongUI::AtoF(paragraph);
                        ispos = false;
                        paragraph = index + 1;
                    }
                }
                // ���Һ������ֵ��Ϊ��ɫ
                else {
                    // [ ��Ϊλ�öα�ʶ��ʼ
                    if (ch == '[') {
                        paragraph = index + 1;
                        ispos = true;
                    }
                    // ] ��Ϊ��ɫ�α�ʶ���� �ý�����
                    else if (ch == ']') {
                        *index = 0;
                        UIControl::MakeColor(paragraph, now_stop->color);
                        ++now_stop;
                        ++stop_count;
                    }
                }
            }
            // ����StopCollection
            m_pd2dDeviceContext->CreateGradientStopCollection(stops, stop_count, &collection);
            if (collection) {
                // ���Խ���?
                if (type == LongUI::BrushType::Type_LinearGradient) {
                    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES lgbprop = {
                        {0.f, 0.f}, {0.f, 0.f}
                    };
                    // �������
                    UIControl::MakeFloats(node.attribute("start").value(), &lgbprop.startPoint.x, 2);
                    UIControl::MakeFloats(node.attribute("end").value(), &lgbprop.startPoint.x, 2);
                    // ������ˢ
                    m_pd2dDeviceContext->CreateLinearGradientBrush(
                        &lgbprop, &brush_prop, collection, &lgb
                        );
                }
                // ���򽥱��ˢ
                else {
                    D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES rgbprop = {
                        {0.f, 0.f}, {0.f, 0.f}, 0.f, 0.f
                    };
                    // �������
                    UIControl::MakeFloats(node.attribute("center").value(), &rgbprop.center.x, 2);
                    UIControl::MakeFloats(node.attribute("offset").value(), &rgbprop.gradientOriginOffset.x, 2);
                    UIControl::MakeFloats(node.attribute("rx").value(), &rgbprop.radiusX, 1);
                    UIControl::MakeFloats(node.attribute("ry").value(), &rgbprop.radiusY, 1);
                    // ������ˢ
                    m_pd2dDeviceContext->CreateRadialGradientBrush(
                        &rgbprop, &brush_prop, collection, &rgb
                        );
                }
                collection->Release();
                collection = nullptr;
            }
        }
        break;
    case LongUI::BrushType::Type_Bitmap:
        if (str = node.attribute("bitmap").value()) {
            auto index = LongUI::AtoI(str);
            D2D1_BITMAP_BRUSH_PROPERTIES1 bbprop = {
                D2D1_EXTEND_MODE_CLAMP, D2D1_EXTEND_MODE_CLAMP,D2D1_INTERPOLATION_MODE_LINEAR
            };
            // �������
            if (str = node.attribute("extendx").value()) {
                bbprop.extendModeX = static_cast<D2D1_EXTEND_MODE>(LongUI::AtoI(str));
            }
            if (str = node.attribute("extendy").value()) {
                bbprop.extendModeY = static_cast<D2D1_EXTEND_MODE>(LongUI::AtoI(str));
            }
            if (str = node.attribute("interpolation").value()) {
                bbprop.interpolationMode = static_cast<D2D1_INTERPOLATION_MODE>(LongUI::AtoI(str));
            }
            // ������ˢ
            m_pd2dDeviceContext->CreateBitmapBrush(
                static_cast<ID2D1Bitmap*>(m_bitmaps[index]), 
                &bbprop, &brush_prop, &b1b
                );
        }
        break;
    }
    // �������Ӽ��һ��
    assert(brush);
    m_brushes.push_back(brush);
}

// ���ͼԪ
void LongUIMethodCall LongUI::CUIManager::add_meta(
    const pugi::xml_node node) noexcept {
    LongUI::Meta meta = {
        nullptr, BitmapRenderRule::Rule_Scale, 
        D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
        { 0.f, 0.f, 1.f, 1.f}
    };
    const char* str = nullptr;
    assert(node && "bad argument");
    // ��ȡλͼ
    meta.bitmap = static_cast<ID2D1Bitmap1*>(m_bitmaps[LongUI::AtoI(node.attribute("bitmap").value())]);
    // ��ȡ��Ⱦ����
    if (str = node.attribute("rule").value()) {
        meta.rule = static_cast<BitmapRenderRule>(LongUI::AtoI(str));
    }
    // ��ȡ��ֵģʽ
    if (str = node.attribute("interpolation").value()) {
        meta.interpolation = static_cast<uint16_t>(LongUI::AtoI(str));
    }
    // ��ȡ����
    UIControl::MakeFloats(node.attribute("rect").value(), &meta.src_rect.left, 4);
    // ����
    m_metas.push_back(meta);
    // ͼ��
    m_metaicons.push_back(nullptr);
}


// ����ı���ʽ
void LongUIMethodCall LongUI::CUIManager::add_textformat(
    const pugi::xml_node node) noexcept {
    register const char* str = nullptr;
    assert(node && "bad argument");
    CUIString fontfamilyname(L"Arial");
    DWRITE_FONT_WEIGHT fontweight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE fontstyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH fontstretch = DWRITE_FONT_STRETCH_NORMAL;
    float fontsize = 12.f ;
    // ��ȡ��������
    UIControl::MakeString(node.attribute("family").value(), fontfamilyname);
    // ��ȡ�����ϸ
    if (str = node.attribute("weight").value()) {
        fontweight = static_cast<DWRITE_FONT_WEIGHT>(LongUI::AtoI(str));
    }
    // ��ȡ������
    if (str = node.attribute("style").value()) {
        fontstyle = static_cast<DWRITE_FONT_STYLE>(LongUI::AtoI(str));
    }
    // ��ȡ��������
    if (str = node.attribute("stretch").value()) {
        fontstretch = static_cast<DWRITE_FONT_STRETCH>(LongUI::AtoI(str));
    }
    // ��ȡ�����С
    if (str = node.attribute("size").value()) {
        fontsize = LongUI::AtoF(str);
    }
    // ������������
    IDWriteTextFormat* textformat = nullptr;
    m_pDWriteFactory->CreateTextFormat(
        fontfamilyname.c_str(),
        m_pFontCollection,
        fontweight,
        fontstyle,
        fontstretch,
        fontsize,
        m_szLocaleName,
        &textformat
        );
    // �ɹ���ȡ��������
    if (textformat) {
        // DWRITE_LINE_SPACING_METHOD;
        DWRITE_FLOW_DIRECTION flowdirection = DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM;
        float tabstop = fontsize * 4.f;
        DWRITE_PARAGRAPH_ALIGNMENT valign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        DWRITE_TEXT_ALIGNMENT halign = DWRITE_TEXT_ALIGNMENT_LEADING;
        DWRITE_READING_DIRECTION readingdirection = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
        DWRITE_WORD_WRAPPING wordwrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
        // ���������з���
        if (str = node.attribute("flowdirection").value()) {
            flowdirection = static_cast<DWRITE_FLOW_DIRECTION>(LongUI::AtoI(str));
        }
        // ���Tab���
        if (str = node.attribute("tabstop").value()) {
            tabstop = LongUI::AtoF(str);
        }
        // ������(��ֱ)����
        if (str = node.attribute("valign").value()) {
            valign = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(LongUI::AtoI(str));
        }
        // ����ı�(ˮƽ)����
        if (str = node.attribute("halign").value()) {
            halign = static_cast<DWRITE_TEXT_ALIGNMENT>(LongUI::AtoI(str));
        }
        // ����Ķ����з���
        if (str = node.attribute("readingdirection").value()) {
            readingdirection = static_cast<DWRITE_READING_DIRECTION>(LongUI::AtoI(str));
        }
        // ����Զ�����
        if (str = node.attribute("wordwrapping").value()) {
            wordwrapping = static_cast<DWRITE_WORD_WRAPPING>(LongUI::AtoI(str));
        }
        // ���ö������з���
        textformat->SetFlowDirection(flowdirection);
        // ����Tab���
        textformat->SetIncrementalTabStop(tabstop);
        // ���ö���(��ֱ)����
        textformat->SetParagraphAlignment(valign);
        // �����ı�(ˮƽ)����
        textformat->SetTextAlignment(halign);
        // �����Ķ����з���
        textformat->SetReadingDirection(readingdirection);
        // �����Զ�����
        textformat->SetWordWrapping(wordwrapping);
    }
    m_textFormats.push_back(textformat);
}

// ��ʽ������
/*
control char    C-Type      Infomation                                  StringInlineParamSupported
  
%%               [none]      As '%' Character(like %% in ::printf)                 ---
%a %A      [const wchar_t*] string add(like %S in ::printf)                Yes but no "," char 

%C              [float4*]    new font color range start                            Yes
%c              [uint32_t]   new font color range start, with alpha                Yes
!! color is also a drawing effect

%d %D         [IUnknown*]    new drawing effect range start                 Yes and Extensible

%S %S            [float]     new font size range start                             Yes

%n %N       [const wchar_t*] new font family name range start               Yes but No "," char

%h %H            [enum]      new font stretch range start                          Yes

%y %Y            [enum]      new font style range start                            Yes

%w %W            [enum]      new font weight range start                           Yes

%u %U            [BOOL]      new underline range start                          Yes(0 or 1)

%e %E            [BOOL]      new strikethrough range start                      Yes(0 or 1)

%i %I            [IDIO*]     new inline object range start                  Yes and Extensible

%] %}            [none]      end of the last range                                 ---
//  Unsupported
%f %F   [UNSPT]  [IDFC*]     new font collection range start                       ---
                                IDWriteFontCollection*

%t %T   [UNSPT]  [IDT*]      new inline object range start                         ---
                                IDWriteTypography*

%t %T   [UNSPT] [char_t*]    new locale name range start                           ---

FORMAT IN STRING
the va_list(ap) can be nullptr while string format
include the PARAMETERS,
using %p or %P to mark PARAMETERS start

*/

// ������ʽ�ı�
auto LongUI::CUIManager::FormatTextCore(
    FormatTextConfig& config,
    const wchar_t* format,
    ...) noexcept->IDWriteTextLayout* {
    va_list ap;
    va_start(ap, format);
    return CUIManager::FormatTextCore(config, format, ap);
    
}
/*
 L"He%llo, World"
*/
#define COLOR8TOFLOAT(a) (static_cast<float>(a)/255.f)

// find next param
template<typename T>
const wchar_t*  __fastcall FindNextToken(T* buffer, const wchar_t* stream, size_t token_num) {
    register wchar_t ch;
    while (ch = *stream) {
        ++stream;
        if (ch == L',' && !(--token_num)) {
            break;
        }
        *buffer = static_cast<T>(ch);
        ++buffer;
    }
    *buffer = 0;
    return stream;
}


#define CUIManager_GetNextTokenW(n) param = FindNextToken(param_buffer, param, n)
#define CUIManager_GetNextTokenA(n) param = FindNextToken(reinterpret_cast<char*>(param_buffer), param, n)


// ������ʽ�ı�
// ��������ʱ�ο�:
// �����ͷ�����(::SafeRelease(layout))
// 1. L"%cHello%], world!%p#FFFF0000"
// Debug    : ѭ�� 1000000(һ����)�Σ���ʱ8750ms(��ȷ��16ms)
// Release  : ѭ�� 1000000(һ����)�Σ���ʱ3484ms(��ȷ��16ms)
// 2. L"%cHello%], world!%cHello%], world!%p#FFFF0000, #FF00FF00"
// Debug    : ѭ�� 1000000(һ����)�Σ���ʱ13922ms(��ȷ��16ms)
// Release  : ѭ�� 1000000(һ����)�Σ���ʱ 6812ms(��ȷ��16ms)
// ����: Release��ÿ����һ���ַ�(������ʽ�����)ƽ������0.12΢��, Debug��ӱ�
// ����: 60Hzÿ֡16ms �ó�8ms��������, ���Դ���6��6���ַ�
//һ����: ������ÿ֡����6����, һ�����ÿ֡���������ַ�(æµʱ), ���Ժ��Բ���
auto  LongUI::CUIManager::FormatTextCore(
    FormatTextConfig& config,
    const wchar_t* format, 
    va_list ap) noexcept->IDWriteTextLayout* {
    const wchar_t* param = nullptr;
    // ����Ƿ������
    if (!ap) {
        register auto format_param_tmp = format;
        register wchar_t ch;
        while (ch = *format_param_tmp) {
            if (ch == L'%') {
                ++format_param_tmp;
                ch = *format_param_tmp;
                if (ch == L'p' || ch == L'p') {
                    param = format_param_tmp + 1;
                    break;
                }
            }
            ++format_param_tmp;
        }
        assert(param && "ap set to nullptr, but none param found.");
    }
    // Color
    union ARGBColor {
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
        uint32_t u32;
    };
    // Range Type
    enum class R :size_t { N, W, Y, H, S, U, E, D, I };
    // Range Data
    struct RangeData {
        DWRITE_TEXT_RANGE       range;
        union {
            const wchar_t*      name;       // N
            DWRITE_FONT_WEIGHT  weight;     // W
            DWRITE_FONT_STYLE   style;      // Y
            DWRITE_FONT_STRETCH stretch;    // H
            float               size;       // S
            BOOL                underline;  // U
            BOOL                strikethr;  // E
            IUnknown*           draweffect; // D
            IDWriteInlineObject*inlineobj;  // I
            // ----------------------------
            D2D1_COLOR_F*       color;      // C
            uint32_t            u32;        // c
            ARGBColor           u32color;   // c
        };
        R                       range_type;
    } range_data;
    assert(format && "bad argument");
    IDWriteTextLayout* layout = nullptr;
    register UIColorEffect* tmp_color = nullptr;
    // �����ַ�������
    uint32_t string_length = 0;
    // ��ǰ�ַ�
    wchar_t ch;
    // ����������
    wchar_t* buffer_index;
    // ����������
    wchar_t param_buffer[256];
    // ������
    wchar_t buffer[LongUIStringBufferLength];
    // ������
    wchar_t fontname_buffer[LongUIStringBufferLength];
    auto fontname_buffer_index = fontname_buffer;
    // ʹ��ջ
    FixedStack<RangeData, 1024> stack_check, statck_set;
    // �������
    buffer_index = buffer;
    // ����
    while (ch = *format) {
        // Ϊ%ʱ, �����һ�ַ�
        if (ch == L'%' && (++format, ch = *format)) {
            switch (ch)
            {
            case L'%':
                // ���%
                *buffer_index = L'%';
                ++buffer_index;
                ++string_length;
                break;
            case L'A': case L'a': // [A]dd string
                // �����ַ���
                {
                    register const wchar_t* i;
                    if (ap) {
                        i = va_arg(ap, const wchar_t*);
                    }
                    else {
                        CUIManager_GetNextTokenW(1);
                        i = param_buffer;
                    }
                    for (; *i; ++i) {
                        *buffer_index = *i;
                        ++string_length;
                        ++range_data.name;
                    }
                }
                break;
            case L'C': // [C]olor in float4
                // ����������ɫ��ʼ���: 
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.color = va_arg(ap, D2D1_COLOR_F*);
                }
                range_data.range_type = R::D;
                // ��̬������ɫЧ��
                tmp_color = UIColorEffect::Create();
                assert(tmp_color && "C");
                // �ӷ�Χ�����л�ȡ
                if (ap) {
                    tmp_color->color = *range_data.color;
                }
                // ֱ������
                else {
                    CUIManager_GetNextTokenA(4);
                    UIControl::MakeColor(reinterpret_cast<char*>(param_buffer), tmp_color->color);
                }
                range_data.draweffect = tmp_color;
                stack_check.push(range_data);
                break;
            case L'c': // [C]olor in uint32
                // 32λ��ɫ��ʼ���: 
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.u32 = va_arg(ap, uint32_t);
                }
                range_data.range_type = R::D;
                // ��̬������ɫЧ��
                tmp_color = UIColorEffect::Create();
                assert(tmp_color && "c");
                if (ap) {
                    tmp_color->color.b = COLOR8TOFLOAT(range_data.u32color.b);
                    tmp_color->color.g = COLOR8TOFLOAT(range_data.u32color.g);
                    tmp_color->color.r = COLOR8TOFLOAT(range_data.u32color.r);
                    tmp_color->color.a = COLOR8TOFLOAT(range_data.u32color.a);
                }
                else {
                    CUIManager_GetNextTokenA(1);
                    UIControl::MakeColor(reinterpret_cast<char*>(param_buffer), tmp_color->color);
                }
                range_data.draweffect = tmp_color;
                stack_check.push(range_data);
                break;
            case 'D': case 'd': // [D]rawing effect
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.draweffect = va_arg(ap, IUnknown*);
                }
                else {
                    CUIManager_GetNextTokenW(1);
                    IUnknown* result = nullptr;
                    if (config.inline_handler) {
                        result = config.inline_handler(0, param_buffer);
                    }
                    range_data.draweffect = result;
                }
                range_data.range_type = R::D;
                stack_check.push(range_data);
                break;
            case 'E': case 'e': // strik[E]through
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.strikethr = va_arg(ap, BOOL);
                }
                else {
                    CUIManager_GetNextTokenA(1);
                    range_data.strikethr = static_cast<BOOL>(
                        LongUI::AtoI(reinterpret_cast<char*>(param_buffer))
                        );
                }
                range_data.range_type = R::E;
                stack_check.push(range_data);
                break;
            case 'H': case 'h': // stretc[H]
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.stretch = va_arg(ap, DWRITE_FONT_STRETCH);
                }
                else {
                    CUIManager_GetNextTokenA(1);
                    range_data.stretch = static_cast<DWRITE_FONT_STRETCH>(
                        LongUI::AtoI(reinterpret_cast<char*>(param_buffer))
                        );
                }
                range_data.range_type = R::H;
                stack_check.push(range_data);
                break;
            case 'I': case 'i': // [I]nline object
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.inlineobj = va_arg(ap, IDWriteInlineObject*);
                }
                else {
                    CUIManager_GetNextTokenW(1);
                    IDWriteInlineObject* result = nullptr;
                    if (config.inline_handler) {
                        result = static_cast<IDWriteInlineObject*>(
                            config.inline_handler(0, param_buffer)
                            );
                    }
                    range_data.inlineobj = result;
                }
                range_data.range_type = R::I;
                stack_check.push(range_data);
                break;
            case 'N': case 'n': // family [N]ame
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.name = va_arg(ap, const wchar_t*);
                }
                else {
                    // ������������ ��ȥ��ǰ��հ�
                    register wchar_t now_ch;
                    auto param_buffer_index = param_buffer;
                    wchar_t* last_firststart_while = nullptr;
                    const wchar_t* firststart_notwhile = nullptr;
                    bool nameless = true;
                    while (now_ch = *param_buffer) {
                        *fontname_buffer_index = now_ch;
                        if (nameless && (now_ch == L' ' || now_ch == L'\t')) {
                            last_firststart_while = fontname_buffer_index;
                            nameless = false;
                        }
                        else {
                            nameless = true;
                            if (!firststart_notwhile) {
                                param_buffer_index = fontname_buffer_index;
                            }
                        }
                        ++fontname_buffer_index;
                    }
                    *last_firststart_while = 0;
                    fontname_buffer_index = last_firststart_while + 1;
                    range_data.name = firststart_notwhile;
                }
                range_data.range_type = R::N;
                stack_check.push(range_data);
                break;
            case 'S': case 's': // [S]ize
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.size = va_arg(ap, float);
                }
                else {
                    CUIManager_GetNextTokenA(1);
                    range_data.size = LongUI::AtoF(
                        reinterpret_cast<char*>(param_buffer)
                        );
                }
                range_data.range_type = R::S;
                stack_check.push(range_data);
                break;
            case 'U': case 'u': // [U]nderline
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.underline = va_arg(ap, BOOL);
                }
                else {
                    CUIManager_GetNextTokenA(1);
                    range_data.underline = static_cast<BOOL>(
                        LongUI::AtoI(reinterpret_cast<char*>(param_buffer))
                        );
                }
                range_data.range_type = R::U;
                stack_check.push(range_data);
                break;
            case 'W': case 'w': // [W]eight
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.weight = va_arg(ap, DWRITE_FONT_WEIGHT);
                }
                else {
                    CUIManager_GetNextTokenA(1);
                    range_data.weight = static_cast<DWRITE_FONT_WEIGHT>(
                        LongUI::AtoI(reinterpret_cast<char*>(param_buffer))
                        );
                }
                range_data.range_type = R::W;
                stack_check.push(range_data);
                break;
            case L'Y': case L'y': // st[Y]le
                range_data.range.startPosition = string_length;
                if (ap) {
                    range_data.style = va_arg(ap, DWRITE_FONT_STYLE);
                }
                else {
                    CUIManager_GetNextTokenA(1);
                    range_data.style = static_cast<DWRITE_FONT_STYLE>(
                        LongUI::AtoI(reinterpret_cast<char*>(param_buffer))
                        );
                }
                range_data.range_type = R::Y;
                stack_check.push(range_data);
                break;
            case L'P': case L'p': // end of main string, then, is the param
                goto force_break;
            case L']': case L'}': // All Range type end
                // ���ջ����
                stack_check.pop();
                // ���㳤��
                stack_check.top->range.length = string_length - stack_check.top->range.startPosition;
                // ѹ������ջ
                statck_set.push(*stack_check.top);
                break;
            }
        }
        // ���
        else {
            *buffer_index = ch;
            ++buffer_index;
            ++string_length;
        }
        ++format;
    }
force_break:
    // β��0
    *buffer_index = 0;
    // ���㳤��
    assert(string_length < lengthof(buffer));
    // ������Ҫ����
    config.text_length = string_length;
    register auto string_length_need = static_cast<uint32_t>(static_cast<float>(string_length + 1) * config.progress);
    LongUIClamp(string_length_need, 0, string_length);
    // ����
    va_end(ap);
    // ��������
    if (config.dw_factory && SUCCEEDED(config.dw_factory->CreateTextLayout(
        buffer, string_length_need, config.text_format, config.width, config.height,&layout
        ))) {
        // ����
        while (!statck_set.empty()) {
            statck_set.pop();
            // ������(progress)��Χ �ͷ�����
            if (statck_set.top->range.startPosition 
                + statck_set.top->range.length > string_length_need) {
                if (statck_set.top->range_type == R::D || statck_set.top->range_type == R::I) {
                    ::SafeRelease(statck_set.top->draweffect);
                }
                continue;
            };
            // enum class R :size_t { N, W, Y, H, S, U, E, D, I };
            switch (statck_set.top->range_type)
            {
            case R::N:
                layout->SetFontFamilyName(statck_set.top->name, statck_set.top->range);
                break;
            case R::W:
                layout->SetFontWeight(statck_set.top->weight, statck_set.top->range);
                break;
            case R::Y:
                layout->SetFontStyle(statck_set.top->style, statck_set.top->range);
                break;
            case R::H:
                layout->SetFontStretch(statck_set.top->stretch, statck_set.top->range);
                break;
            case R::S:
                layout->SetFontSize(statck_set.top->size, statck_set.top->range);
                break;
            case R::U:
                layout->SetUnderline(statck_set.top->underline, statck_set.top->range);
                break;
            case R::E:
                layout->SetStrikethrough(statck_set.top->strikethr, statck_set.top->range);
                break;
            case R::D:
                layout->SetDrawingEffect(statck_set.top->draweffect, statck_set.top->range);
                break;
            case R::I:
                layout->SetInlineObject(statck_set.top->inlineobj, statck_set.top->range);
                break;
            }
        }
    }
    return layout;
}


// ���ļ���ȡλͼ
auto LongUI::CUIManager::LoadBitmapFromFile(
    LongUIRenderTarget *pRenderTarget,
    IWICImagingFactory *pIWICFactory,
    PCWSTR uri,
    UINT destinationWidth,
    UINT destinationHeight,
    ID2D1Bitmap1 **ppBitmap
    ) noexcept -> HRESULT {
    IWICBitmapDecoder *pDecoder = nullptr;
    IWICBitmapFrameDecode *pSource = nullptr;
    IWICStream *pStream = nullptr;
    IWICFormatConverter *pConverter = nullptr;
    IWICBitmapScaler *pScaler = nullptr;

    register HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
        uri,
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder
        );

    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }


    if (SUCCEEDED(hr)) {
        if (destinationWidth != 0 || destinationHeight != 0)  {
            UINT originalWidth, originalHeight;
            hr = pSource->GetSize(&originalWidth, &originalHeight);
            if (SUCCEEDED(hr)) {
                if (destinationWidth == 0) {
                    FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
                    destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
                }
                else if (destinationHeight == 0) {
                    FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
                    destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
                }

                hr = pIWICFactory->CreateBitmapScaler(&pScaler);
                if (SUCCEEDED(hr)) {
                    hr = pScaler->Initialize(
                        pSource,
                        destinationWidth,
                        destinationHeight,
                        WICBitmapInterpolationModeCubic
                        );
                }
                if (SUCCEEDED(hr)) {
                    hr = pConverter->Initialize(
                        pScaler,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapDitherTypeNone,
                        nullptr,
                        0.f,
                        WICBitmapPaletteTypeMedianCut
                        );
                }
            }
        }
        else
        {
            hr = pConverter->Initialize(
                pSource,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.f,
                WICBitmapPaletteTypeMedianCut
                );
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            pConverter,
            nullptr,
            ppBitmap
            );
    }

    ::SafeRelease(pDecoder);
    ::SafeRelease(pSource);
    ::SafeRelease(pStream);
    ::SafeRelease(pConverter);
    ::SafeRelease(pScaler);

    return hr;
}


// ��Ӵ���
void LongUIMethodCall LongUI::CUIManager::AddWindow(UIWindow * wnd) noexcept {
    assert(wnd && "bad argument");
#ifdef _DEBUG
    if (std::find(m_windows.cbegin(), m_windows.cend(), wnd) != m_windows.cend()) {
        assert(!"target window has been added.");
    }
#endif
    try {
        m_windows.push_back(wnd);
    }
    catch (...) {
    }
}

// �Ƴ�����
void LongUIMethodCall LongUI::CUIManager::RemoveWindow(UIWindow * wnd) noexcept {
    assert(wnd && "bad argument");
#ifdef _DEBUG
    if (std::find(m_windows.cbegin(), m_windows.cend(), wnd) == m_windows.cend()) {
        assert(!"target window not in windows vector");
    }
#endif
    try {
        m_windows.erase(std::find(m_windows.cbegin(), m_windows.cend(), wnd));
    }
    catch (...) {
    }
}

// �Ƿ��Թ���ԱȨ������
bool LongUI::CUIManager::IsRunAsAdministrator() noexcept {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    ::AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsGroup
    );
    // �ɹ�? ������Դ
    if (pAdministratorsGroup) {
        ::CheckTokenMembership(nullptr, pAdministratorsGroup, &fIsRunAsAdmin);
        ::FreeSid(pAdministratorsGroup);
        pAdministratorsGroup = nullptr;
    }
    // ���ؽ��
    return fIsRunAsAdmin != 0;
}

// ����Ȩ��
bool LongUI::CUIManager::TryElevateUACNow(const wchar_t* parameters, bool exit) noexcept {
    if (!CUIManager::IsRunAsAdministrator()) {
        wchar_t szPath[MAX_PATH];
        // ��ȡʵ�����
        if (::GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath))) {
            // Launch itself as admin
            SHELLEXECUTEINFO sei = { 0 };
            sei.cbSize = sizeof(sei);
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.lpParameters = parameters;
            sei.hwnd = nullptr;
            sei.nShow = SW_NORMAL;
            // ִ��
            if (!::ShellExecuteExW(&sei)) {
                DWORD dwError = ::GetLastError();
                assert(dwError == ERROR_CANCELLED && "anyelse?");
                return false;
            }
            else if(exit) {
                // �˳�
                UIManager.Exit();
            }
        }
    }
    return true;
}

#include <valarray>

#ifdef _DEBUG

// ����ˢ������
template <>
auto LongUI::CUIManager::operator<<(LongUI::EndL) noexcept ->CUIManager& {
    wchar_t chs[3] = { L'\r',L'\n', 0 }; 
    this->Output(m_lastLevel, chs);
    return *this;
}

template <>
auto LongUI::CUIManager::operator<<(DXGI_ADAPTER_DESC* pdesc) noexcept->CUIManager& {
    const DXGI_ADAPTER_DESC& desc = *pdesc;
    wchar_t buffer[LongUIStringBufferLength];
    ::swprintf(
        buffer, LongUIStringBufferLength,
        L"Adapter:   { \n\t Description: %ls\n\t VendorId:0x%08X"
        L"\t\t DeviceId:0x%08X\n\t SubSysId:0x%08X\t\t Revision:0x%08X\n"
        L"\t DedicatedVideoMemory: %.3lfMB\n"
        L"\t DedicatedSystemMemory: %.3lfMB\n"
        L"\t SharedSystemMemory: %.3lfMB\n"
        L"\t AdapterLuid: 0x%08X--%08X\n }",
        desc.Description,
        desc.VendorId,
        desc.DeviceId,
        desc.SubSysId,
        desc.Revision,
        static_cast<double>(desc.DedicatedVideoMemory) / (1024.*1024.),
        static_cast<double>(desc.DedicatedSystemMemory) / (1024.*1024.),
        static_cast<double>(desc.SharedSystemMemory) / (1024.*1024.),
        desc.AdapterLuid.HighPart,
        desc.AdapterLuid.LowPart
        );
    this->OutputNoFlush(m_lastLevel, buffer);
    return *this;
}

// ���UTF-8�ַ��� ��ˢ��
void LongUI::CUIManager::Output(DebugStringLevel l, const char * s) noexcept {
    wchar_t buffer[LongUIStringBufferLength];
    buffer[LongUI::UTF8toWideChar(s, buffer)] = 0;
    this->Output(m_lastLevel, buffer);
}

// ���UTF-8�ַ���
void LongUI::CUIManager::OutputNoFlush(DebugStringLevel l, const char * s) noexcept {
    wchar_t buffer[LongUIStringBufferLength];
    buffer[LongUI::UTF8toWideChar(s, buffer)] = 0;
    this->OutputNoFlush(m_lastLevel, buffer);
}

// ��������
template <>
auto LongUI::CUIManager::operator<<(float f) noexcept ->CUIManager&  {
    wchar_t buffer[LongUIStringBufferLength];
    ::swprintf(buffer, LongUIStringBufferLength, L"%f", f);
    this->OutputNoFlush(m_lastLevel, buffer);
    return *this;
}

// ��������
template <>
auto LongUI::CUIManager::operator<<(long l) noexcept ->CUIManager& {
    wchar_t buffer[LongUIStringBufferLength];
    ::swprintf(buffer, LongUIStringBufferLength, L"%d", l);
    this->OutputNoFlush(m_lastLevel, buffer);
    return *this;
}


// �������
#define OutputDebug(a, b)\
void LongUI::CUIManager::a(const wchar_t* format, ...) noexcept {\
    wchar_t buffer[LongUIStringBufferLength];\
    va_list argList;\
    va_start(argList, format);\
    auto ret = ::vswprintf(buffer, LongUIStringBufferLength - 1, format, argList);\
    buffer[ret] = 0;\
    va_end(argList);\
    this->Output(b, buffer);\
}

OutputDebug(OutputN, DLevel_None)
OutputDebug(OutputL, DLevel_Log)
OutputDebug(OutputH, DLevel_Hint)
OutputDebug(OutputW, DLevel_Warning)
OutputDebug(OutputE, DLevel_Error)
OutputDebug(OutputF, DLevel_Fatal)

#endif