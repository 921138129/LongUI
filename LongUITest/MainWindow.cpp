#include "stdafx.h"
#include "included.h"


// ���캯��
MainWindow::MainWindow(pugi::xml_node node, LongUI::UIWindow* p) :Super(node, p) {

}


// ��������
MainWindow::~MainWindow(){

}

// UAC ��ť����
bool MainWindow::OnUACButtonOn(UIControl * sender) {
    // ��ȡUACȨ��
    register auto re = LongUI::CUIManager::TryElevateUACNow();
    return re;
}

// �رմ���
void LongUIMethodCall MainWindow::Close() noexcept {
    // ��������
    //operator delete(this, this);
    this->~MainWindow();
    UIManager.Exit();
}

// �¼�
bool LongUIMethodCall MainWindow::DoEvent(LongUI::EventArgument& arg) noexcept {
    // if sender is valid, it's some events need window to handle
    // ���sender��Ч, ˵����Ҫ�����ڴ������Ϣ
    if (arg.sender){
        bool done = false;
        switch (arg.event)
        {
        case LongUI::Event::Event_ButtoClicked:
            UIManager << DL_Hint << L"Button Clicked!@" << arg.sender->GetNameStr() << LongUI::endl;
            done = true;
            break;
        //case LongUI::Event::Event_FinishedTreeBuliding:
            //this->SetEventCallBackT(L"uac", LongUI::Event::Event_ButtoClicked, &MainWindow::OnUACButtonOn);
        }

        if(done) return true;
        // ������Ϣ�ɸ��ദ��
    }
    // if null,it's a system message, you can handle msg that you interested in, 
    // or, send it to super class, super class will handle it
    // ����, ֤��Ϊϵͳ��Ϣ, ����Դ���һЩ����Ȥ����Ϣ,
    // ����, �������ദ��
    return Super::DoEvent(arg);
}