#include <control/ui_label.h>
#include <control/ui_scale.h>
#include <control/ui_viewport.h>


/// <summary>
/// </summary>
/// <param name="viewport">The viewport.</param>
/// <returns></returns>
void InitViewport_Scale(LongUI::UIViewport& viewport) noexcept {
    using namespace LongUI;
    auto& window = viewport.RefWindow();
    const auto display = longui_cast<UILabel*>(window.FindControl("display"));
    const auto progress = longui_cast<UIScale*>(window.FindControl("slider"));
    progress->AddGuiEventListener(
        UIScale::_onChange(), [=](const LongUI::GuiEventArg&) noexcept {
        const auto text = CUIString::FromDouble(progress->GetValue(), 0, 2, '.');
        display->SetText(text.view());
        return Event_Accept;
    });
    // force trigger
    //progress->FireEvent(UIScale::_onChange());
}