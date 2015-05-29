#pragma once



// test window ���Դ���
class MainWindow final : public LongUI::UIWindow {
    // super class -- ��������
    typedef LongUI::UIWindow Super;
public:
    // Render ��Ⱦ!
    //virtual HRESULT LongUIMethodCall Render() noexcept override;
    // do event �¼�����
    virtual bool    LongUIMethodCall DoEvent(LongUI::EventArgument&) noexcept override;
    // Ԥ��Ⱦ
    //virtual void    LongUIMethodCall PreRender() noexcept override;
    // recreate �ؽ�
    //virtual HRESULT LongUIMethodCall Recreate(LongUIRenderTarget*) noexcept override;
    // close this control �رտؼ�
    virtual void    LongUIMethodCall Close() noexcept override;
public:
    // ���캯��
    MainWindow(pugi::xml_node , LongUI::UIWindow* );
    // ��������
    ~MainWindow();
    // delete this method , we donn't need it ɾ�����ƹ���
    MainWindow(const MainWindow&) = delete;
private:
    // UAC ��ť����
    bool OnUACButtonOn(UIControl* sender);
private:
    // your own data
    void*           m_pNameless = nullptr;
    // your own data
    void*           m_pNameless1 = nullptr;
    size_t          m_u1 = 123;
    size_t          m_u2 = 456;
    size_t          m_u3 = 789;
};