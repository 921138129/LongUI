#include "stdafx.h"
#include "included.h"


// ����XML &#xD; --> \r &#xA; --> \n
#if 1
const char* test_xml = u8R"xml(<?xml version="1.0" encoding="utf-8"?>
<Window size="1024, 768" name="MainWindow" >
    <VerticalLayout name="VLayout1" pos="0, 0, 512, 128">
        <!--Video name="asd" /-->
        <Test name="test" text="Hello, world!"/>
        <Slider name="6" renderparent="true"/>
    </VerticalLayout>
    <HorizontalLayout name="HLayout" pos="0, 0, 0, 256">
        <EditBasic name="edit01" textmultiline="true" text="Hello, world!&#xD;&#xA;�຾, ����!"/>
        <VerticalLayout name="VLayout2">
            <Label name="2" texttype="core" text="%cHello%], world!�຾!����!%p#F00"/>
            <Button name="4" disabledmeta="1" normalmeta="2" script="App.click_button1($apparg)"
                hovermeta="3" pushedmeta="4" text="Hello, world!"/>
            <CheckBox name="5" text="Hello, world!"/>
            <!--Button name="uac" disabledmeta="1" normalmeta="2" 
                hovermeta="3" pushedmeta="4" text="Try  Elevate UAC Now "/-->
        </VerticalLayout>
    </HorizontalLayout>
</Window>
)xml";
#else
const char* test_xml = u8R"xml(<?xml version="1.0" encoding="utf-8"?>
<Window size="1024, 768" name="MainWindow" >
    <VerticalLayout name="VLayout1">
        <Button name="1" disabledmeta="1" normalmeta="2" hovermeta="3" pushedmeta="4" text="Hello, world!"/>
        <Button name="2" disabledmeta="1" normalmeta="2" hovermeta="3" pushedmeta="4" text="Hello, world!"/>
    </VerticalLayout>
    <HorizontalLayout name="HLayout">
        <Button name="3" disabledmeta="1" normalmeta="2" hovermeta="3" pushedmeta="4" text="Hello, world!"/>
        <Button name="4" disabledmeta="1" normalmeta="2" hovermeta="3" pushedmeta="4" text="Hello, world!"/>
    </HorizontalLayout>
</Window>
)xml";
#endif

constexpr char* res_xml = u8R"xml(<?xml version="1.0" encoding="utf-8"?>
<Resource>
    <!-- Bitmap����Zone -->
    <Bitmap desc="��ť1" res="btn.png"/>
    <!-- Meta����Zone -->
    <Meta desc="��ť1��ЧͼԪ" bitmap="1" rect="0,  0, 96, 24"/>
    <Meta desc="��ť1ͨ��ͼԪ" bitmap="1" rect="0, 72, 96, 96"/>
    <Meta desc="��ť1����ͼԪ" bitmap="1" rect="0, 24, 96, 48"/>
    <Meta desc="��ť1����ͼԪ" bitmap="1" rect="0, 48, 96, 72"/>
</Resource>
)xml";

DWORD color_a; 


// Test UIControl
class TestControl : public LongUI::UIControl {
    // super class define
    typedef LongUI::UIControl Super;
public:
    // create ����
    static UIControl* WINAPI CreateControl(pugi::xml_node node) noexcept {
        if (!node) {
            UIManager << DL_Warning << L"node null" << LongUI::endl;
        }
        // ����ռ�
        auto pControl = LongUI::UIControl::AllocRealControl<TestControl>(
            node,
            [=](void* p) noexcept { new(p) TestControl(node);}
        );
        if (!pControl) {
            UIManager << DL_Error << L"alloc null" << LongUI::endl;
        }
        return pControl;
    }
public:
    // Render This Control
    virtual HRESULT LongUIMethodCall Render() noexcept {
        D2D1_RECT_F draw_rect = GetDrawRect(this);
        D2D1_COLOR_F color = D2D1::ColorF(0xfcf7f4);
        m_pBrush_SetBeforeUse->SetColor(&color);
        m_pRenderTarget->FillRectangle(&draw_rect, m_pBrush_SetBeforeUse);
        color.a = float(reinterpret_cast<uint8_t*>(&color_a)[3]) / 255.f;
        color.r = float(reinterpret_cast<uint8_t*>(&color_a)[2]) / 255.f;
        color.g = float(reinterpret_cast<uint8_t*>(&color_a)[1]) / 255.f;
        color.b = float(reinterpret_cast<uint8_t*>(&color_a)[0]) / 255.f;
        m_pBrush_SetBeforeUse->SetColor(&color);
        m_pRenderTarget->FillRectangle(&draw_rect, m_pBrush_SetBeforeUse);
        m_pRenderTarget->DrawImage(m_pEffectOut);
        return S_OK;
    }
    //do the event
    virtual bool    LongUIMethodCall DoEvent(LongUI::EventArgument& arg) noexcept {
        if (arg.sender) {
            if (arg.event == LongUI::Event::Event_FindControl &&
                LongUI::IsPointInRect(this->show_zone, arg.pt)) {
                arg.ctrl = this;
            }
            else if (arg.event == LongUI::Event::Event_FinishedTreeBuliding) {
                // ע���¼�
                this->SetEventCallBackT(L"6", LongUI::Event::Event_SliderValueChanged, &TestControl::OnValueChanged);
            }
        }
        return false;
    }
    // prerender
    virtual void    LongUIMethodCall PreRender() noexcept {
        if (m_bDrawSizeChanged) {
            this->draw_zone = this->show_zone;
        }
        D2D1_RECT_F draw_rect = GetDrawRect(this);
        // ��鲼��
        if (m_bDrawSizeChanged) {
            ::SafeRelease(m_pCmdList);
            m_pRenderTarget->CreateCommandList(&m_pCmdList);
            // ���ô�С
            m_text.SetNewSize(this->draw_zone.width, this->draw_zone.height);
            // ��Ⱦ����
            m_pRenderTarget->SetTarget(m_pCmdList);
            m_pRenderTarget->BeginDraw();
            m_text.Render(draw_rect.left, draw_rect.top);
            m_pRenderTarget->EndDraw();
            m_pCmdList->Close();
            // ����Ϊ����
            m_pEffect->SetInput(0, m_pCmdList);
        }
    }

    // recreate resource
    virtual HRESULT LongUIMethodCall Recreate(LongUIRenderTarget* target) noexcept {
        ::SafeRelease(m_pEffectOut);
        ::SafeRelease(m_pEffect);
        // ������Ч
        target->CreateEffect(CLSID_D2D1GaussianBlur, &m_pEffect);
        assert(m_pEffect);
        // ����ģ������
        m_pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_GAUSSIANBLUR_OPTIMIZATION_QUALITY );
        m_pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, 0.f  );
        // ��ȡ���
        m_pEffect->GetOutput(&m_pEffectOut);
        // ��ע����ע��
        if (m_FirstRecreate) {
            m_pWindow->RegisterPreRender2D(this);
            m_FirstRecreate = false;
        }
        return Super::Recreate(target);
    }
    // On Value Changed
    bool LongUIMethodCall OnValueChanged(UIControl* control) {
        register auto value = static_cast<LongUI::UISlider*>(control)->GetValue();
        m_pEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, value * 10.f);
        return true;
    }
    // close this control
    virtual void    LongUIMethodCall Close() noexcept { delete this; };
protected:
    // constructor
    TestControl(pugi::xml_node node) noexcept : Super(node), m_text(node){
    }
    // destructor
    ~TestControl() {
        ::SafeRelease(m_pCmdList);
        ::SafeRelease(m_pEffectOut);
        ::SafeRelease(m_pEffect);
    }
protected:
    // text
    LongUI::UIText              m_text;
    // bool
    bool                        m_FirstRecreate = true;
    //
    bool                test_unused[sizeof(void*) / sizeof(bool) - 2];
    // command list
    ID2D1CommandList*           m_pCmdList = nullptr;
    // effect
    ID2D1Effect*                m_pEffect = nullptr;
    // effect output
    ID2D1Image*                 m_pEffectOut = nullptr;
};



// Test Video Control
class UIVideoAlpha : public LongUI::UIControl {
    // super class define
    typedef LongUI::UIControl Super;
public:
    // create ����
    static UIControl* WINAPI CreateControl(pugi::xml_node node) noexcept {
        if (!node) {
            UIManager << DL_Warning << L"node null" << LongUI::endl;
        }
        // ����ռ�
        auto pControl = LongUI::UIControl::AllocRealControl<UIVideoAlpha>(
            node,
            [=](void* p) noexcept { new(p) UIVideoAlpha(node); }
            );
        if (!pControl) {
            UIManager << DL_Error << L"alloc null" << LongUI::endl;
        }
        return pControl;
    }
public:
    // Render This Control
    virtual HRESULT LongUIMethodCall Render() noexcept {
        if (m_bDrawSizeChanged) {
            this->draw_zone = this->show_zone;
        }
        D2D1_RECT_F draw_rect = GetDrawRect(this);
        m_video.Render(&draw_rect);
        m_pWindow->StartRender(1.f, this);
        return S_OK;
    }
    //do the event
    virtual bool    LongUIMethodCall DoEvent(LongUI::EventArgument& arg) noexcept {
        if (arg.sender) {
            if (arg.event == LongUI::Event::Event_FindControl &&
                LongUI::IsPointInRect(this->show_zone, arg.pt)) {
                arg.ctrl = this;
            }
            else if (arg.event == LongUI::Event::Event_FinishedTreeBuliding) {
            }
        }
        return false;
    }
    // recreate resource
    virtual HRESULT LongUIMethodCall Recreate(LongUIRenderTarget* target) noexcept {
        // �ؽ���Ƶ
        register auto hr = m_video.Recreate(target);
        // �ؽ�����
        if (SUCCEEDED(hr)) {
            hr = Super::Recreate(target);
        }
        return hr;
    }
    // close this control
    virtual void    LongUIMethodCall Close() noexcept { delete this; };
protected:
    // constructor
    UIVideoAlpha(pugi::xml_node node) noexcept : Super(node) {
        m_video.Init();
        auto re = m_video.HasVideo();
        auto hr = m_video.SetSource(L"arcv45.mp4");
    }
    // destructor
    ~UIVideoAlpha() {
    }
protected:
    // video
    LongUI::CUIVideoComponent       m_video;
};


// Ӧ�ó������
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow) {
    DWORD colors[32];
    const DWORD ycolor = (237 << 16) | (222 << 8) | (105);
    for (int i = 0; i < lengthof(colors); ++i) {
        colors[i] = ::GetSysColor(i);
    }
    DWORD buffer_size = sizeof DWORD;
    auto error_code = ::RegGetValueA(
        HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\DWM",
        "ColorizationColor",
        RRF_RT_DWORD,
        nullptr,
        &color_a,
        &buffer_size
        );
    ::HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
    // configure for this demo
    class DemoConfigure final : public LongUI::CUIDefaultConfigure {
        typedef LongUI::CUIDefaultConfigure Super;
    public:
        // ���캯��
        DemoConfigure() : Super() { this->script = &mruby; this->resource = res_xml; }
        // ��ȡ��������
        auto GetLocaleName(wchar_t name[/*LOCALE_NAME_MAX_LENGTH*/]) noexcept->void override {
            ::wcscpy(name, L"en-us");
        };
        // ����Զ���ؼ�
        auto AddCustomControl(LongUI::CUIManager& manager) noexcept->void override{
            manager.AddS2CPair(L"Test", TestControl::CreateControl);
            manager.AddS2CPair(L"Video", UIVideoAlpha::CreateControl);
        };
        // ʹ��CPU��Ⱦ
        auto IsRenderByCPU() noexcept ->bool override { return false; }
    private:
        // mruby script
        MRubyScript     mruby;
    } config;
    //
    // Buffer of MainWindow, align for 4(x86)
    alignas(sizeof(void*)) size_t buffer[sizeof(MainWindow) / sizeof(size_t) + 1];
    // ��ʼ�� OLE (OLE�����CoInitializeEx��ʼ��COM)
    if (SUCCEEDED(::OleInitialize(nullptr))) {
        // ��ʼ�� ���ڹ����� 
        UIManager.Initialize(&config);
        // ��ս��������!
        UIManager << DL_Hint << L"Battle Control Online!" << LongUI::endl;
        // ����������
        UIManager.CreateUIWindow<MainWindow>(test_xml, buffer);
        // ���б�����
        UIManager.Run();
        // ��ս������ֹ!
        UIManager << DL_Hint << L"Battle Control Terminated!" << LongUI::endl;
        // ����ʼ�� ���ڹ�����
        UIManager.UnInitialize();
        // ����ʼ�� COM �� OLE
        ::OleUninitialize(); 
    }
    return EXIT_SUCCESS;
}
