﻿// Gui
#include <luiconf.h>
#include <core/ui_window.h>
#include <core/ui_string_view.h>
#include <util/ui_unicode.h>
#include <core/ui_manager.h>
#include <core/ui_string.h>
#include <style/ui_native_style.h>
#include <input/ui_kminput.h>
#include <style/ui_ssvalue.h>
#include <graphics/ui_dcomp.h>
#include <filesystem/ui_file.h>
#include <core/ui_color_list.h>
#include <container/pod_hash.h>
#include <graphics/ui_cursor.h>
#include <control/ui_viewport.h>
#include <core/ui_popup_window.h>
#include <util/ui_color_system.h>
#include <graphics/ui_graphics_decl.h>
// private
#include "../private/ui_private_control.h"


// C++
#include <cmath>
#include <cassert>
#include <algorithm>

// Windows
#define NOMINMAX
#include <Windows.h>
#include <Dwmapi.h>
//#include <ShellScalingApi.h>

#ifndef NDEBUG
#include <util/ui_time_meter.h>
#endif

#ifdef LUI_ACCESSIBLE
#include <accessible/ui_accessible_win.h>
#endif

// error beep
extern "C" void longui_error_beep() noexcept;

// LongUI::impl
namespace LongUI { namespace impl {
    // get dpi scale
    auto get_dpi_scale_from_hwnd(HWND hwnd) noexcept -> Size2F;
    // get subpixcel rendering level
    void get_subpixel_text_rendering(uint32_t&) noexcept;
    // eval script for window
    void eval_script_for_window(U8View view, CUIWindow* window) noexcept {
        assert(window && "eval script but no window");
        UIManager.Evaluation(view, *window);
    }
    // mark two rect
    inline auto mark_two_rect_dirty(
        const RectF& a,
        const RectF& b,
        const Size2F size,
        RectF* output
    ) noexcept {
        const auto is_valid = [size](const RectF& rect) noexcept {
            return LongUI::GetArea(rect) > 0.f
                && rect.right > 0.f
                && rect.bottom > 0.f
                && rect.left < size.width
                && rect.top < size.height
                ;
        };
        // 判断有效性
        const auto av = is_valid(a);
        const auto bv = is_valid(b);
        // 都无效
        if (!av && !bv) return output;
        // 默认处理包含仅有A有效的情况
        RectF merged_rect = a;
        // 都有效
        if (av && bv) {
            // 存在交集
            if (LongUI::IsOverlap(a, b)) {
                merged_rect.top = std::min(a.top, b.top);
                merged_rect.left = std::min(a.left, b.left);
                merged_rect.right = std::max(a.right, b.right);
                merged_rect.bottom = std::max(a.bottom, b.bottom);
            }
            // 没有交集
            else { *output = b; ++output; }
        }
        // 只有B有效
        else if (bv) merged_rect = b;
        // 标记
        *output = merged_rect; ++output;
        return output;
    }
}}

// LongUI::detail
namespace LongUI { namespace detail {
    /// <summary>
    /// Lowests the common ancestor.
    /// </summary>
    /// <param name="now">The now.</param>
    /// <param name="old">The old.</param>
    /// <returns></returns>
    UIControl* lowest_common_ancestor(UIControl* now, UIControl* old) noexcept {
        /* 
            由于控件有深度信息, 所以可以进行优化
            时间复杂度 O(q) q是目标解与最深条件节点之间深度差
        */
        // now不能为空
        assert(now && "new one cannot be null");
        // old为空则返回now
        if (!old) return now;
        // 连接到相同深度
        UIControl* upper, *lower;
        if (now->GetLevel() < old->GetLevel()) {
            // 越大就是越低
            lower = old; upper = now;
        }
        else {
            // 越小就是越高
            lower = now; upper = old;
        }
        // 将低节点调整至于高节点同一水平
        auto adj = lower->GetLevel() - upper->GetLevel();
        while (adj) {
            lower = lower->GetParent();
            --adj;
        }
        // 共同遍历
        while (upper != lower) {
            assert(upper->IsTopLevel() == false);
            assert(lower->IsTopLevel() == false);
            upper = upper->GetParent();
            lower = lower->GetParent();
        }
        assert(upper && lower && "cannot be null");
        return upper;
    }
    // 获取透明窗口适合的大小
    inline auto get_fit_size_for_trans(uint32_t len) noexcept {
        constexpr uint32_t UNIT = TRANSPARENT_WIN_BUFFER_UNIT;
        return static_cast<uint32_t>(len + UNIT - 1) / UNIT * UNIT;
    }
}}

/// <summary>
/// Sets the control world changed.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetControlWorldChanged(UIControl& ctrl) noexcept {
    assert(ctrl.GetWindow() == this);
    m_pMiniWorldChange = detail::lowest_common_ancestor(&ctrl, m_pMiniWorldChange);
    //LUIDebug(Log) << ctrl << ctrl.GetLevel() << endl;
}

/// <summary>
/// Deletes the later.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::DeleteLater() noexcept {
    const auto view = &this->RefViewport();
    view->DeleteLater();
}

/// <summary>
/// Deletes this instance.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Delete() noexcept {
    const auto view = &this->RefViewport();
    delete view;
}



// ui namespace
namespace LongUI {
    // dirty rect
    struct DirtyRect {
        // control
        UIControl*          control;
        // rectangle
        RectF               rectangle;
    };
#ifndef LUI_DISABLE_STYLE_SUPPORT
    // make style sheet
    auto MakeStyleSheet(U8View view, SSPtr old) noexcept->SSPtr;
    // make style sheet
    auto MakeStyleSheetFromFile(U8View view, SSPtr old) noexcept->SSPtr;
#endif

    /// <summary>
    /// Gets the size of the client.
    /// </summary>
    /// <param name="hwnd">The HWND.</param>
    /// <returns></returns>
    auto GetClientSize(HWND hwnd) noexcept -> Size2U {
        RECT client_rect; ::GetClientRect(hwnd, &client_rect);
        return {
            uint32_t(client_rect.right - client_rect.left),
            uint32_t(client_rect.bottom - client_rect.top)
        };
    }
    /// <summary>
    /// private data for CUIWindow
    /// </summary>
    class CUIWindow::Private : public CUIObject {
        // dcomp
        using dcomp = impl::dcomp_window_buf;
        // window list
        //using Windows = POD::Vector<CUIWindow*>;
        // release data
        void release_data() noexcept;
        // release common tooltip
        void check_cmntlp() noexcept { common_tooltip = nullptr; }
        // begin render
        void begin_render() const noexcept;
        // begin render
        auto end_render() const noexcept->Result;
        // render caret
        void render_caret(I::Renderer2D&) const noexcept;
        // render focus rect
        void render_focus(I::Renderer2D&) const noexcept;
        // clean up
        //void cleanup() noexcept;
    public:
        // destroy window only
        static void Destroy(HWND hwnd, bool acc) noexcept;
    public:
        // empty render
        void EmptyRender() noexcept;
        // reelase device data
        void ReleaseDeviceData() noexcept { this->release_data();  }
        // ctor
        Private() noexcept;
        // dtor
        ~Private() noexcept { this->release_data(); this->check_cmntlp(); }
        // init window pos
        void InitWindowPos() noexcept;
        // init
        HWND Init(HWND parent, CUIWindow::WindowConfig config) noexcept;
        // recreate_device
        auto Recreate(HWND hwnd) noexcept->Result;
        // render
        auto Render(const UIViewport& v) const noexcept->Result;
        // mark dirt rect
        void MarkDirtRect(const DirtyRect& rect) noexcept;
        // before render
        void BeforeRender(Size2F) noexcept;
        // close popup
        void ClosePopup() noexcept;
        // set common tooltip text
        void SetTooltipText(CUIString&&)noexcept;
        // set dcomp support
        void SetLayeredWindowSupport() noexcept;
    public:
        // mouse event[thread safe]
        void DoMouseEventTs(const MouseEventArg& args) noexcept;
        // do msg
        auto DoMsg(HWND, UINT, WPARAM, LPARAM) noexcept ->intptr_t;
        // do msg
        static auto DoMsgNull(HWND, UINT, WPARAM, LPARAM) noexcept->intptr_t;
        // when create
        void OnCreate(HWND) noexcept {}
        // when resize[thread safe]
        void OnResizeTs(Size2U) noexcept;
        // when key down/ up
        void OnKeyDownUp(InputEvent,CUIInputKM::KB key) noexcept;
        // when system key down
        void OnSystemKeyDown(CUIInputKM::KB key, uintptr_t lp) noexcept;
        // when input a utf-16 char
        void OnChar(char16_t ch) noexcept;
        // when input a utf-32 char[thread safe]
        void OnCharTs(char32_t ch) noexcept;
        // do access key
        void OnAccessKey(uintptr_t id) noexcept;
        // on dpi changed
        void OnDpiChanged(uintptr_t, const RectL& rect) noexcept;
        // IME postion
        bool OnIME(HWND hwnd) const noexcept;
    public:
        // full-rendering this frame?
        bool is_fr_for_update() const noexcept { return full_rendering_update; }
        // mark as full-rendering
        void mark_fr_for_update() noexcept { full_rendering_update = true; }
        // clear full-rendering 
        void clear_fr_for_update() noexcept { full_rendering_update = false; }
    private:
        // full-rendering this frame?
        bool is_fr_for_render() const noexcept { return dirty_count_presenting == DIRTY_RECT_COUNT + 1; }
        // will render in this frame?
        auto is_r_for_render() const noexcept { return dirty_count_presenting; }
        // mark as full-rendering
        void mark_fr_for_render() noexcept { dirty_count_presenting = DIRTY_RECT_COUNT + 1; }
        // mark as full-rendering
        void clear_fr_for_render() noexcept { dirty_count_presenting = 0; }
        // using direct composition?
        bool is_direct_composition() const noexcept { return dcomp_support; }
        // is skip render?
        bool is_skip_render() const noexcept { return system_skip_rendering; }
    public:
        // register the window class
        static void RegisterWindowClass() noexcept;
        // viewport
        auto viewport() noexcept { 
            const auto ptr = reinterpret_cast<char*>(this);
            const auto offset = offsetof(CUIWindow, m_private);
            const auto window = reinterpret_cast<CUIWindow*>(ptr - offset);
            assert(this && window->pimpl() == (void*)this);
            return &window->RefViewport();
        }
    public:
        // swap chian
        I::Swapchan*    swapchan = nullptr;
        // bitmap buffer
        I::Bitmap*      bitmap = nullptr;
        // focused control
        UIControl*      focused = nullptr;
        // captured control
        UIControl*      captured = nullptr;
        // caret display control
        UIControl*      careted = nullptr;
        // now default control
        UIControl*      now_default = nullptr;
        // window default control
        UIControl*      wnd_default = nullptr;
        // focus list
        ControlNode     focus_list = { nullptr };
        // named list
        ControlNode     named_list = { nullptr };
        // popup window
        CUIWindow*      popup = nullptr;
        // common tooltip viewport
        UIViewport*     common_tooltip = nullptr;
        // now cursor
        CUICursor       cursor = { CUICursor::Cursor_Arrow };
        // dcomp
        dcomp           dcomp_buf;
        // window clear color
        ColorF          clear_color = ColorF::FromRGBA_CT<RGBA_TianyiBlue>();
        // rect of caret
        RectF           caret = {};
        // rect of foucs area
        RectF           foucs = {};
        // color of caret
        ColorF          caret_color = ColorF::FromRGBA_CT<RGBA_Black>();
        // rect of window
        RectWHL         rect = {};
        // window adjust
        RectL           adjust = {};
        // window buffer logical size
        Size2L          wndbuf_logical = {};
#ifndef NDEBUG
        // full render counter
        uint32_t        dbg_full_render_counter = 0;
        // dirty render counter
        uint32_t        dbg_dirty_render_counter = 0;
#endif
    public:
        // title name
        CUIString       titlename;
        // mouse track data
        alignas(void*) TRACKMOUSEEVENT track_mouse;
        // get first
        auto GetFirst() noexcept { return &this->focused; }
        // get last
        auto GetLast() noexcept { return &this->wnd_default; }
    public:
        // text render type
        uint32_t        text_antialias = 0;
        // auto sleep count
        uint32_t        auto_sleep_count = 0;
        // popup type
        PopupType       popup_type = PopupType::Type_Exclusive;
        // ime input count
        uint16_t        ime_count = 0;
        // save utf-16 char
        char16_t        saved_utf16 = 0;
        // ma return code
        uint8_t         ma_return_code = 3;
    public:
        // sized
        bool            flag_sized : 1;
        // mouse enter
        bool            mouse_enter : 1;
        // dcomp support
        bool            dcomp_support : 1;
        // accessibility called
        bool            accessibility : 1;
        // moving or resizing
        bool            moving_resizing : 1;
        // mouse left down
        bool            mouse_left_down : 1;
        // skip window via system
        bool            system_skip_rendering : 1;
        // layered window support
        bool            layered_window_support : 1;
    public:
        // focus on
        bool            focus_ok : 1;
        // caret on
        bool            caret_ok : 1;
    public:
        // visible window
        bool            window_visible = false;
        // access key display
        bool            access_key_display = false;
        // full renderer in update
        bool            full_rendering_update = false;
    public:
        // dirty count for recording
        uint32_t        dirty_count_recording = 0;
        // dirty count for presenting
        uint32_t        dirty_count_presenting = 0;
        // dirty rect for recording
        DirtyRect       dirty_rect_recording[LongUI::DIRTY_RECT_COUNT];
        // dirty rect for presenting [+ 2 for safty]
        RectF           dirty_rect_presenting[LongUI::DIRTY_RECT_COUNT+2];
    public:
        // access key map
        UIControl*      access_key_map['Z' - 'A' + 1];
    private:
        // toggle access key display
        void toggle_access_key_display() noexcept;
        // resize window buffer
        void resize_window_buffer() noexcept;
        // forece resize
        void force_resize_window_buffer() const noexcept {
            const_cast<Private*>(this)->resize_window_buffer(); 
        }
    public:
        // 处理函数
        static LRESULT WINAPI WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept {
            // 创建窗口时设置指针
            if (message == WM_CREATE) {
                // 获取指针
                auto* window = reinterpret_cast<CUIWindow::Private*>(
                    (reinterpret_cast<LPCREATESTRUCT>(lParam))->lpCreateParams
                    );
                // 设置窗口指针
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, LONG_PTR(window));
                // TODO: 创建完毕
                window->OnCreate(hwnd);
                // 返回1
                return 1;
            }
            // 其他情况则获取储存的指针
            else {
                const auto lptr = ::GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                const auto window = reinterpret_cast<CUIWindow::Private*>(lptr);
                if (!window) return DoMsgNull(hwnd, message, wParam, lParam);
                return window->DoMsg(hwnd, message, wParam, lParam );
            }
        };
    };
    /// <summary>
    /// Privates the window.
    /// </summary>
    /// <returns></returns>
    LongUI::CUIWindow::Private::Private() noexcept : titlename(u"LUI"_sv) {
        flag_sized = false;
        mouse_enter = false;
        dcomp_support = false;
        accessibility = false;
        moving_resizing = false;
        mouse_left_down = false;
        system_skip_rendering = false;
        layered_window_support = false;
        focus_ok = false;
        caret_ok = false;
        std::memset(access_key_map, 0, sizeof(access_key_map));
        impl::init_dcomp(dcomp_buf);
        //sizeof(*this)
        using ty = detail::private_window<sizeof(void*)>;
        //sizeof(Private);
        static_assert(sizeof(Private) <= ty::size, "buffer not safe");
        static_assert(alignof(Private) <= ty::align, "buffer not safe");
    }
}


/// <summary>
/// update focus rect
/// </summary>
/// <param name="cfg">The config.</param>
/// <param name="parent">The parent.</param>
void LongUI::CUIWindow::UpdateFocusRect(const RectF& rect) noexcept {
    pimpl()->foucs = rect;
}

/// <summary>
/// Initializes a new instance of the <see cref="CUIWindow" /> class.
/// </summary>
/// <param name="cfg">The config.</param>
/// <param name="parent">The parent.</param>
LongUI::CUIWindow::CUIWindow(CUIWindow* parent, WindowConfig cfg) noexcept :
    m_pParent(parent), config(cfg) {
    this->prev = this->next = nullptr;
    // 创建私有对象
    detail::ctor_dtor<Private>::create(&m_private);
    // 节点
    m_oHead = { nullptr, static_cast<CUIWindow*>(&m_oTail) };
    m_oTail = { static_cast<CUIWindow*>(&m_oHead), nullptr };
    m_oListNode = { nullptr, nullptr };
    // 初始化BF
    m_inDtor = false;
    m_bInExec = false;
    //m_bBigIcon = false;
    //m_bCtorFaild = false;
    // XXX: 错误处理
    //if (!m_private) { m_bCtorFaild = true; return;}
    // 子像素渲染
    if (UIManager.flag & ConfigureFlag::Flag_SubpixelTextRenderingAsDefault)
        impl::get_subpixel_text_rendering(pimpl()->text_antialias);
    // XXX: 内联窗口的场合
    // 添加分层窗口支持
    if ((cfg & Config_LayeredWindow)) 
        pimpl()->SetLayeredWindowSupport();
    // 初始化

    // TODO: 初始化默认大小位置
    pimpl()->InitWindowPos();
    // XXX: 自动睡眠?
    //if (this->IsAutoSleep()) {

    //}
}


/// <summary>
/// Initializes this instance.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::init() noexcept {
    // 存在父窗口则加入父窗口
    if (m_pParent) m_pParent->add_child(*this);
    // 否则加入窗口管理器的顶层管理
    else UIManager.add_topwindow(*this);
    // 为了方便遍历, 添加到全窗口列表
    UIManager.add_to_allwindow(*this);
}


#ifndef LUI_DISABLE_STYLE_SUPPORT
/// <summary>
/// Makes the style sheet from file.
/// </summary>
/// <param name="file">The file.</param>
/// <param name="old">The old.</param>
/// <returns></returns>
auto LongUI::MakeStyleSheetFromFile(U8View file, SSPtr old) noexcept -> SSPtr {
    const CUIStringU8 old_dir = UIManager.GetXulDir();
    auto path = old_dir; path += file;
    // 获取CSS目录以便获取正确的文件路径
    const auto view = LongUI::FindLastDir(path.view());
    // 设置CSS目录作为当前目录
    UIManager.SetXulDir(view);
    // 待使用缓存
    POD::Vector<uint8_t> css_buffer;
    // 载入文件
    UIManager.LoadDataFromUrl(path.view(), luiref css_buffer);
    // 字符缓存有效
    if (const auto len = css_buffer.size()) {
        const auto bptr = &*css_buffer.cbegin();
        const auto ptr0 = reinterpret_cast<const char*>(bptr);
        const U8View view{ ptr0, ptr0 + len };
        old = LongUI::MakeStyleSheet(view, old);
    }
    // 设置之前的目录作为当前目录
    UIManager.SetXulDir(old_dir.view());
    return old;
}

/// <summary>
/// Loads the CSS file.
/// </summary>
/// <param name="file">The file.</param>
/// <returns></returns>
void LongUI::CUIWindow::LoadCssFile(U8View file) noexcept {
    m_pStyleSheet = LongUI::MakeStyleSheetFromFile(file, m_pStyleSheet);
}

/// <summary>
/// Loads the CSS string.
/// </summary>
/// <param name="string">The string.</param>
/// <returns></returns>
void LongUI::CUIWindow::LoadCssString(U8View string) noexcept {
    m_pStyleSheet = LongUI::MakeStyleSheet(string, m_pStyleSheet);
}

#endif


/// <summary>
/// Adds the child.
/// </summary>
/// <param name="child">The child.</param>
/// <returns></returns>
void LongUI::CUIWindow::add_child(CUIWindow& child) noexcept {
    // TODO: 在之前的父控件移除该控件
    assert(child.GetParent() == this && this);

    // 连接前后节点
    m_oTail.prev->next = &child;
    child.prev = m_oTail.prev;
    child.next = static_cast<CUIWindow*>(&m_oTail);
    m_oTail.prev = &child;
}

/// <summary>
/// Finalizes an instance of the <see cref="CUIWindow"/> class.
/// </summary>
/// <returns></returns>
LongUI::CUIWindow::~CUIWindow() noexcept {
    // 需要即时修改节点信息, 只能上超级锁了
    UIManager.DataLock();
    UIManager.RenderLock();
#ifdef LUI_ACCESSIBLE
    if (m_pAccessible) {
        LongUI::FinalizeAccessible(*m_pAccessible);
        m_pAccessible = nullptr;
    }
#endif
#ifndef LUI_DISABLE_STYLE_SUPPORT
    // 释放样式表
    LongUI::DeleteStyleSheet(m_pStyleSheet);
    m_pStyleSheet = nullptr;
#endif
    // 有脚本
    if (this->custom_script) UIManager.FinalizeScript(*this);
    // 析构中
    m_inDtor = true;
    // 未初始化就被删除的话节点为空
    if (this->prev) {
        // 连接前后节点
        this->prev->next = this->next;
        this->next->prev = this->prev;
        this->prev = this->next = nullptr;
    }
    UIManager.remove_from_allwindow(*this);
    // 存在父窗口
    //m_pParent->remove_child(*this);
    // 有效私有数据
    {
        // 弹出窗口会在下一步删除
        pimpl()->popup = nullptr;
        // XXX: 删除自窗口?
        while (m_oHead.next != &m_oTail) 
            m_oTail.prev->Delete();
        // 管理器层移除引用
        //UIManager.remove_window(*this);
        // 摧毁窗口
        if (m_hwnd) Private::Destroy(m_hwnd, pimpl()->accessibility);
        //m_hwnd = nullptr;
        // 删除数据
        pimpl()->~Private();
    }
    UIManager.RenderUnlock();
    UIManager.DataUnlock();
}

/// <summary>
/// Pres the render.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::BeforeRender() noexcept {
    const auto size = this->RefViewport().GetRealSize();
    pimpl()->BeforeRender(size);
}

/// <summary>
/// Renders this instance.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::Render() noexcept -> Result {
    // 可见可渲染
    if (this->IsVisible())
        return pimpl()->Render(this->RefViewport());
    // 不可见增加
    static_assert(WINDOW_AUTOSLEEP_TIME > 10, "must large than 10");
    if (auto& count = pimpl()->auto_sleep_count) {
        count += UIManager.GetDeltaTimeMs();
        if (count > WINDOW_AUTOSLEEP_TIME) {
            this->IntoSleepImmediately();
            count = 0;
        }
    }
    return{ Result::RS_OK };
}

namespace LongUI {
    /// <summary>
    /// Recursives the recreate_device.
    /// </summary>
    /// <param name="ctrl">The control.</param>
    /// <param name="release">if set to <c>true</c> [release].</param>
    /// <returns></returns>
    auto RecursiveRecreate(UIControl& ctrl, bool release) noexcept ->Result {
        Result hr = ctrl.Recreate(release);
        for (auto& x : ctrl) {
#if 0
            const auto code = LongUI::RecursiveRecreate(x, release);
            if (!code) hr = code;
#else
            hr = LongUI::RecursiveRecreate(x, release);
            if (!hr) break;
#endif
        }
        return hr;
    }
}

/// <summary>
/// Recreates the device data.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::RecreateDeviceData() noexcept -> Result {
    // 重建窗口设备资源
    const auto hr = this->recreate_window();
    if (!hr) return hr;
    // 重建控件设备资源
    return LongUI::RecursiveRecreate(this->RefViewport(), false);
}

/// <summary>
/// Releases the device data.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::ReleaseDeviceData() noexcept {
    // 释放窗口设备资源
    this->release_window_only_device();
    // 释放控件设备资源
    LongUI::RecursiveRecreate(this->RefViewport(), true);
}

/// <summary>
/// Releases the window only device.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::release_window_only_device() noexcept {
    pimpl()->ReleaseDeviceData();
}

/// <summary>
/// Recreates the window.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::recreate_window() noexcept -> Result {
    return pimpl()->Recreate(m_hwnd);
}

/// <summary>
/// Gets the position.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::GetPos() const noexcept -> Point2L {
    return { pimpl()->rect.left , pimpl()->rect.top };
}

/// <summary>
/// Gets the position.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::GetAbsoluteSize() const noexcept -> Size2L {
    return { pimpl()->rect.width , pimpl()->rect.height };
}

/// <summary>
/// Registers the access key.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
void LongUI::CUIWindow::RegisterAccessKey(UIControl& ctrl) noexcept {
    const auto ch = ctrl.GetAccessKey();
    if (ch >= 'A' && ch <= 'Z') {
        const auto index = ch - 'A';
        const auto map = pimpl()->access_key_map;
#ifndef NDEBUG
        if (map[index]) {
            LUIDebug(Warning)
                << "Access key("
                << ch
                << ") existed"
                << endl;
        }
#endif
        map[index] = &ctrl;
    }
    else assert(ch == 0 && "unsupported access key");
}

/// <summary>
/// Finds the control.
/// </summary>
/// <param name="id">The identifier.</param>
/// <returns></returns>
auto LongUI::CUIWindow::FindControl(const char* id) noexcept -> UIControl* {
    assert(id && "bad id");
    U8View view = U8View::FromCStyle(id);
    return this->FindControl(UIManager.GetUniqueText(view));
}


/// <summary>
/// Finds the control.
/// </summary>
/// <param name="id">The identifier.</param>
/// <returns></returns>
auto LongUI::CUIWindow::FindControl(U8View id) noexcept -> UIControl* {
    assert(id.size() && "bad id");
    return this->FindControl(UIManager.GetUniqueText(id));
}

PCN_NOINLINE
/// <summary>
/// Finds the control.
/// </summary>
/// <param name="id">The identifier.</param>
/// <returns></returns>
auto LongUI::CUIWindow::FindControl(ULID id) noexcept -> UIControl * {
    if (id.id == CUIConstShortString::EMPTY) return nullptr;
    assert(id.id && *id.id && "bad string");
    auto node = pimpl()->named_list.first;
    // 遍历命名列表
    while (node) {
        if (node->GetID().id == id.id) return node;
        node = node->m_oManager.next_named;
    }
    return nullptr;
}

/// <summary>
/// Controls the attached.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
void LongUI::CUIWindow::ControlAttached(UIControl& ctrl) noexcept {
    // 本函数调用地点
    // A. UIControl::init
    // B. UIControl::set_window_force
    assert(ctrl.is_inited() && "call after init");
    // 注册焦点链
    if (ctrl.IsTabstop()) {
        assert(ctrl.IsFocusable());
        auto& list = pimpl()->focus_list;
#ifndef NDEBUG
        // 必须不在里面
        auto node = list.first;
        while (node) {
            if (*node == ctrl) {
                LUIDebug(Error) LUI_FRAMEID
                    << "add focus control but control exist: "
                    << ctrl
                    << endl;
            }
            node = node->m_oManager.next_tabstop;
        }
#endif
        const size_t offset = offsetof(UIControl, m_oManager.next_tabstop);
        CUIControlControl::AddControlToList(ctrl, list, offset);
    }
}

/// <summary>
/// Adds the named control.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
void LongUI::CUIWindow::AddNamedControl(UIControl& ctrl) noexcept {
    if (!this) return;
    // XXX: 自己就是窗口的场合
    if (this->RefViewport() == ctrl) return;
    // 注册命名控件
    if (ctrl.GetID().id != CUIConstShortString::EMPTY) {
        assert(ctrl.GetID().id && ctrl.GetID().id[0]);
#ifndef NDEBUG
        // 必须没有被注册过
        if (this->FindControl(ctrl.GetID())) {
            LUIDebug(Error) LUI_FRAMEID
                << "add named control but id exist: "
                << ctrl.GetID()
                << endl;
        }
#endif
        auto& list = pimpl()->named_list;
        const size_t offset = offsetof(UIControl, m_oManager.next_named);
        CUIControlControl::AddControlToList(ctrl, list, offset);
    }
}

/// <summary>
/// Controls the disattached.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <remarks>
/// null this ptr acceptable
/// </remarks>
/// <returns></returns>
void LongUI::CUIWindow::ControlDisattached(UIControl& ctrl) noexcept {
    // 没有承载窗口就算了
    if (!this) return;
    // 清除
    const auto cleanup = [this, &ctrl]() noexcept {
        // 强制重置
        ctrl.m_oStyle.state.focus = false;
        m_pMiniWorldChange = nullptr;
        pimpl()->dirty_count_recording = 0;
    };
    // 析构中
    if (m_inDtor) return cleanup();
    // 移除相关弱引用

    CUIDataAutoLocker locker;
    // 2. 移除最小世界修改
    if (m_pMiniWorldChange == &ctrl) {
        // TEST THIS
        assert(m_pMiniWorldChange != &RefViewport());
        m_pMiniWorldChange = m_pMiniWorldChange->GetParent();
    }
    // 3. 移除在脏矩形列表
    if (UIControlPrivate::IsInDirty(ctrl)) {
        UIControlPrivate::ClearInDirty(ctrl);
        // 查找
        if (!pimpl()->is_fr_for_update()) {
            const auto b = pimpl()->dirty_rect_recording;
            const auto e = b + pimpl()->dirty_count_recording;
            const auto itr = std::find_if(b, e, [&ctrl](const auto& x) noexcept {
                return x.control == &ctrl;
            });
            // 安全起见
            if (itr != e) {
                // 最后一个和itr调换
                // 特殊情况: itr == e[-1], 调换并不会出现问题
                std::swap(*itr, e[-1]);
                assert(pimpl()->dirty_count_recording);
                pimpl()->dirty_count_recording--;
            }
            else assert(!"NOT FOUND");
        }
    }
    // 4. 移除普通弱引用
    std::for_each(
        pimpl()->GetFirst(), 
        pimpl()->GetLast() + 1, 
        [&](auto& p) noexcept { if (p == &ctrl) p = nullptr; }
    );
    // 5. 移除快捷弱引用
    const auto ch = ctrl.GetAccessKey();
    if (ch >= 'A' && ch <= 'Z') {
        const auto index = ch - 'A';
        const auto map = pimpl()->access_key_map;
        // 移除引用
        if (map[index] == &ctrl) map[index] = nullptr;
        // 提出警告
#ifndef NDEBUG
        else {
            LUIDebug(Warning)
                << "map[index] != &ctrl:(map[index]:"
                << map[index]
                << ")"
                << endl;
        }
#endif

    }
    // 5. 移除查找表中的弱引用
    if (ctrl.GetID().id[0]) {
        auto& list = pimpl()->named_list;
        const size_t offset = offsetof(UIControl, m_oManager.next_named);
        CUIControlControl::RemoveControlInList(ctrl, list, offset);
    }
    // 6. 移除焦点表中的弱引用
    if (ctrl.IsFocusable()) {
        auto& list = pimpl()->focus_list;
        const size_t offset = offsetof(UIControl, m_oManager.next_tabstop);
        CUIControlControl::RemoveControlInList(ctrl, list, offset);
    }
    ctrl.m_oManager.next_tabstop = nullptr;
    ctrl.m_oManager.next_named = nullptr;
}

/// <summary>
/// Sets the focus.
/// 为目标控件设置键盘焦点
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
bool LongUI::CUIWindow::SetFocus(UIControl& ctrl) noexcept {
    // 不可聚焦
    if (!ctrl.IsFocusable()) return false;
    // 焦点控件
    auto& focused = pimpl()->focused;
    // 当前焦点不能是待聚焦控件的祖先控件
#ifndef DEBUG
    if (focused) {
        assert(
            !ctrl.IsAncestorForThis(*focused) && 
            "cannot set focus to control that ancestor was focused"
        );
    }
    const auto old_focused = focused;
#endif
    // 已为焦点
    if (focused == &ctrl) return true;
    // 释放之前焦点
    if (focused) this->KillFocus(*focused);
    // 设为焦点
    focused = &ctrl;
    ctrl.StartAnimation({ StyleStateType::Type_Focus, true });
    // Focus 事件
    ctrl.TriggerEvent(UIControl::_onFocus());
    return true;
}


/// <summary>
/// set focus to prev control
/// 设置键盘焦点至上一个焦点
/// </summary>
/// <returns></returns>
bool LongUI::CUIWindow::FocusPrev() noexcept {
    // 搜索上一个
    const auto find_prev = [this](UIControl* focused) noexcept {
        auto node = pimpl()->focus_list.first;
        UIControl* rcode = nullptr;
        while (node) {
            if (node == focused) break;
            if (node->IsEnabled()) rcode = node;
            node = node->m_oManager.next_tabstop;
        }
        return rcode;
    };
    UIControl* target = nullptr;
    // 当前焦点的下一个
    if (pimpl()->focused) target = find_prev(pimpl()->focused);
    // 没有就最后一个
    // XXX: 最后一个无效时候怎么处理
    if (!target) target = pimpl()->focus_list.last;
    // 还没有就算了
    if (!target) return false;
    this->SetDefault(*target);
    return this->SetFocus(*target);
}

/// <summary>
/// set focus to next control
/// 设置键盘焦点至下一个焦点
/// </summary>
/// <returns></returns>
bool LongUI::CUIWindow::FocusNext() noexcept {
    // TODO: 下一个可以成为焦点控件但是不能成为默认控件时候
    // 默认控件应该回退至窗口初始的默认控件


    // 搜索下一个
    const auto find_next = [](UIControl* node) noexcept {
        while (true) {
            node = node->m_oManager.next_tabstop;
            if (!node || node->IsEnabled()) break;
        }
        return node;
    };
    UIControl* target = nullptr;
    // 当前焦点的下一个
    if (pimpl()->focused) target = find_next(pimpl()->focused);
    // 没有就第一个
    // XXX: 第一个无效时候怎么处理
    if (!target) target = pimpl()->focus_list.first;
    // 还没有就算了
    if (!target) return false;
    this->SetDefault(*target);
    return this->SetFocus(*target);
}

/// <summary>
/// Shows the caret.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::ShowCaret(UIControl& ctrl, const RectF& rect) noexcept {
    assert(this);
    pimpl()->careted = &ctrl;
    pimpl()->caret_ok = true;
    pimpl()->caret = rect;
    //ctrl.MapToWindow(pimpl()->caret);
}

/// <summary>
/// Hides the caret.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::HideCaret() noexcept {
    assert(this);
    pimpl()->careted = nullptr;
    pimpl()->caret_ok = false;
}

/// <summary>
/// Sets the color of the caret.
/// </summary>
/// <param name="color">The color.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetCaretColor(const ColorF& color) noexcept {
    assert(this);
    pimpl()->caret_color = color;
}

/// <summary>
/// Sets the capture.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetCapture(UIControl& ctrl) noexcept {
    pimpl()->captured = &ctrl;
    //LUIDebug(Hint) << ctrl << endl;
}

/// <summary>
/// Releases the capture.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
bool LongUI::CUIWindow::ReleaseCapture(UIControl& ctrl) noexcept {
    if (&ctrl == pimpl()->captured) {
        //LUIDebug(Hint) << ctrl << endl;
        assert(pimpl()->captured);
        pimpl()->captured = nullptr;
        return true;
    }
    return false;
}

/// <summary>
/// Kills the focus.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
void LongUI::CUIWindow::KillFocus(UIControl& ctrl) noexcept {
    if (pimpl()->focused == &ctrl) {
        pimpl()->focused = nullptr;
        //m_private->saved_focused = nullptr;
        ctrl.StartAnimation({ StyleStateType::Type_Focus, false });
        // Blur 事件
        ctrl.TriggerEvent(UIControl::_onBlur());
    }
}

/// <summary>
/// Resets the default.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::ResetDefault() noexcept {
    if (pimpl()->wnd_default) {
        this->SetDefault(*pimpl()->wnd_default);
    }
}

/// <summary>
/// Invalidates the control.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::InvalidateControl(UIControl& ctrl, const RectF* rect) noexcept {
    // 已经在里面就算了
    if (UIControlPrivate::IsInDirty(ctrl)) return;
    assert(ctrl.GetWindow() == this);
    // 全渲染
    if (pimpl()->is_fr_for_update()) return;
    // TODO: 相对矩形
    assert(rect == nullptr && "unsupported yet");
    pimpl()->MarkDirtRect({ &ctrl, ctrl.GetBox().visible });
}


/// <summary>
/// Marks the full rendering.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::MarkFullRendering() noexcept {
    pimpl()->mark_fr_for_update();
}

/// <summary>
/// Determines whether [is full render this frame].
/// </summary>
/// <returns></returns>
bool LongUI::CUIWindow::IsFullRenderThisFrame() const noexcept {
    return pimpl()->is_fr_for_update();
}

/// <summary>
/// Sets the default.
/// </summary>
/// <param name="ctrl">The control.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetDefault(UIControl& ctrl) noexcept {
    assert(this && "null this ptr");
    if (!ctrl.IsDefaultable()) return;
    constexpr auto dtype = StyleStateType::Type_Default;
    auto& nowc = pimpl()->now_default;
    if (nowc) nowc->StartAnimation({ dtype, false });
    (nowc = &ctrl)->StartAnimation({ dtype, true });
}

/// <summary>
/// Sets the color of the clear.
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
void LongUI::CUIWindow::SetClearColor(const ColorF& color) noexcept {
    pimpl()->clear_color = color;
}

/// <summary>
/// Sets the now cursor.
/// </summary>
/// <param name="cursor">The cursor.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetNowCursor(const CUICursor& cursor) noexcept {
    pimpl()->cursor = cursor;
}

/// <summary>
/// Sets the now cursor.
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
void LongUI::CUIWindow::SetNowCursor(std::nullptr_t) noexcept {
    pimpl()->cursor = { CUICursor::Cursor_Arrow };
}

/// <summary>
/// Called when [resize].
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::OnResizeTs(Size2U size) noexcept {
    assert(size.width && size.height && "bad size");
    // 不一样才处理
    const auto samew = this->rect.width == size.width;
    const auto sameh = this->rect.height == size.height;
    if (samew && sameh) return;
    // 数据锁
    CUIDataAutoLocker locker;
    //LUIDebug(Hint) << size.width << ", " << size.height << endl;
    this->mark_fr_for_update();
    // 修改
    this->rect.width = size.width;
    this->rect.height = size.height;
    const auto fw = static_cast<float>(size.width);
    const auto fh = static_cast<float>(size.height);
    // 重置大小
    this->viewport()->resize_window({ fw, fh });
    // 修改窗口缓冲帧大小
    this->flag_sized = true;
}

// ----------------------------------------------------------------------------
// ------------------- Windows
// ----------------------------------------------------------------------------



/// <summary>
/// Resizes the absolute.
/// </summary>
/// <param name="size">The size.</param>
/// <returns></returns>
void LongUI::CUIWindow::ResizeAbsolute(Size2L size) noexcept {
    assert(size.width && size.height);
    // 不一样才处理
    const auto& old = pimpl()->rect;
    if (old.width == size.width && old.height == size.height) {
        pimpl()->mark_fr_for_update();
        return;
    }
    // 睡眠模式
    if (this->IsInSleepMode()) { 
        pimpl()->rect.width = size.width;
        pimpl()->rect.height = size.height;
        return;
    }
    // 内联窗口
    if (this->IsInlineWindow()) {
        assert(!"NOT IMPL");
    }
    else {
        // 调整大小
        const auto realw = size.width + pimpl()->adjust.right - pimpl()->adjust.left;
        const auto realh = size.height + pimpl()->adjust.bottom - pimpl()->adjust.top;
        // 改变窗口
        constexpr UINT flag = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_ASYNCWINDOWPOS;
        ::SetWindowPos(m_hwnd, nullptr, 0, 0, realw, realh, flag);
    }
}

/// <summary>
/// Resizes the relative.
/// </summary>
/// <param name="size">The size.</param>
/// <returns></returns>
void LongUI::CUIWindow::ResizeRelative(Size2F size) noexcept {
    // 先要清醒才行
    this->WakeUp();
    return this->ResizeAbsolute(RefViewport().AdjustSize(size));
}


/// <summary>
/// Maps to screen.
/// </summary>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::MapToScreen(RectL& rect) const noexcept {
    // 内联窗口
    if (this->IsInlineWindow()) {
        assert(!"NOT IMPL");
    }
    // 系统窗口
    else {
        POINT pt{ 0, 0 };
        ::ClientToScreen(m_hwnd, &pt);
        rect.left += pt.x;
        rect.top += pt.y;
        rect.right += pt.x;
        rect.bottom += pt.y;
    }
}

/// <summary>
/// Maps to screen.
/// </summary>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::MapToScreen(RectF& rect) const noexcept {
    // 内联窗口
    if (this->IsInlineWindow()) {
        assert(!"NOT IMPL");
    }
    // 系统窗口
    else {
        POINT pt{ 0, 0 };
        ::ClientToScreen(m_hwnd, &pt);
        const auto px = static_cast<float>(pt.x);
        const auto py = static_cast<float>(pt.y);
        rect.top += py;
        rect.left += px;
        rect.right += px;
        rect.bottom += py;
    }
}

/// <summary>
/// Maps to screen.
/// </summary>
/// <param name="pos">The position.</param>
/// <returns></returns>
auto LongUI::CUIWindow::MapToScreenEx(Point2F pos) const noexcept ->Point2F {
    RectF rect = { pos.x, pos.y, pos.x, pos.y };
    this->MapToScreen(rect);
    return { rect.left, rect.top };
}

/// <summary>
/// Maps from screen.
/// </summary>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::MapFromScreen(Point2F& pos) const noexcept {
    // 内联窗口
    if (this->IsInlineWindow()) {
        assert(!"NOT IMPL");
    }
    // 系统窗口
    else {
        POINT pt{ 0, 0 };
        ::ScreenToClient(m_hwnd, &pt);
        const auto px = static_cast<float>(pt.x);
        const auto py = static_cast<float>(pt.y);
        pos.x += px;
        pos.y += py;
    }
}

/// <summary>
/// His the dpi support.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::HiDpiSupport() noexcept {
    if (UIManager.flag & ConfigureFlag::Flag_NoAutoScaleOnHighDpi) return;
    const auto dpi = impl::get_dpi_scale_from_hwnd(m_hwnd);
    this->RefViewport().JustResetZoom(dpi.width, dpi.height);
    //this->RefViewport().JustResetZoom(2.f, 2.f);
}


/// <summary>
/// Closes the window.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::close_window() noexcept {
    // 取消
    this->SetResult(0);
    // 通用处理
    UIManager.close_helper(*this);
    // 提醒VP: 窗口关了
    this->RefViewport().WindowClosed();
}


PCN_NOINLINE
/// <summary>
/// Shows the window.
/// </summary>
/// <param name="sw">The sw type.</param>
/// <returns></returns>
void LongUI::CUIWindow::show_window(TypeShow sw) noexcept {
    // 隐藏
    if (sw == Show_Hide) {
        this->TrySleep();
        pimpl()->window_visible = false;
    }
    // 显示
    else {
        this->WakeUp();
        pimpl()->window_visible = true;
    }
    assert(m_hwnd && "bad window");
    /*
        This function posts a show-window event to the message 
        queue of the given window. An application can use this 
        function to avoid becoming nonresponsive while waiting 
        for a nonresponsive application to finish processing a 
        show-window event.
    */
    // 使用非阻塞的函数
    ::ShowWindowAsync(m_hwnd, sw);
}

/// <summary>
/// Wakeks up.
/// </summary>
void LongUI::CUIWindow::WakeUp() noexcept {
    // 不在睡眠状态强行渲染一帧
    if (!this->IsInSleepMode()) {
        if (!pimpl()->window_visible) pimpl()->EmptyRender();
        return;
    }
    // XXX: 0. 尝试唤醒父窗口
    if (m_pParent) m_pParent->WakeUp();
    // 1. 创建窗口
    const auto pwnd = m_pParent ? m_pParent->GetHwnd() : nullptr;
    m_hwnd = pimpl()->Init(pwnd, this->config);
    // 1.5 检测DPI支持
    this->HiDpiSupport();
    // 2. 创建资源
    const auto hr = this->recreate_window();
    assert(hr && "TODO: error handle");
}

/// <summary>
/// Intoes the sleep.
/// </summary>
void LongUI::CUIWindow::IntoSleepImmediately() noexcept {
    if (this->IsInSleepMode()) return;
#ifndef NDEBUG
    LUIDebug(Hint) 
        << this << this->RefViewport()
        << "into sleep mode" << endl;
#endif // !NDEBUG
    //assert(this->begin() == this->end());

    // 释放资源
    pimpl()->ReleaseDeviceData();
    Private::Destroy(m_hwnd, pimpl()->accessibility);
    m_hwnd = nullptr;
}

/// <summary>
/// Tries the sleep.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::TrySleep() noexcept {
    // 已经进入就算了
    if (this->IsInSleepMode()) return;
    // 必须是自动休眠
    if (!this->IsAutoSleep()) return;
    // XXX: 存在子窗口就算了?
    //if (this->begin() != this->end()) return;
    // 增加1毫秒(+1ms)长度表示进入睡眠计时
    pimpl()->auto_sleep_count = 1;
}

/// <summary>
/// Executes this instance.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::Exec() noexcept->uintptr_t {
    uintptr_t rv = 0;
    const auto parent = this->GetParent();
    const auto pahwnd = parent ? parent->GetHwnd() : nullptr;
    m_bInExec = true;
    {
        CUIBlockingGuiOpAutoUnlocker unlocker;
        // 禁止父窗口调用
        ::EnableWindow(pahwnd, false);
        // 增加一层消息循环
        UIManager.RecursionMsgLoop();
        // 恢复父窗口调用
        ::EnableWindow(pahwnd, true);
        // 激活父窗口
        ::SetActiveWindow(pahwnd);
    }
    m_bInExec = false;
    return rv;
}


/// <summary>
/// Recursives the set result.
/// </summary>
/// <param name="result">The result.</param>
/// <returns></returns>
void LongUI::CUIWindow::recursive_set_result(uintptr_t result) noexcept {
    // TEST
    assert(this->IsInExec());
    auto node = m_oHead.next;
    const auto tail = &m_oTail;
    while (node != tail) {
        if (node->IsInExec()) node->recursive_set_result(0);
        node = node->next;
    }
    UIManager.BreakMsgLoop(result);
}

/// <summary>
/// Sets the result.
/// </summary>
/// <param name="result">The result.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetResult(uintptr_t result) noexcept {
    if (m_bInExec) this->recursive_set_result(result);
}

/// <summary>
/// Closes the window.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::CloseWindow() noexcept {
    ::PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

/// <summary>
/// Actives the window.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::ActiveWindow() noexcept {
    ::SetActiveWindow(m_hwnd);
}


/// <summary>
/// Determines whether this instance is visible.
/// </summary>
/// <returns></returns>
bool LongUI::CUIWindow::IsVisible() const noexcept {
    return pimpl()->window_visible;
    //return !!::IsWindowVisible(m_hwnd);
}


/// <summary>
/// Sets the name of the title.
/// </summary>
/// <param name="name">The name.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetTitleName(const char16_t* name) noexcept {
    assert(name && "bad name");
    this->SetTitleName(CUIString{ name });
}

/// <summary>
/// Gets the name of the title.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::GetTitleName() const noexcept -> U16View {
    return pimpl()->titlename.view();
}

/// <summary>
/// Popups the window.
/// </summary>
/// <param name="wnd">The WND.</param>
/// <param name="pos">The position.</param>
/// <param name="type">The type.</param>
/// <returns></returns>
void LongUI::CUIWindow::PopupWindow(CUIWindow& wnd, Point2L pos, PopupType type) noexcept {
    auto& this_popup = pimpl()->popup;
    // 再次显示就是关闭
    if (this_popup == &wnd) {
        this->ClosePopup();
    }
    else {
        this->ClosePopup();
        this_popup = &wnd;
        // 记录类型备用
        pimpl()->popup_type = type;
        // 提示窗口
        auto& view = wnd.RefViewport();
        view.HosterPopupBegin();
        this->RefViewport().SubViewportPopupBegin(view, type);
        // 计算位置
        wnd.SetPos(pos);
        wnd.ShowNoActivate();
    }
}

/// <summary>
/// Tooltips the text.
/// </summary>
/// <param name="text">The text.</param>
/// <returns></returns>
auto LongUI::CUIWindow::TooltipText(CUIString&& text)noexcept->UIViewport* {
    pimpl()->SetTooltipText(std::move(text));
    return pimpl()->common_tooltip;
}

/// <summary>
/// Closes the popup.
/// 关闭当前的弹出窗口
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::ClosePopup() noexcept {
    pimpl()->ClosePopup();
}

/// <summary>
/// Gets the now popup.
/// 获取当前的弹出窗口
/// </summary>
/// <returns>
/// <see cref="CUIWindow"/>指针, 当前没有弹出窗口则返回空指针
///</returns>
auto LongUI::CUIWindow::GetNowPopup() const noexcept-> CUIWindow* {
    return pimpl()->popup;
}

/// <summary>
/// Gets the now popup with specify type.
/// 获取当前指定类型的弹出窗口
/// </summary>
/// <param name="type">The type.</param>
/// <returns>
/// <see cref="CUIWindow" />指针, 当前没有指定类型的弹出窗口则返回空指针
/// </returns>
auto LongUI::CUIWindow::GetNowPopup(PopupType type) const noexcept-> CUIWindow* {
    return pimpl()->popup_type == type ? pimpl()->popup : nullptr;
}

/// <summary>
/// Closes the popup high level.
/// 尽可能向上级关闭弹出窗口
/// </summary>
/// <remarks>
/// 该函数允许传递空this指针
/// </remarks>
void LongUI::CUIWindow::ClosePopupHighLevel() noexcept {
#if 0
    // 根据Parent信息
    auto winp = this;
    // 正式处理
    while (winp && winp->config & Config_Popup) winp = winp->GetParent();
#else
    // 根据Hoster信息
    auto& view = this->RefViewport();
    auto hoster = view.GetHoster();
    CUIWindow* winp = nullptr;
    while (hoster) {
        winp = hoster->GetWindow();
        if (winp) hoster = winp->RefViewport().GetHoster();
    }
    assert(!winp || !(winp->config & Config_Popup));
#endif
    if (winp) winp->ClosePopup();
    else LUIDebug(Error) << "winp -> null" << endl;
}

/// <summary>
/// Closes the tooltip.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::CloseTooltip() noexcept {
    if (pimpl()->popup_type == PopupType::Type_Tooltip) {
        pimpl()->ClosePopup();
    }
}

/// <summary>
/// Sets the absolute rect.
/// </summary>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetAbsoluteRect(const RectL& rect) noexcept {
    // 懒得判断了
    auto& write = pimpl()->rect;
    write.left = rect.left;
    write.top = rect.top;
    write.width = rect.right - rect.left;
    write.height = rect.bottom - rect.top;
    // 睡眠模式
    if (this->IsInSleepMode()) return;
    // 内联窗口
    if (this->IsInlineWindow()) {
        assert(!"NOT IMPL");
    }
    // 系统窗口
    else {
        ::SetWindowPos(
            m_hwnd, 
            nullptr, 
            write.left, write.top, write.width, write.height,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
    }
}

/// <summary>
/// Sets the position.
/// </summary>
/// <param name="pos">The position.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetPos(Point2L pos) noexcept {
    auto& this_pos = reinterpret_cast<Point2L&>(pimpl()->rect.left);
    // 无需移动窗口
    if (this_pos.x == pos.x && this_pos.y == pos.y) return; 
    this_pos = pos;
    // 睡眠模式
    if (this->IsInSleepMode()) return;
    // 内联窗口
    if (this->IsInlineWindow()) {
        assert(!"NOT IMPL");
    }
    // 系统窗口
    else {
        const auto adjx = 0; // m_private->adjust.left;
        const auto adjy = 0; // m_private->adjust.top;
        constexpr UINT flag = SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE;
        ::SetWindowPos(m_hwnd, nullptr, pos.x + adjx, pos.y + adjy, 0, 0, flag);
    }
}

// longui namespace
namespace LongUI {
    // Commons the tooltip create.
    auto CommonTooltipCreate(UIControl& hoster) noexcept->UIViewport*;
    // Commons the tooltip set text.
    void CommonTooltipSetText(UIViewport& viewport, CUIString&& text) noexcept;
}

/// <summary>
/// Sets the tooltip text.
/// </summary>
/// <param name="text">The text.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::SetTooltipText(CUIString&& text) noexcept {
    auto& ptr = this->common_tooltip;
    if (!ptr) ptr = LongUI::CommonTooltipCreate(*this->viewport());
    if (ptr) LongUI::CommonTooltipSetText(*ptr, std::move(text));
}

/// <summary>
/// Sets the layered window support.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::SetLayeredWindowSupport() noexcept {
    this->layered_window_support = true;
    this->dcomp_support = impl::check_dcomp_support();
}

/// <summary>
/// Called when [key down].
/// </summary>
/// <param name="vk">The vk.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::OnKeyDownUp(InputEvent ekey, CUIInputKM::KB key) noexcept {
    switch (key)
    {
    case CUIInputKM::KB_ESCAPE:
        // 检查释放Esc关闭窗口
        if (this->viewport()->RefWindow().config
            & CUIWindow::Config_EscToCloseWindow)
            return this->viewport()->RefWindow().CloseWindow();
        break;
    case CUIInputKM::KB_RETURN:
        // 回车键: 直接将输入引导到默认控件
        if (const auto defc = this->now_default) {
            if (defc->IsEnabled()) defc->DoInputEvent({ ekey, 0, key });
            return;
        }
        break;
    case CUIInputKM::KB_SPACE:
        // 空格键: 直接将输入引导到焦点控件
        if (const auto nowfocus = this->focused) {
            if (nowfocus->IsEnabled()) nowfocus->DoInputEvent({ ekey, 0, key });
            return;
        }
        break;
    case CUIInputKM::KB_TAB:
        // Tab键: 聚焦上/下键盘焦点
        this->focus_ok = true;
        if (ekey == InputEvent::Event_KeyUp) {
            if (CUIInputKM::GetKeyState(CUIInputKM::KB_SHIFT))
                this->viewport()->RefWindow().FocusPrev();
            else
                this->viewport()->RefWindow().FocusNext();
        }
        return;
    }
    // 直接将输入引导到焦点控件
    if (const auto focused_ctrl = this->focused) {
        assert(focused_ctrl->IsEnabled());
        // 检查输出
        const auto rv = focused_ctrl->DoInputEvent({ ekey, 0, key });
        // 回车键无视了?!
        if (rv == Event_Ignore && key == CUIInputKM::KB_RETURN) {
            // 直接将输入引导到默认控件
            if (const auto defc = this->now_default) {
                if (defc->IsEnabled()) defc->DoInputEvent({ ekey, 0, key });
                return;
            }
        }
    }
}

/// <summary>
/// Called when [system key down].
/// </summary>
/// <param name="key">The key.</param>
/// <param name="lp">The lp.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::OnSystemKeyDown(CUIInputKM::KB key, uintptr_t lp) noexcept {
    switch (key)
    {
    case CUIInputKM::KB_MENU:
        // 检查访问键
        // 只有按下瞬间有效, 后续的重复触发不算
        if (!(lp & (1 << 30))) {
            // 有popup就关闭
            if (this->popup) this->ClosePopup();
            // 没有就开关ACCESSKEY显示
            else this->toggle_access_key_display();
        }
        break;
    }
}


/// <summary>
/// Toggles the access key display.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::toggle_access_key_display() noexcept {
    auto& key = this->access_key_display;
    key = !key;
    const EventArg arg{ NoticeEvent::Event_ShowAccessKey, key };
    for (auto ctrl : this->access_key_map) {
        if (ctrl) ctrl->DoEvent(nullptr, arg);
    }
}

// c
extern "C" {
    // char16 -> char32
    char32_t impl_char16x2_to_char32(char16_t lead, char16_t trail) noexcept;
}

/// <summary>
/// Called when [character].
/// </summary>
/// <param name="ch">The ch.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::OnChar(char16_t ch) noexcept {
    char32_t ch32;
    // UTF16 第一字
    if (Unicode::IsHighSurrogate(ch)) { this->saved_utf16 = ch; return; }
    // UTF16 第二字
    else if (Unicode::IsLowSurrogate(ch)) {
        ch32 = impl_char16x2_to_char32(this->saved_utf16, ch);
        this->saved_utf16 = 0;
    }
    // UTF16 仅一字
    else ch32 = static_cast<char32_t>(ch);
    // 处理输入
    this->OnCharTs(ch32);
}

/// <summary>
/// Called when [character].
/// </summary>
/// <param name="ch">The ch.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::OnCharTs(char32_t ch) noexcept {
    // IME输入优化
    if (this->ime_count) --this->ime_count;
    // TODO: 自己检查有效性?
    if ((ch >= 0x20 && ch != 0x7f) || ch == '\t') {
        // 直接将输入引导到焦点控件
        if (const auto focused_ctrl = this->focused) {
            CUIDataAutoLocker locker;
            focused_ctrl->DoInputEvent({ 
                LongUI::InputEvent::Event_Char, this->ime_count, ch 
                });
        }
    }
    else assert(this->ime_count == 0 && "IME cannot input control char");
}

/// <summary>
/// Called when [dpi changed].
/// </summary>
/// <param name="wParam">The w parameter.</param>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::OnDpiChanged(uintptr_t wParam, const RectL& rect) noexcept {
    // dpi改变了
    if (UIManager.flag & ConfigureFlag::Flag_NoAutoScaleOnHighDpi) return;
    float xdpi = float(uint16_t(LOWORD(wParam)));
    float ydpi = float(uint16_t(HIWORD(wParam)));
    float x = xdpi / float(LongUI::BASIC_DPI);
    float y = ydpi / float(LongUI::BASIC_DPI);
    CUIDataAutoLocker locker;
    auto& vp = *this->viewport();
    auto& window = vp.RefWindow();
    // 固定大小应该需要缩放窗口
    if (window.config & Config_FixedSize /*|| true*/) {
        vp.JustResetZoom(x, y);
        window.SetAbsoluteRect(rect);
    }
    // 不定大小应该懒得弄了
    else {
        vp.JustResetZoom(x, y);
        const auto fw = static_cast<float>(this->rect.width);
        const auto fh = static_cast<float>(this->rect.height);
        this->viewport()->resize_window({ fw, fh });
    }
}

/// <summary>
/// Called when [hot key].
/// </summary>
/// <param name="i">The index.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::OnAccessKey(uintptr_t i) noexcept {
    // 存在弹出窗口就指向弹出窗口
    if (this->popup) {
        // TOOLTIP不算
        if (this->popup_type != PopupType::Type_Tooltip) {
            const auto popprivate = this->popup->pimpl();
            popprivate->OnAccessKey(i);
            return;
        }
    }
    // 正式处理
    CUIDataAutoLocker locker;
    const auto ctrl = this->access_key_map[i];
    if (ctrl && ctrl->IsEnabled() && ctrl->IsVisibleToRoot()) {
        ctrl->DoEvent(nullptr, { NoticeEvent::Event_DoAccessAction, 0 });
    }
    else ::longui_error_beep();
}

// LongUI::detail
namespace LongUI { namespace detail {
    // window style
    enum style : DWORD {
        windowed         = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        aero_borderless  = WS_POPUP            | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        basic_borderless = WS_POPUP            | WS_THICKFRAME              | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
    };
    // delete later
    enum msg : uint32_t {
        msg_custom = WM_USER + 10,
        msg_post_destroy,
        msg_post_set_title,
    };
}}


/// <summary>
/// Sets the name of the title.
/// </summary>
/// <param name="name">The name.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetTitleName(CUIString&& name) noexcept {
    pimpl()->titlename = std::move(name);
    if (this->IsInSleepMode()) return;
    ::PostMessageW(m_hwnd, detail::msg_post_set_title, 0, 0);
}

/// <summary>
/// Destroys the window.
/// </summary>
/// <param name="hwnd">The HWND.</param>
/// <param name="accessibility">if set to <c>true</c> [accessibility].</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::Destroy(HWND hwnd, bool accessibility) noexcept {
    //LUIDebug(Hint) LUI_FRAMEID << hwnd << endl;
    // 尝试直接摧毁
    if (::DestroyWindow(hwnd)) return;
#ifndef NDEBUG
    const auto laste = ::GetLastError();
    assert(laste == ERROR_ACCESS_DENIED && "check the code");
#endif
    // 不在消息线程就用 PostMessage
    ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, LONG_PTR(0));
    const auto code = ::PostMessageW(hwnd, detail::msg_post_destroy, 0, 0);
    const auto ec = ::GetLastError();
    assert(code && "PostMessage failed");
}


/// <summary>
/// Initializes the window position.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::InitWindowPos() noexcept {
    auto& winrect = this->rect;
    // 默认大小
    winrect.width = LongUI::DEFAULT_WINDOW_WIDTH;
    winrect.height = LongUI::DEFAULT_WINDOW_HEIGHT;
    // 默认居中
    const auto scw = ::GetSystemMetrics(SM_CXFULLSCREEN);
    const auto sch = ::GetSystemMetrics(SM_CYFULLSCREEN);
    winrect.left = (scw - winrect.width) / 2;
    winrect.top = (sch - winrect.height) / 2;
}

/// <summary>
/// Initializes this instance.
/// </summary>
/// <param name="parent">The parent.</param>
/// <param name="config">The configuration.</param>
/// <returns></returns>
HWND LongUI::CUIWindow::Private::Init(HWND parent, CUIWindow::WindowConfig config) noexcept {
    // 尝试注册
    this->RegisterWindowClass();
    // 初始化
    HWND hwnd = nullptr;
    // 窗口
    {
        // 检查样式样式
        const auto style = [config]() noexcept -> DWORD {
            if (config & CUIWindow::Config_Popup)
                return WS_POPUPWINDOW;
            DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            if (!(config & CUIWindow::Config_FixedSize))
                style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
            return style;
        }();
        // MA返回码
        this->ma_return_code = config & CUIWindow::Config_Popup ? MA_NOACTIVATE : MA_ACTIVATE;
        // 调整大小
        static_assert(sizeof(RECT) == sizeof(this->adjust), "bad type");
        this->adjust = { 0 };
        ::AdjustWindowRect(reinterpret_cast<RECT*>(&this->adjust), style, FALSE);
        // 窗口
        RectWHL window_rect;
        window_rect.left = this->rect.left;
        window_rect.top = this->rect.top;
        window_rect.width = this->rect.width + this->adjust.right - this->adjust.left;
        window_rect.height = this->rect.height + this->adjust.bottom - this->adjust.top;
        // 针对this->rect清空为-1, 否则会检查到未修改
        this->rect = { -1, -1, -1, -1 };
        // 额外
        uint32_t ex_flag = 0;
        // DirectComposition 支持
        if (this->is_direct_composition()) 
            ex_flag |= WS_EX_NOREDIRECTIONBITMAP;
        // WS_EX_TOOLWINDOW 不会让窗口显示在任务栏
        if (config & (CUIWindow::Config_ToolWindow | CUIWindow::Config_Popup))
            ex_flag |= WS_EX_TOOLWINDOW;
        // 创建窗口
        hwnd = ::CreateWindowExW(
            //WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
            ex_flag,
            (config & CUIWindow::Config_Popup) ?
            Attribute::WindowClassNameP : Attribute::WindowClassNameN,
            detail::sys(titlename.c_str()),
            style,
            window_rect.left, window_rect.top,
            window_rect.width, window_rect.height,
            // 弹出窗口使用NUL父窗口方便显示
            config & CUIWindow::Config_Popup ? nullptr : parent ,
            nullptr,
            ::GetModuleHandleA(nullptr),
            this
        );
    }
    // 创建成功
    //if (hwnd)
    this->track_mouse.cbSize = sizeof(this->track_mouse);
    this->track_mouse.dwFlags = TME_HOVER | TME_LEAVE;
    this->track_mouse.hwndTrack = hwnd;
    this->track_mouse.dwHoverTime = HOVER_DEFAULT;
    return hwnd;
}


/// <summary>
/// Registers the window class.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::RegisterWindowClass() noexcept {
    WNDCLASSEXW wcex;
    const auto ins = ::GetModuleHandleW(nullptr);
    // 已经注册过了
    if (::GetClassInfoExW(ins, Attribute::WindowClassNameN, &wcex)) return;
    // 注册一般窗口类
    wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = 0;
    wcex.lpfnWndProc = Private::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(void*);
    wcex.hInstance = ins;
    wcex.hCursor = nullptr;
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = Attribute::WindowClassNameN;
    wcex.hIcon = ::LoadIconW(ins, Attribute::WindowIconName);
    ::RegisterClassExW(&wcex);
    // 注册弹出窗口类
    wcex.style = CS_DROPSHADOW;
    wcex.lpszClassName = Attribute::WindowClassNameP;
    ::RegisterClassExW(&wcex);
}


/// <summary>
/// Sets the native icon data.
/// </summary>
/// <param name="icon">The icon.</param>
/// <param name="big">The big.</param>
/// <returns></returns>
void LongUI::CUIWindow::SetNativeIconData(
    const wchar_t* icon, uintptr_t big) noexcept {
    assert(this->IsAutoSleep() == false && m_hwnd);
    const auto ins = ::GetModuleHandleW(nullptr);
    const auto lp = reinterpret_cast<LPARAM>(::LoadIconW(ins, icon));
    //::SendMessageW(m_hwnd, WM_SETICON, big, lp);
    ::PostMessageW(m_hwnd, WM_SETICON, big, lp);
}

// ui namespace
namespace LongUI {
    // mouse event map(use char to avoid unnecessary memory-waste)
    const uint8_t MEMAP[] = {
        // WM_MOUSEMOVE                 0x0200
        static_cast<uint8_t>(MouseEvent::Event_MouseMove),
        // WM_LBUTTONDOWN               0x0201
        static_cast<uint8_t>(MouseEvent::Event_LButtonDown),
        // WM_LBUTTONUP                 0x0202
        static_cast<uint8_t>(MouseEvent::Event_LButtonUp),
        // WM_LBUTTONDBLCLK             0x0203
        static_cast<uint8_t>(MouseEvent::Event_Unknown),
        // WM_RBUTTONDOWN               0x0204
        static_cast<uint8_t>(MouseEvent::Event_RButtonDown),
        // WM_RBUTTONUP                 0x0205
        static_cast<uint8_t>(MouseEvent::Event_RButtonUp),
        // WM_RBUTTONDBLCLK             0x0206
        static_cast<uint8_t>(MouseEvent::Event_Unknown),
        // WM_MBUTTONDOWN               0x0207
        static_cast<uint8_t>(MouseEvent::Event_MButtonDown),
        // WM_MBUTTONUP                 0x0208
        static_cast<uint8_t>(MouseEvent::Event_MButtonUp),
        // WM_MBUTTONDBLCLK             0x0209
        static_cast<uint8_t>(MouseEvent::Event_Unknown),
        // WM_MOUSEWHEEL                0x020A
        static_cast<uint8_t>(MouseEvent::Event_MouseWheelV),
        // WM_XBUTTONDOWN               0x020B
        static_cast<uint8_t>(MouseEvent::Event_Unknown),
        // WM_XBUTTONUP                 0x020C
        static_cast<uint8_t>(MouseEvent::Event_Unknown),
        // WM_XBUTTONDBLCLK             0x020D
        static_cast<uint8_t>(MouseEvent::Event_Unknown),
        // WM_MOUSEHWHEEL               0x020E
        static_cast<uint8_t>(MouseEvent::Event_MouseWheelH),
    };
}

/// <summary>
/// Marks the dirt rect.
/// </summary>
/// <param name="rect">The rect.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::MarkDirtRect(const DirtyRect& rect) noexcept {
    auto& counter = this->dirty_count_recording;
    assert(counter <= LongUI::DIRTY_RECT_COUNT);
    // 满了/全渲染 就算了
    if (counter == LongUI::DIRTY_RECT_COUNT) 
        return this->mark_fr_for_update();
    // 如果脏矩形列表存在祖先节点就算了
    const auto ctrl = rect.control;
    for (uint32_t i = 0; i != counter; ++i) {
        if (ctrl->IsAncestorForThis(*this->dirty_rect_recording[i].control))
            return;
    }
    // 标记在表
    UIControlPrivate::MarkInDirty(*rect.control);
    // 写入数据
    this->dirty_rect_recording[counter] = rect;
    ++counter;
}

/// <summary>
/// Befores the render.
/// </summary>
/// <param name="size">The size.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::BeforeRender(const Size2F size) noexcept {
    // 先清除信息
    this->clear_fr_for_render();
    const uint32_t count = this->dirty_count_recording;
    this->dirty_count_recording = 0;
    std::for_each(
        this->dirty_rect_recording,
        this->dirty_rect_recording + count,
        [](const DirtyRect& x) noexcept {
        UIControlPrivate::ClearInDirty(*x.control);
    });
    // 全渲染
    if (this->is_fr_for_update()) {
        this->clear_fr_for_update();
        this->mark_fr_for_render();
        return;
    }
    // 初始化信息
    auto itr = this->dirty_rect_presenting;
    const auto endi = itr + LongUI::DIRTY_RECT_COUNT;
    // 遍历脏矩形
    for (uint32_t i = 0; i != count; ++i) {
        const auto& data = this->dirty_rect_recording[i];
        assert(data.control && data.control->GetWindow());
        itr = impl::mark_two_rect_dirty(
            data.rectangle,
            data.control->GetBox().visible,
            size,
            itr
        );
        // 溢出->全渲染
        if (itr > endi) 
            return this->mark_fr_for_render();
    }
    // 写入数据
    const auto pcount = itr - this->dirty_rect_presenting;
    this->dirty_count_presenting = uint32_t(pcount);

#if 0
    // 先清除
    this->clear_full_rendering_for_render();
    // 复制全渲染信息
    if (this->is_full_render_for_update()) {
        this->mark_full_rendering_for_render();
        this->clear_full_rendering_for_update();
        this->dirty_count_for_render = 0;
        this->dirty_count_for_update = 0;
        return;
    }
    // 复制脏矩形信息
    this->dirty_count_for_render = this->dirty_count_for_update;
    this->dirty_count_for_update = 0;
    std::memcpy(
        this->dirty_rect_for_render,
        this->dirty_rect_for_update,
        sizeof(this->dirty_rect_for_update[0]) * this->dirty_count_for_render
    );
#endif
}

PCN_NOINLINE
/// <summary>
/// Closes the popup.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::ClosePopup() noexcept {
    if (this->popup) {
        // 递归关闭
        this->popup->ClosePopup();
        auto& sub = this->popup->RefViewport();
        sub.HosterPopupEnd();
        this->viewport()->SubViewportPopupEnd(sub, this->popup_type);
        this->popup->CloseWindow();
        this->popup = nullptr;
    }
}


/// <summary>
/// Mouses the event.
/// </summary>
/// <param name="args">The arguments.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::DoMouseEventTs(const MouseEventArg & args) noexcept {
    CUIDataAutoLocker locker;
    UIControl* ctrl = this->captured;
    if (!ctrl) ctrl = this->viewport();
    /*const auto code = */
    ctrl->DoMouseEvent(args);
}


#ifndef NDEBUG
volatile UINT g_dbgLastEventId = 0;
#endif


/// <summary>
/// Does the MSG.
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
auto LongUI::CUIWindow::Private::DoMsg(
    const HWND hwnd, const UINT message, 
    const WPARAM wParam, const LPARAM lParam) noexcept -> intptr_t {
#ifndef NDEBUG
    g_dbgLastEventId = message;
#endif
    MouseEventArg arg;
    // 鼠标离开
    if (message == WM_MOUSELEAVE) {
        // BUG: Alt + 快捷键也会触发?
        //CUIInputKM::Instance().GetKeyState(CUIInputKM::KB_MENU);
        arg.px = -1.f; arg.py = -1.f;
        arg.wheel = 0.f;
        arg.type = MouseEvent::Event_MouseLeave;
        arg.modifier = LongUI::Modifier_None;
        this->mouse_enter = false;
        this->DoMouseEventTs(arg);
    }
    // 鼠标悬壶(济世)
    else if (message == WM_MOUSEHOVER) {
        arg.type = MouseEvent::Event_MouseIdleHover;
        arg.wheel = 0.f;
        arg.px = static_cast<float>(int16_t(LOWORD(lParam)));
        arg.py = static_cast<float>(int16_t(HIWORD(lParam)));
        arg.modifier = static_cast<InputModifier>(wParam);
        this->DoMouseEventTs(arg);
    }
    // 一般鼠标消息处理
    else if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) {
        // 检查映射表长度
        constexpr size_t ARYLEN = sizeof(MEMAP) / sizeof(MEMAP[0]);
        constexpr size_t MSGLEN = WM_MOUSELAST - WM_MOUSEFIRST + 1;
        static_assert(ARYLEN == MSGLEN, "must be same");
        // 初始化参数
        arg.type = static_cast<MouseEvent>(MEMAP[message - WM_MOUSEFIRST]);
        arg.wheel = 0.f;
        arg.px = static_cast<float>(int16_t(LOWORD(lParam)));
        arg.py = static_cast<float>(int16_t(HIWORD(lParam)));
        arg.modifier = static_cast<InputModifier>(wParam);
        // shift+滚轮自动映射到水平方向
        if (message == WM_MOUSEWHEEL && (wParam & MK_SHIFT)) {
            arg.type = MouseEvent::Event_MouseWheelH;
        }
        switch (arg.type)
        {
            float delta;
        case MouseEvent::Event_MouseWheelV:
        case MouseEvent::Event_MouseWheelH:
            // 滚轮消息则修改滚轮数据
            delta = static_cast<float>(int16_t(HIWORD(wParam)));
            arg.wheel = delta / static_cast<float>(WHEEL_DELTA);
            break;
        case MouseEvent::Event_MouseMove:
            // 第一次进入
            if (!this->mouse_enter) {
                arg.type = MouseEvent::Event_MouseEnter;
                this->mouse_enter = true;
                this->DoMouseEventTs(arg);
                arg.type = MouseEvent::Event_MouseMove;
            };
            // 鼠标跟踪
            ::TrackMouseEvent(&this->track_mouse);
            break;
        case MouseEvent::Event_LButtonDown:
            // 有弹出窗口先关闭
            // FIXME: 自己也是弹出窗口的话怎么处理?
            if (this->popup && !(viewport()->RefWindow().config & Config_Popup)) {
                this->ClosePopup(); 
                return 0; 
            }
            this->mouse_left_down = true;
            ::SetCapture(hwnd);
            //LUIDebug(Hint) << "\r\n\t\tDown: " << this->captured << endl;
            break;
        case MouseEvent::Event_LButtonUp:
            // 没有按下就不算
            if (!this->mouse_left_down) return 0;
            this->mouse_left_down = false;
            //LUIDebug(Hint) << "\r\n\t\tUp  : " << this->captured << endl;
            ::ReleaseCapture();
            break;
        }
        this->DoMouseEventTs(arg);
    }
    // 其他消息处理
    else
        switch (message)
        {
            BOOL rc;
            PAINTSTRUCT ps;
        case detail::msg_post_destroy:
#ifndef NDEBUG
            LUIDebug(Warning) << "msg_post_destroy but not null" << endl;
#endif
            rc = ::DestroyWindow(hwnd);
            assert(rc && "DestroyWindow failed");
            return 0;
        case detail::msg_post_set_title:
            ::SetWindowTextW(hwnd, detail::sys(this->titlename.c_str()));
            return 0;
        case WM_SHOWWINDOW:
            // TODO: popup?
            break;
        case WM_SETCURSOR:
            ::SetCursor(reinterpret_cast<HCURSOR>(this->cursor.GetHandle()));
            break;
#ifdef LUI_ACCESSIBLE
        case WM_DESTROY:
            ::UiaReturnRawElementProvider(hwnd, 0, 0, nullptr);
            break;
        case WM_GETOBJECT:
            if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId)) {
                const auto window = this->viewport()->GetWindow();
                assert(window && "cannot be null");
                const auto root = CUIAccessibleWnd::FromWindow(*window);
                this->accessibility = true;
                return ::UiaReturnRawElementProvider(hwnd, wParam, lParam, root);
            }
            return 0;
#endif

            // 即时重置大小
#ifdef LUI_RESIZE_IMMEDIATELY
        case WM_EXITSIZEMOVE:
            this->OnResizeTs(LongUI::GetClientSize(hwnd));
            break;
#else
        case WM_ENTERSIZEMOVE:
            this->moving_resizing = true;
            break;
        case WM_EXITSIZEMOVE:
            this->moving_resizing = false;
            this->OnResizeTs(LongUI::GetClientSize(hwnd));
            break;
#endif


        //case WM_NCCALCSIZE:
        //    if (wParam) return 0;
        //    break;
#if 0
        case WM_SETFOCUS:
            ::CreateCaret(hwnd, (HBITMAP)NULL, 2, 20);
            ::SetCaretPos(50, 50);
            ::ShowCaret(hwnd);
            return 0;
        case WM_KILLFOCUS:
            ::HideCaret(hwnd);
            ::DestroyCaret();
        case WM_TOUCH:
            // TODO: TOUCH MESSAGE
            TOUCHINPUT;
            break;
#endif
            if (this->viewport()->RefWindow().config & CUIWindow::Config_Popup) {
                // CloseWindow 仅仅最小化窗口
                ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;
        case WM_PAINT:
            ::BeginPaint(hwnd, &ps);
            ::EndPaint(hwnd, &ps);
            break;
        case WM_SIZE:
            // 先关闭弹出窗口再说
            this->ClosePopup();
            // 最小化不算
            switch (wParam) { case SIZE_MINIMIZED: return 0; }
#ifndef LUI_RESIZE_IMMEDIATELY
            // 拖拽重置不算
            if (this->moving_resizing) return 0;
#endif
            // 重置大小
            this->OnResizeTs({ LOWORD(lParam), HIWORD(lParam) });
            return 0;
        case WM_KEYDOWN:
        case WM_KEYUP:
            static_assert(WM_KEYUP == WM_KEYDOWN + 1, "bad code");
            {
                const uint32_t plus = message - WM_KEYDOWN;
                const auto code = static_cast<uint32_t>(InputEvent::Event_KeyDown);
                const auto inev = static_cast<InputEvent>(code + plus);
                CUIDataAutoLocker locker;
                this->OnKeyDownUp(inev, static_cast<CUIInputKM::KB>(wParam));
            }
            return 0;
#if 0
        case WM_CONTEXTMENU:
            UIManager.DataLock();
            this->OnKeyDownUp(InputEvent::Event_KeyContext, static_cast<CUIInputKM::KB>(0));
            UIManager.DataUnlock();
            return 0;
#endif
        case WM_SYSKEYDOWN:
            // Alt+F4
            if (wParam == CUIInputKM::KB_F4) {
                const auto cfg = this->viewport()->RefWindow().config;
                if (cfg & CUIWindow::Config_AltF4ToCloseWindow)
                    ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            else {
                CUIDataAutoLocker locker;
                this->OnSystemKeyDown(static_cast<CUIInputKM::KB>(wParam), lParam);
            }
            return 0;
        case WM_GETMINMAXINFO:
            [](const LPMINMAXINFO info) noexcept {
                // TODO: 窗口最小大小
                info->ptMinTrackSize.x += DEFAULT_CONTROL_WIDTH;
                info->ptMinTrackSize.y += DEFAULT_CONTROL_HEIGHT;
            }(reinterpret_cast<MINMAXINFO*>(lParam));
            return 0;
        case WM_IME_CHAR:
            // 针对IME输入的优化 - 记录输入字符数量
            if (!Unicode::IsHighSurrogate(static_cast<char16_t>(wParam))) 
                this->ime_count++;
            break;
        case WM_CHAR:
            this->OnChar(static_cast<char16_t>(wParam));
            return 0;
        case WM_UNICHAR:
            this->OnCharTs(static_cast<char32_t>(wParam));
            return 0;
        case WM_IME_COMPOSITION:
            this->OnIME(hwnd);
            break;
        case WM_MOVING:
            // LOCK: 加锁?
            this->rect.top = reinterpret_cast<RECT*>(lParam)->top;
            this->rect.left = reinterpret_cast<RECT*>(lParam)->left;
            return true;
        case WM_CLOSE:
            this->viewport()->RefWindow().close_window();
            return 0;
        case WM_MOUSEACTIVATE:
            return ma_return_code;
        case WM_NCACTIVATE:
            // 非激活状态
            if (LOWORD(wParam) == WA_INACTIVE) {
                // 释放弹出窗口
                this->ClosePopup();
            }
            // 激活状态
            else {

            }
            break;
        case WM_SYSCHAR:
            if (wParam >= 'a' && wParam <= 'z')
                this->OnAccessKey(wParam - 'a');
            return 0;
        case WM_SYSCOMMAND:
            // 点击标题也要关闭弹出窗口
            if (wParam == (SC_MOVE | HTCAPTION))
                this->ClosePopup();
            break;
        case WM_DPICHANGED:
            this->OnDpiChanged(wParam, *reinterpret_cast<RectL*>(lParam));
            return 0;
        }
    // 未处理消息
    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

/// <summary>
/// Does the MSG null.
/// </summary>
/// <param name="hwnd">The HWND.</param>
/// <param name="message">The message.</param>
/// <param name="wParam">The w parameter.</param>
/// <param name="lParam">The l parameter.</param>
/// <returns></returns>
auto LongUI::CUIWindow::Private::DoMsgNull(
    const HWND hwnd, const UINT message,
    const WPARAM wParam, const LPARAM lParam) noexcept -> intptr_t {
    switch (message)
    {
        BOOL rc;
    case detail::msg_post_destroy:
        rc = ::DestroyWindow(hwnd);
        assert(rc && "DestroyWindow failed");
        return 0;
#ifdef LUI_ACCESSIBLE
    case WM_DESTROY:
        ::UiaReturnRawElementProvider(hwnd, 0, 0, nullptr);
        break;
#endif
    }
    // 未处理消息
    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

/// <summary>
/// Called when [IME].
/// </summary>
/// <returns></returns>
bool LongUI::CUIWindow::Private::OnIME(HWND hwnd) const noexcept {
    // 有效性
    if (!this->careted) return false;
    auto rect = this->caret;
    this->careted->MapToWindow(rect);

    bool code = false;
    const auto caret_x = static_cast<LONG>(rect.left);
    const auto caret_y = static_cast<LONG>(rect.top);
    const auto ctrl_w = this->rect.width;
    const auto ctrl_h = this->rect.height;
    if (caret_x >= 0 && caret_y >= 0 && caret_x < ctrl_w && caret_y < ctrl_h) {
        HIMC imc = ::ImmGetContext(hwnd);
        // TODO: caret 高度可能不是字体高度
        const auto caret_h = static_cast<LONG>(rect.bottom - rect.top);
        if (::ImmGetOpenStatus(imc)) {
            COMPOSITIONFORM cf = { 0 };
            cf.dwStyle = CFS_POINT;
            cf.ptCurrentPos.x = caret_x;
            cf.ptCurrentPos.y = caret_y;
            if (::ImmSetCompositionWindow(imc, &cf)) {
                LOGFONTW lf = { 0 };
                lf.lfHeight = caret_h;
                // TODO: 富文本支持
                //lf.lfItalic
                if (::ImmSetCompositionFontW(imc, &lf)) code = true;
            }
        }
        ::ImmReleaseContext(hwnd, imc);
    }
    return code;
}

// ----------------------------------------------------------------------------
// ------------------- Graphics
// ----------------------------------------------------------------------------
#include <core/ui_manager.h>
#include <graphics/ui_graphics_impl.h>

#pragma comment(lib, "dxguid")
//#pragma comment(lib, "dwmapi")

/// <summary>
/// Recreates this instance.
/// </summary>
/// <param name="hwnd">The HWND.</param>
/// <returns></returns>
auto LongUI::CUIWindow::Private::Recreate(HWND hwnd) noexcept -> Result {
    // 可能是节能模式?
    if (!hwnd) return { Result::RS_FALSE };
    if (this->is_skip_render()) return{ Result::RS_OK };
    // 全渲染
    this->mark_fr_for_update();
    // 创建渲染资源, 需要渲染锁
    CUIRenderAutoLocker locker;
    assert(this->bitmap == nullptr && "call release first");
    assert(this->swapchan == nullptr && "call release first");
    // 保证内存不泄漏
    this->release_data();
    Result hr = { Result::RS_OK };
    // 创建交换酱
    if (hr) {
        // 获取窗口大小
        RECT client_rect; ::GetClientRect(hwnd, &client_rect);
        const Size2L wndsize = {
            client_rect.right - client_rect.left,
            client_rect.bottom - client_rect.top
        };
        // 设置逻辑大小
        this->wndbuf_logical = wndsize;
        // 交换链信息
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        // 检测DComp支持
        if (this->is_direct_composition()) {
            swapChainDesc.Width = detail::get_fit_size_for_trans(wndsize.width);
            swapChainDesc.Height = detail::get_fit_size_for_trans(wndsize.height);
        }
        else {
            swapChainDesc.Width = wndsize.width;
            swapChainDesc.Height = wndsize.height;
        }
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
#ifdef LUI_RESIZE_IMMEDIATELY
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
#else
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
#endif
        // TODO: 延迟等待
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        swapChainDesc.Flags = 0;
        // SWAP酱
        IDXGISwapChain1* sc = nullptr;
        // 检查DComp支持
        if (this->is_direct_composition()) {
            // DirectComposition桌面应用程序
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            // 创建DirectComposition交换链
            hr = { UIManager.RefGraphicsFactory().CreateSwapChainForComposition(
                &UIManager.Ref3DDevice(),
                &swapChainDesc,
                nullptr,
                &sc
            ) };
            longui_debug_hr(hr, L"UIManager_DXGIFactory->CreateSwapChainForComposition faild");
        }
        // 创建一般的交换链
        else {
            // 一般桌面应用程序
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            // 利用窗口句柄创建交换链
            const auto temp = UIManager.RefGraphicsFactory().CreateSwapChainForHwnd(
                &UIManager.Ref3DDevice(),
                hwnd,
                &swapChainDesc,
                nullptr,
                nullptr,
                &sc
            );
            // 第一次尝试失败?
#if defined(LUI_WIN10_ONLY) || !defined(LUI_RESIZE_IMMEDIATELY)
            hr.code = temp;
#else
            // 第一次尝试失败? Win7的场合
            if (FAILED(temp)) {
                swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
                hr.code = UIManager.RefGraphicsFactory().CreateSwapChainForHwnd(
                    &UIManager.Ref3DDevice(),
                    hwnd,
                    &swapChainDesc,
                    nullptr,
                    nullptr,
                    &sc
                );
            }
#endif
            longui_debug_hr(hr, L"GraphicsFactory.CreateSwapChainForHwnd faild");
        }
        // 设置 SWAP酱
        this->swapchan = static_cast<I::Swapchan*>(sc);
    }
    IDXGISurface* backbuffer = nullptr;
    // 利用交换链获取Dxgi表面
    if (hr) {
        hr = { this->swapchan->GetBuffer(
            0, IID_IDXGISurface, reinterpret_cast<void**>(&backbuffer)
        ) };
        longui_debug_hr(hr, L"swapchan->GetBuffer faild");
    }
    // 利用Dxgi表面创建位图
    if (hr) {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        ID2D1Bitmap1* bmp = nullptr;
        hr = { UIManager.Ref2DRenderer().CreateBitmapFromDxgiSurface(
            backbuffer,
            &bitmapProperties,
            &bmp
        ) };
        this->bitmap = static_cast<I::Bitmap*>(bmp);
        longui_debug_hr(hr, L"2DRenderer.CreateBitmapFromDxgiSurface faild");
    }
    LongUI::SafeRelease(backbuffer);
    // 使用DComp
    if (this->is_direct_composition()) {
        hr = impl::create_dcomp(this->dcomp_buf, hwnd, *this->swapchan);
    }
    return hr;
}


/// <summary>
/// Releases the data.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::release_data() noexcept {
#ifndef NDEBUG
    //if (this->swapchan) {
    //    this->swapchan->AddRef();
    //    const auto count = this->swapchan->Release();
    //    int bk = 9;
    //}
#endif
    LongUI::SafeRelease(this->bitmap);
    LongUI::SafeRelease(this->swapchan);
    impl::release_dcomp(this->dcomp_buf);
}

/// <summary>
/// Resizes the window buffer.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::resize_window_buffer() noexcept {
    this->flag_sized = false;
    assert(this->rect.width && this->rect.height);
    // 检测是否无需修改
    const auto olds = this->wndbuf_logical;
    const auto samew = olds.width == this->rect.width;
    const auto sameh = olds.height == this->rect.height;
    if (samew && sameh) return;
    // 更新逻辑大小
    this->wndbuf_logical.width = this->rect.width;
    this->wndbuf_logical.height = this->rect.height;
    // 渲染区
    //CUIRenderAutoLocker locker;
    // 取消引用
    //UIManager.Ref2DRenderer().SetTarget(nullptr);
    // 强行重置flag
    bool force_resize = true;
    // 目标大小
    auto target = this->wndbuf_logical;
    // DComp支持
    if (this->is_direct_composition()) {
        const auto old_realx = detail::get_fit_size_for_trans(olds.width);
        const auto old_realy = detail::get_fit_size_for_trans(olds.height);
        target.width = detail::get_fit_size_for_trans(target.width);
        target.height = detail::get_fit_size_for_trans(target.height);
        // 透明窗口只需要比实际大小大就行
        if (old_realx == target.width && old_realy == target.height)
            force_resize = false;
    }
    // 重置缓冲帧大小
    if (force_resize) {
        IDXGISurface* dxgibuffer = nullptr;
        LongUI::SafeRelease(this->bitmap);
        // TODO: 延迟等待
        Result hr = { this->swapchan->ResizeBuffers(
            2, target.width, target.height,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            // DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
            0
        ) };
        longui_debug_hr(hr, L"m_pSwapChain->ResizeBuffers faild");
        // TODO: RecreateResources 检查
        if (hr.code == DXGI_ERROR_DEVICE_REMOVED || 
            hr.code == DXGI_ERROR_DEVICE_RESET) {
            //UIManager.RecreateResources();
            LUIDebug(Hint) << L"Recreate device" << LongUI::endl;
        }
        // 利用交换链获取Dxgi表面
        if (hr) {
            hr = { this->swapchan->GetBuffer(
                0,
                IID_IDXGISurface,
                reinterpret_cast<void**>(&dxgibuffer)
            ) };
            longui_debug_hr(hr, L"m_pSwapChain->GetBuffer faild");
        }
        // 利用Dxgi表面创建位图
        if (hr) {
            D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );
            ID2D1Bitmap1* bmp = nullptr;
            hr = { UIManager.Ref2DRenderer().CreateBitmapFromDxgiSurface(
                dxgibuffer,
                &bitmapProperties,
                &bmp
            ) };
            this->bitmap = static_cast<I::Bitmap*>(bmp);
            longui_debug_hr(hr, L"UIManager_RenderTarget.CreateBitmapFromDxgiSurface faild");
        }
        // 重建失败?
        LongUI::SafeRelease(dxgibuffer);
#ifndef NDEBUG
        LUIDebug(Hint)
            << "resize to["
            << target.width
            << 'x'
            << target.height
            << ']'
            << LongUI::endl;
#endif // !NDEBUG

        // TODO: 错误处理
        if (!hr) {
            LUIDebug(Error) << L" Recreate FAILED!" << LongUI::endl;
            UIManager.OnErrorInfoLost(hr, Occasion_Resizing);
        }
    }
}

/// <summary>
/// Renders this instance.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::Private::Render(const UIViewport& v) const noexcept->Result {
    // 跳过该帧?
    if (this->is_skip_render()) return { Result::RS_OK };
    // 请求渲染
    if (this->is_r_for_render()) {
        // 重置窗口缓冲帧大小
        if (this->flag_sized) this->force_resize_window_buffer();
        // 数据无效
        if (!this->bitmap) return { Result::RS_FALSE };
        // 开始渲染
        this->begin_render();
        // 矩形列表
        const auto rects = this->dirty_rect_presenting;
        const auto count = this->is_fr_for_render() ? 0 : this->dirty_count_presenting;
        // 正式渲染
        CUIControlControl::RecursiveRender(v, rects, count);
#if 0
        //if (this->viewport()->GetWindow()->GetParent())
        LUIDebug(Log) << "End of Frame" << this << endl;
#endif
        // 结束渲染
        return this->end_render();
    }
    return{ Result::RS_OK };
}


// longui::impl
namespace LongUI { namespace impl {
    // get subpixel text render lelvel
    void get_subpixel_text_rendering(uint32_t& level) noexcept {
        level = D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;
    }
}}

/// <summary>
/// Begins the render.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::begin_render() const noexcept {
    // TODO: 完成
    auto& renderer = UIManager.Ref2DRenderer();
    // 文本抗锯齿等级
    const auto tamode = static_cast<D2D1_TEXT_ANTIALIAS_MODE>(this->text_antialias);
    // 设置文本渲染策略
    renderer.SetTextAntialiasMode(tamode);
    // 设为当前渲染对象
    renderer.SetTarget(this->bitmap);
    // 开始渲染
    renderer.BeginDraw();
    // 设置转换矩阵
#if 1
    renderer.SetTransform(D2D1::Matrix3x2F::Identity());
#else
    renderer.SetTransform(&impl::d2d(m_pViewport->box.world));
#endif
    // 清空背景
    renderer.Clear(auto_cast(clear_color));
}


/// <summary>
/// Ends the render.
/// </summary>
/// <returns></returns>
auto LongUI::CUIWindow::Private::end_render() const noexcept->Result {
    auto& renderer = UIManager.Ref2DRenderer();
    renderer.SetTransform({ 1.f,0.f,0.f,1.f,0.f,0.f });
    // 渲染插入符号
    this->render_caret(renderer);
    // 渲染焦点矩形
    this->render_focus(renderer);
#ifndef NDEBUG
    // 显示
    if (UIManager.flag & ConfigureFlag::Flag_DbgDrawDirtyRect) {
        // 水平扫描线: 全局渲染
        if (this->is_fr_for_render()) {
            assert(this->rect.height);
            const float y = float(dbg_full_render_counter % this->rect.height);
            renderer.PushAxisAlignedClip(
                D2D1_RECT_F{ 0, y, float(rect.width), y + 1 },
                D2D1_ANTIALIAS_MODE_ALIASED
            );
            renderer.Clear(D2D1::ColorF{ 0x000000 });
            renderer.PopAxisAlignedClip();
        }
        // 随机颜色方格: 增量渲染
        else {
            static float s_color_value = 0.f;
            constexpr float s_color_step = 0.005f * 360.f;
            ColorSystem::HSLA hsl;
            hsl.a = 0.5f; hsl.s = 1.f; hsl.l = 0.36f; hsl.h = s_color_value;
            // 遍历脏矩形
            std::for_each(
                this->dirty_rect_presenting,
                this->dirty_rect_presenting + this->dirty_count_presenting,
                [&](const RectF& rect) noexcept {
                auto& bursh = CUIManager::RefCCBrush(hsl.toRGBA());
                renderer.FillRectangle(auto_cast(rect), &bursh);
                hsl.h += s_color_step;
                if (hsl.h > 360.f) hsl.h = 0.f;
            });
            s_color_value = hsl.h;
        }
    }
#endif
    // 结束渲染
    Result hr = { renderer.EndDraw() };
    // 清除目标
    renderer.SetTarget(nullptr);
    // TODO: 完全渲染
    if (this->is_fr_for_render()) {
#ifndef NDEBUG
        CUITimeMeterH meter;
        meter.Start();
        hr = { this->swapchan->Present(0, 0) };
        const auto time = meter.Delta_ms<float>();
        if (time > 9.f) {
            LUIDebug(Hint)
                << "present time: "
                << time
                << "ms"
                << LongUI::endl;
        }
        //assert(time < 1000.f && "took too much time, check plz.");
#else
        hr = { this->swapchan->Present(0, 0) };
#endif
        longui_debug_hr(hr, L"swapchan->Present full rendering faild");
    }
    // 增量渲染
    else {
        // 呈现参数设置
        RECT scroll = { 0, 0, 
            this->wndbuf_logical.width, 
            this->wndbuf_logical.height
        };
        RECT rects[LongUI::DIRTY_RECT_COUNT];
        // 转换为整型
        const auto count = this->dirty_count_presenting;
        for (uint32_t i = 0; i != count; ++i) {
            const auto& src = this->dirty_rect_presenting[i];
            const auto right = static_cast<LONG>(std::ceil(src.right));
            const auto bottom = static_cast<LONG>(std::ceil(src.bottom));
            // 写入
            rects[i].top = std::max(static_cast<LONG>(src.top), 0l);
            rects[i].left = std::max(static_cast<LONG>(src.left), 0l);
            rects[i].right = std::min(right, scroll.right);
            rects[i].bottom = std::min(bottom, scroll.bottom);
        }
        // 填充参数
        DXGI_PRESENT_PARAMETERS present_parameters;
        present_parameters.DirtyRectsCount = count;
        present_parameters.pDirtyRects = rects;
        present_parameters.pScrollRect = &scroll;
        present_parameters.pScrollOffset = nullptr;
        // 提交该帧
#ifndef NDEBUG
        CUITimeMeterH meter;
        meter.Start();
        hr = { this->swapchan->Present1(0, 0, &present_parameters) };
        const auto time = meter.Delta_ms<float>();
        if (time > 9.f) {
            LUIDebug(Hint)
                << "present time: "
                << time
                << "ms"
                << LongUI::endl;
        }
        //assert(time < 1000.f && "took too much time, check plz.");
#else
        hr = { this->swapchan->Present1(0, 0, &present_parameters) };
#endif
        longui_debug_hr(hr, L"swapchan->Present full rendering faild");
    }
    // 收到重建消息/设备丢失时 重建UI
    constexpr int32_t DREMOVED = DXGI_ERROR_DEVICE_REMOVED;
    constexpr int32_t DRESET = DXGI_ERROR_DEVICE_RESET;
#ifdef NDEBUG
    if (hr.code == DREMOVED || hr.code == DRESET) {
        UIManager.NeedRecreate();
        hr = { Result::RS_FALSE };
    }
#else
    // TODO: 测试 test_D2DERR_RECREATE_TARGET
    if (hr.code == DREMOVED || hr.code == DRESET ) {
        UIManager.NeedRecreate();
        hr = { Result::RS_FALSE };
        LUIDebug(Hint) << "D2DERR_RECREATE_TARGET!" << LongUI::endl;
    }
    assert(hr);
    // 调试
    if (this->is_fr_for_render())
        ++const_cast<uint32_t&>(dbg_full_render_counter);
    else
        ++const_cast<uint32_t&>(dbg_dirty_render_counter);
    // 更新调试信息
    wchar_t buffer[1024];
    std::swprintf(
        buffer, 1024,
        L"<%ls>: FRC: %d\nDRC: %d\nDRRC: %d",
        L"NAMELESS",
        int(dbg_full_render_counter),
        int(dbg_dirty_render_counter),
        int(0)
    );
    // 设置显示
    /*UIManager.RenderUnlock();
    this->dbg_uiwindow->SetTitleName(buffer);
    UIManager.RenderLock();*/
#endif
    return hr;
}


/// <summary>
/// Renders the caret.
/// </summary>
/// <param name="renderer">The renderer.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::render_caret(I::Renderer2D& renderer) const noexcept {
    // TODO: 脏矩形渲染时候可能没有必要渲染?
    // 渲染插入符号
    if (this->careted /*&& this->caret_ok*/) {
        // 保持插入符号的清晰
        renderer.SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
#if 0
        // 反色操作, 但是较消耗GPU资源
        renderer.DrawImage(
            &img->RefBitmap(),
            auto_cast(reinterpret_cast<Point2F*>(&des)), &auto_cast(rc),
            D2D1_INTERPOLATION_MODE_LINEAR,
            //D2D1_COMPOSITE_MODE_SOURCE_OVER
            D2D1_COMPOSITE_MODE_MASK_INVERT
        );
#endif
        auto rect = this->caret;
        this->careted->MapToWindow(rect);
        auto& brush = UIManager.RefCCBrush(this->caret_color);
        renderer.FillRectangle(auto_cast(rect), &brush);
        renderer.SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }
}



/// <summary>
/// Renders the focus.
/// </summary>
/// <param name="renderer">The renderer.</param>
/// <returns></returns>
void LongUI::CUIWindow::Private::render_focus(I::Renderer2D& renderer) const noexcept {
#ifdef LUI_DRAW_FOCUS_RECT
    // TODO: 脏矩形渲染时候可能没有必要渲染?
    // 渲染焦点矩形
    if (this->focus_ok && this->focused) {
        // 脏矩形符号的清晰
        renderer.SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
        auto rect = this->foucs;
        this->focused->MapToWindow(rect);
        LongUI::NativeStyleFocus(rect);
    }
#endif
}


/// <summary>
/// Gets the system last error.
/// </summary>
/// <returns></returns>
auto LongUI::Result::GetSystemLastError() noexcept -> Result {
    const auto x = ::GetLastError();
    constexpr int32_t UI_FACILITY = 7;
    return{ ((int32_t)(x) <= 0 ? ((int32_t)(x)) :
        ((int32_t)(((x) & 0x0000FFFF) | (UI_FACILITY << 16) | 0x80000000))) };
}

/// <summary>
/// Empties the render.
/// </summary>
/// <returns></returns>
void LongUI::CUIWindow::Private::EmptyRender() noexcept {
    if (this->bitmap) {
        CUIRenderAutoLocker locker;
        this->mark_fr_for_render();
        this->begin_render();
        // TODO: 错误处理
        this->end_render();
    }
}


#ifndef NDEBUG

void ui_dbg_set_window_title(
    LongUI::CUIWindow* pwnd,
    const char * title) noexcept {
    if (pwnd) pwnd->SetTitleName(LongUI::CUIString::FromUtf8(title));
}

void ui_dbg_set_window_title(
    HWND hwnd,
    const char * title) noexcept {
    assert(hwnd && "bad action");
    const auto aaa = LongUI::CUIString::FromUtf8(title);
    ::SetWindowTextW(hwnd, LongUI::detail::sys(aaa.c_str()));
}

#endif
