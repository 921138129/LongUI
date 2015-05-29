#pragma once


// MRuby�ű�
class MRubyScript final : public LongUI::IUIScript {
    // ����
    /*enum ArgumentIndex : uint32_t {
        // �¼�����
        Index_UIEventType = 0,
        // �ؼ�ָ��
        Index_ControlPointer,
        // ������С
        Index_Count,
    };*/
public:
    // ���캯��
    MRubyScript() noexcept;
    // ������������
    ~MRubyScript() noexcept {}
public:
    // �ͷŽű�
    virtual auto Release() noexcept -> int32_t { return 0; };
    // ���нű�
    virtual auto Evaluation(const LongUI::UIScript, const LongUI::EventArgument&) noexcept->size_t;
    // ��ȡ������Ϣ
    virtual auto GetConfigInfo() noexcept->LongUI::ScriptConfigInfo { return LongUI::Info_None; };
    // ��ʼ����
    virtual auto Initialize(LongUI::CUIManager*) noexcept ->bool ;
    // ����ʼ��
    virtual auto UnInitialize() noexcept->void;
    // ���벢��д�ű��ռ�
    virtual auto AllocScript(const char*) noexcept->LongUI::UIScript ;
    // �ͷŽű��ռ�
    virtual auto FreeScript(LongUI::UIScript&) noexcept->void ;
public:
    // ��ȡ��ID
    auto GetClassID(const char*) noexcept->size_t;
private:
    // ���� API �ӿ�
    bool define_api() noexcept;
private:
    // ��ǰ����
    static HWND s_hNowWnd;
    // API.msg_box
    static auto MsgBox(mrb_state *mrb, mrb_value self) noexcept->mrb_value;
private:
    // LongUI ������
    LongUI::CUIManager*             m_pUIManager = nullptr;
    // MRuby ״̬(�����)
    mrb_state*                      m_pMRuby = nullptr;
    // ȫ�ֲ�������
    mrb_sym                         m_symArgument = 0;
};