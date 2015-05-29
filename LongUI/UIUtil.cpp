
#include "LongUI.h"

// UIString �����ַ���
void LongUI::CUIString::Set(const wchar_t* str, uint32_t length) noexcept {
    assert(str && "<LongUI::CUIString::CUIString@const wchar_t*> str == null");
    // δ֪�����
    if (!length && *str) { length = ::wcslen(str); }
    // �����Ļ�
    if (length > m_cBufferLength) {
        m_cBufferLength = length + LongUIStringLength / 2;
        // �����ͷ�
        if (m_pString != m_aDataStatic) {
            LongUICtrlFree(m_pString);
        }
        // �����ڴ�
        m_pString = reinterpret_cast<wchar_t*>(
            LongUICtrlAlloc(sizeof(wchar_t) * (m_cBufferLength))
            );

    }
    // ��������
    assert(str && "<LongUI::CUIString::CUIString@const wchar_t*> m_pString == null");
    ::wcscpy(m_pString, str);
    m_cLength = length;
}

// UIString �ַ�����������
LongUI::CUIString::~CUIString() noexcept {
    // �ͷ�����
    if (m_pString && m_pString != m_aDataStatic) {
        LongUICtrlFree(m_pString);
    }
    m_pString = nullptr;
    m_cLength = 0;
}

// ���ƹ��캯��
LongUI::CUIString::CUIString(const LongUI::CUIString& obj) noexcept {
    // ����
    if (obj.m_pString != obj.m_aDataStatic) {
        m_pString = reinterpret_cast<wchar_t*>(
            LongUICtrlAlloc(sizeof(wchar_t) * (obj.m_cLength + 1))
            );
    }
    // ��������
    assert(m_pString && "<LongUI::CUIString::CUIString@const CUIString&> m_pString == null");
    ::wcscpy(m_pString, obj.m_pString);
    m_cLength = obj.m_cLength;
}

// move���캯��
LongUI::CUIString::CUIString(LongUI::CUIString&& obj) noexcept {
    // ����
    if (obj.m_pString != obj.m_aDataStatic) {
        m_pString = obj.m_pString;
    }
    else {
        // ��������
        ::wcscpy(m_aDataStatic, obj.m_aDataStatic);
    }
    m_cLength = obj.m_cLength;
    obj.m_cLength = 0;
    obj.m_pString = nullptr;
}

// += ����
//const LongUI::CUIString& LongUI::CUIString::operator+=(const wchar_t*);

// DllControlLoader ���캯��
LongUI::DllControlLoader::DllControlLoader(
    const wchar_t* file, const wchar_t* name, const char* proc)
    noexcept : dll(::LoadLibraryW(file)){
    CreateControlFunction tmpfunc = nullptr;
    // ������
    assert(file && name && proc && "bad argument");
    // ��ȡ������ַ
    if (this->dll && (tmpfunc=reinterpret_cast<CreateControlFunction>(::GetProcAddress(dll, proc)))) {
        // ǿ��ת��
        const_cast<CreateControlFunction&>(this->function) = tmpfunc;
        // ��Ӻ���ӳ��
        UIManager.AddS2CPair(name, this->function);
    }
}

// DllControlLoader ��������
LongUI::DllControlLoader::~DllControlLoader() noexcept {
    if (this->dll) {
        ::FreeLibrary(this->dll);
        const_cast<HMODULE&>(this->dll) = nullptr;
    }
}

// UIIcon ���ƹ��캯��
LongUI::UIIcon::UIIcon(const UIIcon & obj) noexcept {
    if (obj.m_hIcon) {
        this->m_hIcon = ::CopyIcon(obj.m_hIcon);
    }
}

// UIIcon �ƶ����캯��
LongUI::UIIcon::UIIcon(UIIcon && obj) noexcept {
    if (obj.m_hIcon) {
        this->m_hIcon = obj.m_hIcon;
        obj.m_hIcon = nullptr;
    }
}


// UIIcon ���캯��
LongUI::UIIcon::UIIcon(const Meta &) noexcept {
    assert(!"not compalte");
}



#define INITBUFFER if(prefix) { ::strcpy(buffer, prefix); } else { *buffer = 0; }
#ifdef Attr
#error "check it"
#endif
#define Attr(a) attribute(::strcat(buffer, a)).value()

// CUIElement ���캯��
LongUI::CUIElement::CUIElement(const pugi::xml_node node, const char* prefix) noexcept:
animationc(AnimationType::Type_QuadraticEaseOut),
animationo(AnimationType::Type_QuadraticEaseOut){
    // ��ʼ��Meta
    ZeroMemory(metas, sizeof(metas));
    // ��ʼ����ɫ
    colors[LongUI::Status_Disabled] = D2D1::ColorF(LongUIDefaultDisabledColor);
    colors[LongUI::Status_Normal] = D2D1::ColorF(LongUIDefaultNormalColor);
    colors[LongUI::Status_Hover] = D2D1::ColorF(LongUIDefaultHoverColor);
    colors[LongUI::Status_Pushed] = D2D1::ColorF(LongUIDefaultClickedColor);
    if (!node)  return;
    char buffer[LongUIStringBufferLength]; *buffer = 0;
    const char* str = nullptr;
    // ��ȡ��������
    INITBUFFER;
    if (str = node.Attr("animationtype")) {
        animationo.type  = animationc.type = static_cast<AnimationType>(::LongUI::AtoI(str));
       
    }
    // ��ȡ��������ʱ��
    INITBUFFER;
    if (str = node.Attr("animationduration")) {
        animationo.duration = animationc.duration = ::LongUI::AtoF(str);
    }
    {
        // ����״̬��Meta
        INITBUFFER;
        if (str = node.Attr("disabledmeta")) {
            UIManager.GetMeta(::LongUI::AtoI(str), metas[Status_Disabled]);
        }
        // ����״̬����ɫ(2ѡһ)
        else {
            INITBUFFER;
            UIControl::MakeColor(node.Attr("disabledcolor"), colors[Status_Disabled]);
        }
    }
    {
        // ͨ��״̬��Meta
        INITBUFFER;
        if (str = node.Attr("normalmeta")) {
            UIManager.GetMeta(::LongUI::AtoI(str), metas[Status_Normal]);
        }
        // ͨ��״̬��ɫ(2ѡһ)
        else {
            INITBUFFER;
            UIControl::MakeColor(node.Attr("normalcolor"), colors[Status_Normal]);
        }
    }
    {
        // ����״̬��Meta
        INITBUFFER;
        if (str = node.Attr("hovermeta")) {
            UIManager.GetMeta(::LongUI::AtoI(str), metas[Status_Hover]);
        }
        // ����״̬��ɫ(2ѡһ)
        else {
            INITBUFFER;
            UIControl::MakeColor(node.Attr("hovercolor"), colors[Status_Hover]);
        }
    }
    {
        // ���״̬��Meta
        INITBUFFER;
        if (str = node.Attr("pushedmeta")) {
            UIManager.GetMeta(::LongUI::AtoI(str), metas[Status_Pushed]);
        }
        // ���״̬��ɫ(2ѡһ)
        else {
            INITBUFFER;
            UIControl::MakeColor(node.Attr("pushedcolor"), colors[Status_Pushed]);
        }
    }
}

// ͼ��
// A--B-----C--D
// | 0|  1  | 2|
// E--F-----G--H
// |  |  4  |  |
// | 3|     | 5|
// I--J-----K--L
// | 6|  7  | 8|
// M--N-----O--P

// ����λ��A
#define RenderMeta_SetZoneA(tmp, src, wd4, hd4) (tmp)[0] = (src)[0];        (tmp)[1] = (src)[1];
// ����λ��B
#define RenderMeta_SetZoneB(tmp, src, wd4, hd4) (tmp)[0] = (src)[0] + wd4;  (tmp)[1] = (src)[1];
// ����λ��C
#define RenderMeta_SetZoneC(tmp, src, wd4, hd4) (tmp)[0] = (src)[2] - wd4;  (tmp)[1] = (src)[1];
// ����λ��D
#define RenderMeta_SetZoneD(tmp, src, wd4, hd4) (tmp)[0] = (src)[2];        (tmp)[1] = (src)[1];
// ����λ��E
#define RenderMeta_SetZoneE(tmp, src, wd4, hd4) (tmp)[0] = (src)[0];        (tmp)[1] = (src)[1] + hd4;
// ����λ��F
#define RenderMeta_SetZoneF(tmp, src, wd4, hd4) (tmp)[0] = (src)[0] + wd4;  (tmp)[1] = (src)[1] + hd4;
// ����λ��G
#define RenderMeta_SetZoneG(tmp, src, wd4, hd4) (tmp)[0] = (src)[2] - wd4;  (tmp)[1] = (src)[1] + hd4;
// ����λ��H
#define RenderMeta_SetZoneH(tmp, src, wd4, hd4) (tmp)[0] = (src)[2];        (tmp)[1] = (src)[1] + hd4;
// ����λ��I
#define RenderMeta_SetZoneI(tmp, src, wd4, hd4) (tmp)[0] = (src)[0];        (tmp)[1] = (src)[3] - hd4;
// ����λ��J
#define RenderMeta_SetZoneJ(tmp, src, wd4, hd4) (tmp)[0] = (src)[0] + wd4;  (tmp)[1] = (src)[3] - hd4;
// ����λ��K
#define RenderMeta_SetZoneK(tmp, src, wd4, hd4) (tmp)[0] = (src)[2] - wd4;  (tmp)[1] = (src)[3] - hd4;
// ����λ��L
#define RenderMeta_SetZoneL(tmp, src, wd4, hd4) (tmp)[0] = (src)[2];        (tmp)[1] = (src)[3] - hd4;
// ����λ��M
#define RenderMeta_SetZoneM(tmp, src, wd4, hd4) (tmp)[0] = (src)[0];        (tmp)[1] = (src)[3];
// ����λ��N
#define RenderMeta_SetZoneN(tmp, src, wd4, hd4) (tmp)[0] = (src)[0] + wd4;  (tmp)[1] = (src)[3];
// ����λ��O
#define RenderMeta_SetZoneO(tmp, src, wd4, hd4) (tmp)[0] = (src)[2] - wd4;  (tmp)[1] = (src)[3];
// ����λ��P
#define RenderMeta_SetZoneP(tmp, src, wd4, hd4) (tmp)[0] = (src)[2];        (tmp)[1] = (src)[3];



// ��Ⱦλͼ
#define RenderMeta_DrawBitmap(i, m) \
    this->target->DrawBitmap(\
        meta.bitmap, tmp_des_rect + i, opa,\
        static_cast<D2D1_INTERPOLATION_MODE>(m),\
        tmp_src_rect + i, nullptr)

// ��ȾͼԪ
void LongUI::CUIElement::RenderMeta(Meta& meta, D2D1_RECT_F* des_rect, float opa) noexcept {
    if (meta.rule == BitmapRenderRule::Rule_ButtonLike) {
#pragma region Button Like Rule �������ƹ���
        auto src_width = meta.src_rect.right - meta.src_rect.left;
        auto src_height= meta.src_rect.bottom - meta.src_rect.top;
        auto width_difference = des_rect->right - des_rect->left - src_width;
        auto height_difference = des_rect->bottom - des_rect->top - src_height;
        // ��Ϊ�Ź�����Ⱦ
        D2D1_RECT_F tmp_src_rect[9], tmp_des_rect[9];
        // �߶�һ��?(���ʸ�)
        if (height_difference < 0.5f  && width_difference > -0.5f) {
            // ���һ��?
            if (width_difference < 0.5f  && width_difference > -0.5f) {
                // ֱ����Ⱦ
                this->target->DrawBitmap(
                    meta.bitmap,
                    des_rect, opa,
                    D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                    &meta.src_rect,
                    nullptr
                    );
                return;
            }
            float src_width_div4 = src_width * 0.25f;
            // ����1  ���� ABMN A->N
            RenderMeta_SetZoneA(&tmp_src_rect[0].left, &meta.src_rect.left, src_width_div4, void);
            RenderMeta_SetZoneN(&tmp_src_rect[0].right, &meta.src_rect.left, src_width_div4, void);
            RenderMeta_SetZoneA(&tmp_des_rect[0].left, &(des_rect->left), src_width_div4, void);
            RenderMeta_SetZoneN(&tmp_des_rect[0].right, &(des_rect->left), src_width_div4, void);
            // ����2  ���� BCNO B->O
            RenderMeta_SetZoneB(&tmp_src_rect[1].left, &meta.src_rect.left, src_width_div4, void);
            RenderMeta_SetZoneO(&tmp_src_rect[1].right, &meta.src_rect.left, src_width_div4, void);
            RenderMeta_SetZoneB(&tmp_des_rect[1].left, &(des_rect->left), src_width_div4, void);
            RenderMeta_SetZoneO(&tmp_des_rect[1].right, &(des_rect->left), src_width_div4, void);
            // ����3  ���� CDOP C->P
            RenderMeta_SetZoneC(&tmp_src_rect[2].left, &meta.src_rect.left, src_width_div4, void);
            RenderMeta_SetZoneP(&tmp_src_rect[2].right, &meta.src_rect.left, src_width_div4, void);
            RenderMeta_SetZoneC(&tmp_des_rect[2].left, &(des_rect->left), src_width_div4, void);
            RenderMeta_SetZoneP(&tmp_des_rect[2].right, &(des_rect->left), src_width_div4, void);
            // ��Ⱦ
            RenderMeta_DrawBitmap(0, 0);
            RenderMeta_DrawBitmap(1, meta.interpolation);
            RenderMeta_DrawBitmap(2, 0);
            return;
        }
        // ���һ��?
        else if (width_difference < 0.5f  && width_difference > -0.5f) {
            float src_height_div4 = src_height * 0.25f;
            // ����1  ���� ADEH A->H
            RenderMeta_SetZoneA(&tmp_src_rect[0].left, &meta.src_rect.left, void, src_height_div4);
            RenderMeta_SetZoneH(&tmp_src_rect[0].right, &meta.src_rect.left, void, src_height_div4);
            RenderMeta_SetZoneA(&tmp_des_rect[0].left, &(des_rect->left), void, src_height_div4);
            RenderMeta_SetZoneH(&tmp_des_rect[0].right, &(des_rect->left), void, src_height_div4);
            // ����2  ���� EHIL E->L
            RenderMeta_SetZoneE(&tmp_src_rect[1].left, &meta.src_rect.left, void, src_height_div4);
            RenderMeta_SetZoneL(&tmp_src_rect[1].right, &meta.src_rect.left, void, src_height_div4);
            RenderMeta_SetZoneE(&tmp_des_rect[1].left, &(des_rect->left), void, src_height_div4);
            RenderMeta_SetZoneL(&tmp_des_rect[1].right, &(des_rect->left), void, src_height_div4);
            // ����3  ���� ILMP I->P
            RenderMeta_SetZoneI(&tmp_src_rect[2].left, &meta.src_rect.left, void, src_height_div4);
            RenderMeta_SetZoneP(&tmp_src_rect[2].right, &meta.src_rect.left, void, src_height_div4);
            RenderMeta_SetZoneI(&tmp_des_rect[2].left, &(des_rect->left), void, src_height_div4);
            RenderMeta_SetZoneP(&tmp_des_rect[2].right, &(des_rect->left), void, src_height_div4);
            // ��Ⱦ
            RenderMeta_DrawBitmap(0, 0);
            RenderMeta_DrawBitmap(1, meta.interpolation);
            RenderMeta_DrawBitmap(2, 0);
            return;
        }
        // һ��
        else {
            float hdiv4 = src_height * 0.25f;
            float wdiv4 = src_width * 0.25f;
            // ����0  ���� ABEF A->F
            RenderMeta_SetZoneA(&tmp_src_rect[0].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneF(&tmp_src_rect[0].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneA(&tmp_des_rect[0].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneF(&tmp_des_rect[0].right, &(des_rect->left), wdiv4, hdiv4);
            // ����1  ���� BCFG B->G
            RenderMeta_SetZoneB(&tmp_src_rect[1].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneG(&tmp_src_rect[1].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneB(&tmp_des_rect[1].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneG(&tmp_des_rect[1].right, &(des_rect->left), wdiv4, hdiv4);
            // ����2  ���� CDGH C->H
            RenderMeta_SetZoneC(&tmp_src_rect[2].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneH(&tmp_src_rect[2].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneC(&tmp_des_rect[2].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneH(&tmp_des_rect[2].right, &(des_rect->left), wdiv4, hdiv4);
            // ����3  ���� EFIJ E->J
            RenderMeta_SetZoneE(&tmp_src_rect[3].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneJ(&tmp_src_rect[3].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneE(&tmp_des_rect[3].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneJ(&tmp_des_rect[3].right, &(des_rect->left), wdiv4, hdiv4);
            // ����4  ���� FGJK F->K
            RenderMeta_SetZoneF(&tmp_src_rect[4].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneK(&tmp_src_rect[4].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneF(&tmp_des_rect[4].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneK(&tmp_des_rect[4].right, &(des_rect->left), wdiv4, hdiv4);
            // ����5  ���� GHKL G->L
            RenderMeta_SetZoneG(&tmp_src_rect[5].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneL(&tmp_src_rect[5].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneG(&tmp_des_rect[5].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneL(&tmp_des_rect[5].right, &(des_rect->left), wdiv4, hdiv4);
            // ����6  ���� IJMN I->N
            RenderMeta_SetZoneI(&tmp_src_rect[6].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneN(&tmp_src_rect[6].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneI(&tmp_des_rect[6].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneN(&tmp_des_rect[6].right, &(des_rect->left), wdiv4, hdiv4);
            // ����7  ���� JKNO J->O
            RenderMeta_SetZoneJ(&tmp_src_rect[7].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneO(&tmp_src_rect[7].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneJ(&tmp_des_rect[7].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneO(&tmp_des_rect[7].right, &(des_rect->left), wdiv4, hdiv4);
            // ����8  ���� KLOP K->P
            RenderMeta_SetZoneK(&tmp_src_rect[8].left, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneP(&tmp_src_rect[8].right, &meta.src_rect.left, wdiv4, hdiv4);
            RenderMeta_SetZoneK(&tmp_des_rect[8].left, &(des_rect->left), wdiv4, hdiv4);
            RenderMeta_SetZoneP(&tmp_des_rect[8].right, &(des_rect->left), wdiv4, hdiv4);
            // ��Ⱦ 
            RenderMeta_DrawBitmap(0, 0);
            RenderMeta_DrawBitmap(1, 0);
            RenderMeta_DrawBitmap(2, 0);
            RenderMeta_DrawBitmap(3, 0);
            RenderMeta_DrawBitmap(4, meta.interpolation);
            RenderMeta_DrawBitmap(5, 0);
            RenderMeta_DrawBitmap(6, 0);
            RenderMeta_DrawBitmap(7, 0);
            RenderMeta_DrawBitmap(8, 0);
            
        }
#pragma endregion
    }
    else {
        // ֱ����Ⱦ
        this->target->DrawBitmap(
            meta.bitmap,
            des_rect, opa,
            static_cast<D2D1_INTERPOLATION_MODE>(meta.interpolation),
            &meta.src_rect,
            nullptr
            );
    }
}


// ��Ⱦ UIElement
void LongUI::CUIElement::Render(D2D1_RECT_F * des_rect) noexcept{
    if (UIElement_IsMetaMode) {
        // �Ȼ��Ƶ�ǰ״̬
        if (animationo.value < animationo.end)
            this->RenderMeta(metas[old_status], des_rect, animationo.end);
        // �ٻ���Ŀ��״̬
        this->RenderMeta(metas[tar_status], des_rect, animationo.value);
    }
    else {
        brush->SetColor(&animationc.value);
        target->FillRectangle(des_rect, brush);
    }
}

// �����µ�״̬
auto LongUI::CUIElement::SetNewStatus(ControlStatus new_status) noexcept ->float{
    old_status = tar_status;
    tar_status = new_status;
    register float rc = 0.f;
    if (UIElement_IsMetaMode) {
        animationo.value = 0.f;
        rc = animationo.time = animationo.duration;
    }
    else{
        animationc.start = animationc.value;
        animationc.end = colors[new_status];
        rc = animationc.time = animationc.duration;
    }
    // ��30ms(60Hz��2֡)��֤�����������
    return rc + 0.033f;
}

// ��ʼ��״̬
void LongUI::CUIElement::InitStatus(ControlStatus s) noexcept {
    old_status = tar_status = s;
    animationc.end = animationc.start = animationc.value = colors[s];
    animationo.end = 1.f;
    animationo.start = 0.f;
    animationo.value = 0.f;
}

#define white_space(c) ((c) == ' ' || (c) == '\t')
#define valid_digit(c) ((c) >= '0' && (c) <= '9')


// �Լ�ʵ��LongUI::AtoI
auto __fastcall LongUI::AtoI(const char* str) -> int {
    if (!str) return 0;
    register bool negative = false;
    register int value = 0;
    register char ch = 0;
    while (ch = *str) {
        if (!white_space(ch)) {
            if (ch == '-') {
                negative = true;
            }
            else if (valid_digit(ch)) {
                value *= 10;
                value += ch - '0';
            }
            else {
                break;
            }
        }
        ++str;
    }
    if (negative) {
        // value *= -1;
        value = -value;
    }
    return value;
}


// �Լ�ʵ�ֵ�LongUI::AtoF
auto __fastcall LongUI::AtoF(const char* p) -> float {
    bool negative = false;
    float value, scale;
    // �����հ�
    while (white_space(*p)) ++p;
    // ������
    if (*p == '-') {
        negative = true;
        ++p;
    }
    else if (*p == '+') {
        ++p;
    }
    // ��ȡС�������ָ��֮ǰ������(�еĻ�)
    for (value = 0.0f; valid_digit(*p); ++p) {
        value = value * 10.0f + static_cast<float>(*p - '0');
    }
    // ��ȡС�������ָ��֮�������(�еĻ�)
    if (*p == '.') {
        float pow10 = 10.0f;
        ++p;
        while (valid_digit(*p)) {
            value += (*p - '0') / pow10;
            pow10 *= 10.0f;
            ++p;
        }
    }
    // ����ָ��(�еĻ�)
    bool frac = false;
    scale = 1.0f;
    if ((*p == 'e') || (*p == 'E')) {
        // ��ȡָ���ķ���(�еĻ�)
        ++p;
        if (*p == '-') {
            frac = true;
            ++p;
        }
        else if (*p == '+') {
            ++p;
        }
        unsigned int expon;
        // ��ȡָ��������(�еĻ�)
        for (expon = 0; valid_digit(*p); ++p) {
            expon = expon * 10 + (*p - '0');
        }
        // float ���38 double ���308
        if (expon > 38) expon = 38;
        // �����������
        while (expon >= 8) { scale *= 1E8f;  expon -= 8; }
        while (expon) { scale *= 10.0f; --expon; }
    }
    // ����
    register float returncoude = (frac ? (value / scale) : (value * scale));
    if (negative) {
        // float
        returncoude = -returncoude;
    }
    return returncoude;
}



// Դ: http://llvm.org/svn/llvm-project/llvm/trunk/lib/Support/ConvertUTF.c
// ���޸�

static constexpr int halfShift = 10;

static constexpr char32_t halfBase = 0x0010000UL;
static constexpr char32_t halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START      (char32_t)0xD800
#define UNI_SUR_HIGH_END        (char32_t)0xDBFF
#define UNI_SUR_LOW_START       (char32_t)0xDC00
#define UNI_SUR_LOW_END         (char32_t)0xDFFF

#define UNI_REPLACEMENT_CHAR    (char32_t)0x0000FFFD
#define UNI_MAX_BMP             (char32_t)0x0000FFFF
#define UNI_MAX_UTF16           (char32_t)0x0010FFFF
#define UNI_MAX_UTF32           (char32_t)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32     (char32_t)0x0010FFFF

#define UNI_MAX_UTF8_BYTES_PER_CODE_POINT 4

#define UNI_UTF16_BYTE_ORDER_MARK_NATIVE  0xFEFF
#define UNI_UTF16_BYTE_ORDER_MARK_SWAPPED 0xFFFE

// ת����
static constexpr char trailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/*
* Magic values subtracted from a buffer value during UTF8 conversion.
* This table contains as many values as there might be trailing bytes
* in a UTF-8 sequence.
*/
static constexpr char32_t offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
* Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
* into the first byte, depending on how many bytes follow.  There are
* as many entries in this table as there are UTF-8 sequence types.
* (I.e., one byte sequence, two byte... etc.). Remember that sequencs
* for *legal* UTF-8 will be 4 or fewer bytes total.
*/
static constexpr char firstByteMark[7] = { (char)0x00, (char)0x00, (char)0xC0, (char)0xE0, (char)0xF0,(char)0xF8, (char)0xFC };


// ����
auto __fastcall LongUI::Base64Encode(IN const uint8_t* bindata, IN size_t binlen, OUT char* const base64 ) noexcept -> char * {
    register uint8_t current;
    register auto base64_index = base64;
    // 
    for (size_t i = 0; i < binlen; i += 3) {
        current = (bindata[i] >> 2);
        current &= static_cast<uint8_t>(0x3F);
        *base64_index = Base64Chars[current]; ++base64_index;

        current = (static_cast<uint8_t>((bindata)[i] << 4)) & (static_cast<uint8_t>(0x30));
        if (i + 1 >= binlen) {
            *base64_index = Base64Chars[current]; ++base64_index;
            *base64_index = '='; ++base64_index;
            *base64_index = '='; ++base64_index;
            break;
        }
        current |= (static_cast<uint8_t>((bindata)[i + 1] >> 4)) & (static_cast<uint8_t>(0x0F));
        *base64_index = Base64Chars[current]; ++base64_index;

        current = (static_cast<uint8_t>((bindata)[i + 1] << 2)) & (static_cast<uint8_t>(0x3C));
        if (i + 2 >= binlen) {
            *base64_index = Base64Chars[current]; ++base64_index;
            *base64_index = '='; ++base64_index;
            break;
        }
        current |= (static_cast<uint8_t>((bindata)[i + 2] >> 6)) & (static_cast<uint8_t>(0x03));
        *base64_index = Base64Chars[current]; ++base64_index;

        current = (static_cast<uint8_t>(bindata[i + 2])) & (static_cast<uint8_t>(0x3F));
        *base64_index = Base64Chars[current]; ++base64_index;
    }
    *base64_index = 0;
    return base64;
}

// ����
auto __fastcall LongUI::Base64Decode(IN const char * base64, OUT uint8_t * bindata) noexcept -> size_t{
    // �����Ƴ���
    register union { uint8_t temp[4]; uint32_t temp_u32; };
    register uint8_t* bindata_index = bindata;
    // ��ѭ��
    while (*base64) {
        temp_u32 = uint32_t(-1);
        // ����ת��
        temp[0] = Base64Datas[base64[0]];  temp[1] = Base64Datas[base64[1]];
        temp[2] = Base64Datas[base64[2]];  temp[3] = Base64Datas[base64[3]];
        // ��һ������������
        *bindata_index = ((temp[0] << 2) & uint8_t(0xFC)) | ((temp[1] >> 4) & uint8_t(0x03));
        ++bindata_index;
        if (base64[2] == '=') break;
        // ����������������
        *bindata_index = ((temp[1] << 4) & uint8_t(0xF0)) | ((temp[2] >> 2) & uint8_t(0x0F));
        ++bindata_index;
        if (base64[3] == '=') break;
        // ����������������
        *bindata_index = ((temp[2] << 6) & uint8_t(0xF0)) | ((temp[2] >> 0) & uint8_t(0x3F));
        ++bindata_index;
        base64 += 4;
    }
    return bindata_index - bindata;
}

// UTF-16 to UTF-8
// Return: UTF-8 string length, 0 maybe error
auto __fastcall LongUI::UTF16toUTF8(const char16_t* pUTF16String, char* pUTF8String) noexcept->uint32_t {
    UINT32 length = 0;
    const char16_t* source = pUTF16String;
    char* target = pUTF8String;
    //char* targetEnd = pUTF8String + uBufferLength;
    // ת��
    while (*source) {
        char32_t ch;
        unsigned short bytesToWrite = 0;
        const char32_t byteMask = 0xBF;
        const char32_t byteMark = 0x80;
        const char16_t* oldSource = source; /* In case we have to back up because of target overflow. */
        ch = *source++;
        /* If we have a surrogate pair, convert to UTF32 first. */
        if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) {
            /* If the 16 bits following the high surrogate are in the source buffer... */
            if (*source) {
                char32_t ch2 = *source;
                /* If it's a low surrogate, convert to UTF32. */
                if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
                    ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
                        + (ch2 - UNI_SUR_LOW_START) + halfBase;
                    ++source;
                }
            }
            else {
                --source;
                length = 0;
                assert(!"end of string");
                break;
            }
#ifdef STRICT_CONVERSION
        else { /* it's an unpaired high surrogate */
            --source; /* return to the illegal value itself */
            result = sourceIllegal;
            break;
        }
#endif
        }
#ifdef STRICT_CONVERSION
        else {
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END) {
                --source; /* return to the illegal value itself */
                result = sourceIllegal;
                break;
            }
        }
#endif
        /* Figure out how many bytes the result will require */
        if (ch < (char32_t)0x80) {
            bytesToWrite = 1;
        }
        else if (ch < (char32_t)0x800) {
            bytesToWrite = 2;
        }
        else if (ch < (char32_t)0x10000) {
            bytesToWrite = 3;
        }
        else if (ch < (char32_t)0x110000) {
            bytesToWrite = 4;
        }
        else {
            bytesToWrite = 3;
            ch = UNI_REPLACEMENT_CHAR;
        }

        target += bytesToWrite;
        /*if (target > targetEnd) {
            source = oldSource; // Back up source pointer!
            target -= bytesToWrite;
            length = 0; break;
        }*/
        switch (bytesToWrite) { /* note: everything falls through. */
        case 4: *--target = (char)((ch | byteMark) & byteMask); ch >>= 6;
        case 3: *--target = (char)((ch | byteMark) & byteMask); ch >>= 6;
        case 2: *--target = (char)((ch | byteMark) & byteMask); ch >>= 6;
        case 1: *--target = (char)(ch | firstByteMark[bytesToWrite]);
        }
        target += bytesToWrite;
        length += bytesToWrite;
    }
    return length;
}



// UTF-8 to UTF-16
// Return: UTF-16 string length, 0 maybe error
auto __fastcall LongUI::UTF8toUTF16(const char* pUTF8String, char16_t* pUTF16String) noexcept -> uint32_t {
    UINT32 length = 0;
    auto source = reinterpret_cast<const unsigned char*>(pUTF8String);
    char16_t* target = pUTF16String;
    //char16_t* targetEnd = pUTF16String + uBufferLength;
    // ����
    while (*source) {
        char32_t ch = 0;
        unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
        /*if (extraBytesToRead >= sourceEnd - source) {
        result = sourceExhausted; break;
        }*/
        /* Do this check whether lenient or strict */
        /*if (!isLegalUTF8(source, extraBytesToRead + 1)) {
        result = sourceIllegal;
        break;
        }*/
        /*
        * The cases all fall through. See "Note A" below.
        */
        switch (extraBytesToRead) {
        case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
        case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
        case 3: ch += *source++; ch <<= 6;
        case 2: ch += *source++; ch <<= 6;
        case 1: ch += *source++; ch <<= 6;
        case 0: ch += *source++;
        }
        ch -= offsetsFromUTF8[extraBytesToRead];

        /*if (target >= targetEnd) {
            source -= (extraBytesToRead + 1); // Back up source pointer!
            length = 0; break;
        }*/
        if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
                                 /* UTF-16 surrogate values are illegal in UTF-32 */
            if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END) {
#ifdef STRICT_CONVERSION
                source -= (extraBytesToRead + 1); /* return to the illegal value itself */
                length = 0;
                break;
#else
                *target++ = UNI_REPLACEMENT_CHAR;
                ++length;
#endif
            }
            else {
                *target++ = (char16_t)ch; /* normal case */
                ++length;
            }
        }
        else if (ch > UNI_MAX_UTF16) {
#ifdef STRICT_CONVERSION
            length = 0;
            source -= (extraBytesToRead + 1); /* return to the start */
            break; /* Bail out; shouldn't continue */
#else
            *target++ = UNI_REPLACEMENT_CHAR;
            ++length;
#endif
        }
        else {
            /* target is a character in range 0xFFFF - 0x10FFFF. */
            /*if (target + 1 >= targetEnd) {
                source -= (extraBytesToRead + 1); // Back up source pointer!
                length = 0; break;
            }*/
            ch -= halfBase;
            *target++ = (char16_t)((ch >> halfShift) + UNI_SUR_HIGH_START);
            *target++ = (char16_t)((ch & halfMask) + UNI_SUR_LOW_START);
            length += 2;
        }
    }
    // �������
    return length;
}


// ----------------------------
#define UIText_NewAttribute(a) { ::strcpy(attribute_buffer, prefix); ::strcat(attribute_buffer, a); }
// UIText ���캯��
LongUI::UIText::UIText(pugi::xml_node node, const char * prefix) noexcept
    : m_pTextRenderer(nullptr){
    m_config = {
        ::SafeAcquire(UIManager_DWriteFactory),
        nullptr,
        UIManager.inline_handler,
        128.f, 64.f, 1.f, 0
    };
    // ������
    assert(node && prefix && "bad arguments");
    register union { const char* str; const uint32_t* pui32; };
    str = nullptr;
    char attribute_buffer[256];
    // ��ȡ����
    {
        UIText_NewAttribute("progress");
        if ((str = node.attribute(attribute_buffer).value())) {
            m_config.progress = LongUI::AtoF(str);
        }
    }
    // ��ȡ��Ⱦ��
    {
        int renderer_index = Type_NormalTextRenderer;
        UIText_NewAttribute("renderer");
        if ((str = node.attribute(attribute_buffer).value())) {
            renderer_index = LongUI::AtoI(str);
        }
        auto renderer = UIManager.GetTextRenderer(renderer_index);
        m_pTextRenderer = renderer;
        // ��֤������
        if (renderer) {
            auto length = renderer->GetContextSizeInByte();
            if (length) {
                UIText_NewAttribute("context");
                if ((str = node.attribute(attribute_buffer).value())) {
                    m_buffer.NewSize(length);
                    renderer->CreateContextFromString(m_buffer.data, str);
                }
            }
        }
    }
    {
        // ��������ɫ
        m_basicColor = D2D1::ColorF(D2D1::ColorF::Black);
        UIText_NewAttribute("color");
        UIControl::MakeColor(node.attribute(attribute_buffer).value(), m_basicColor);
    }
    {
        // ����ʽ
        uint32_t format_index = 0;
        UIText_NewAttribute("format");
        if ((str = node.attribute(attribute_buffer).value())) {
            format_index = static_cast<uint32_t>(LongUI::AtoI(str));
        }
        m_config.text_format = UIManager.GetTextFormat(format_index);
    }
    {
        // �������
        UIText_SetIsRich(false);
        UIText_NewAttribute("type");
        if (str = node.attribute(attribute_buffer).value()) {
            switch (*pui32)
            {
            case "xml"_longui32:
            case "XML"_longui32:
                UIText_SetIsRich(true);
                UIText_SetIsXML(true);
                break;
            case "core"_longui32:
            case "Core"_longui32:
            case "CORE"_longui32:
                UIText_SetIsRich(true);
                UIText_SetIsXML(false);
                break;
            default:
                UIText_SetIsRich(false);
                UIText_SetIsXML(false);
                break;
            }
        }
    }
    // �ؽ�
    this->recreate(node.attribute(prefix).value());
}

// UIText = L"***"
LongUI::UIText& LongUI::UIText::operator=(const wchar_t* new_string) noexcept {
    // ������XMLģʽ
    assert(UIText_GetIsXML() == false && "=(const wchar_t*) must be in core-mode, can't be xml-mode");
    m_text.Set(new_string);
    this->recreate();
    return *this;
}

// UIText = "***"
LongUI::UIText & LongUI::UIText::operator=(const char* str) noexcept {
    if (UIText_GetIsXML()) {
        this->recreate(str);
        return *this;
    }
    else {
        wchar_t buffer[LongUIStringBufferLength];
        buffer[LongUI::UTF8toWideChar(str, buffer)] = L'\0';
        return this->operator=(buffer);
    }
}


// UIText ����
LongUI::UIText::~UIText() noexcept{
    m_pTextRenderer.SafeRelease();
    ::SafeRelease(m_pLayout);
    ::SafeRelease(m_config.dw_factory);
    ::SafeRelease(m_config.text_format);
}

void LongUI::UIText::recreate(const char* utf8) noexcept{
    wchar_t text_buffer[LongUIStringBufferLength];
    // ת��Ϊ����ģʽ
    if (UIText_GetIsXML() && UIText_GetIsRich()) {
        CUIManager::XMLToCoreFormat(utf8, text_buffer);
    }
    else if(utf8){
        // ֱ��ת��
        register auto length = LongUI::UTF8toWideChar(utf8, text_buffer);
        text_buffer[length] = L'\0';
        m_text.Set(text_buffer, length);
    }
    // ��������
    ::SafeRelease(m_pLayout);
    // ���ı�
    if (UIText_GetIsRich()) {
        m_pLayout = CUIManager::FormatTextCore(
            m_config,
            m_text.c_str(),
            nullptr
            );
    }
    // ƽ̨�ı�
    else {
        register auto string_length_need = static_cast<uint32_t>(static_cast<float>(m_text.length() + 1) * 
            m_config.progress);
        LongUIClamp(string_length_need, 0, m_text.length());
        m_config.dw_factory->CreateTextLayout(
            m_text.c_str(),
            string_length_need,
            m_config.text_format,
            m_config.width,
            m_config.height,
            &m_pLayout
            );
        m_config.text_length = m_text.length();
    }
}


// UIFileLoader ��ȡ�ļ�
bool LongUIMethodCall LongUI::CUIFileLoader::ReadFile(WCHAR* file_name) noexcept {
    // ���ļ�
    FILE* file = nullptr;
    if (file = ::_wfopen(file_name, L"rb")) {
        // ��ȡ�ļ���С
        ::fseek(file, 0L, SEEK_END);
        m_cLength = ::ftell(file);
        ::fseek(file, 0L, SEEK_SET);
        // ���治��?
        if (m_cLength > m_cLengthReal) {
            m_cLengthReal = m_cLength;
            if (m_pData) free(m_pData);
            m_pData = malloc(m_cLength);
        }
        // ��ȡ�ļ�
        if (m_pData) ::fread(m_pData, 1, m_cLength, file);
    }
    // �ر��ļ�
    if (file) ::fclose(file);
    return file && m_pData;
}

// CUIDefaultConfigure::LoadBitmapByRI Impl
auto LongUI::CUIDefaultConfigure::LoadBitmapByRI(CUIManager& manager, const char* res_iden) noexcept->ID2D1Bitmap1* {
    wchar_t buffer[MAX_PATH * 4]; buffer[LongUI::UTF8toWideChar(res_iden, buffer)] = L'\0';
    ID2D1Bitmap1* bitmap = nullptr;
    CUIManager::LoadBitmapFromFile(manager, manager, buffer, 0, 0, &bitmap);
    return bitmap;
}



// --------------  CUIConsole ------------
// CUIConsole ���캯��
LongUI::CUIConsole::CUIConsole() noexcept {
    ::InitializeCriticalSection(&m_cs);
    m_name[0] = 0;
    { if (m_hConsole != INVALID_HANDLE_VALUE) this->Close(); }
}

// CUIConsole ��������
LongUI::CUIConsole::~CUIConsole() noexcept {
    this->Close();
    // �ر�
    if (m_hConsole != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_hConsole);
        m_hConsole = INVALID_HANDLE_VALUE;
    }
    ::DeleteCriticalSection(&m_cs);
}

// CUIConsole �ر�
long LongUI::CUIConsole::Close() noexcept {
    if (!(*this))
        return -1;
    else
        return ::DisconnectNamedPipe(m_hConsole);
}

// CUIConsole ���
long LongUI::CUIConsole::Output(const wchar_t * str, bool flush, long len) noexcept {
    if (len == -1) len = ::wcslen(str);
    // ���������
    if (len > LongUIStringBufferLength) {
        // ֱ�ӵݹ�
        while (len) {
            auto len_in = len > LongUIStringBufferLength ? LongUIStringBufferLength : len;
            this->Output(str, true, len_in);
            len -= len_in;
            str += len_in;
        }
        return 0;
    }
    // ����Ŀ��
    if (m_length + len > LongUIStringBufferLength) {
        flush = true;
    }
    // д��
    if (m_length + len < LongUIStringBufferLength) {
        ::memcpy(m_buffer + m_length, str, len * sizeof(wchar_t));
        m_length += len;
        str = nullptr;
        // ��flush
        if(!flush) return 0;
    }
    DWORD dwWritten = DWORD(-1);
    // ��д�뻺����
    if (m_length) {
        this->SafeWriteFile(m_hConsole, m_buffer, m_length * sizeof(wchar_t), &dwWritten, nullptr);
        m_length = 0;
    }
    // ��д��Ŀ��
    if (str) {
        len *= sizeof(wchar_t);
        return (!this->SafeWriteFile(m_hConsole, str, len, &dwWritten, nullptr)
            || (int)dwWritten != len) ? -1 : (int)dwWritten;
    }
    return 0;
}

// CUIConsole ����
long LongUI::CUIConsole::Create(const wchar_t* lpszWindowTitle, Config& config) noexcept {
    // ���δ���?
    if (m_hConsole != INVALID_HANDLE_VALUE) {
        ::DisconnectNamedPipe(m_hConsole);
        ::CloseHandle(m_hConsole);
        m_hConsole = INVALID_HANDLE_VALUE;
    }
    // �ȸ���
    ::wcsncpy(m_name, LR"(\\.\pipe\)", 9);
    wchar_t logger_name_buffer[128];
    // δ��logger?
    if (!config.logger_name)  {
        ::swprintf(
            logger_name_buffer, lengthof(logger_name_buffer),
            L"logger_%f", float(::rand()) / float(RAND_MAX)
            );
        config.logger_name = logger_name_buffer;
    }
    ::wcscat(m_name, config.logger_name);
    // �����ܵ�
    m_hConsole = ::CreateNamedPipeW(
        m_name,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        4096,   // �������
        0,      // ���뻺��
        1,
        nullptr
    );
    // ��Ч
    if (m_hConsole == INVALID_HANDLE_VALUE) {
        ::MessageBoxW(nullptr, L"CreateNamedPipe failed", L"CUIConsole::Create failed", MB_ICONERROR);
        return -1;
    }
    // ��������̨
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ::GetStartupInfoW(&si);

    const wchar_t* DEFAULT_HELPER_EXE = L"ConsoleHelper.exe";

    wchar_t cmdline[MAX_PATH];;
    if (!config.helper_executable)
        config.helper_executable = DEFAULT_HELPER_EXE;

    ::swprintf(cmdline, MAX_PATH, L"%ls %ls", config.helper_executable, config.logger_name);
    BOOL bRet = ::CreateProcessW(nullptr, cmdline, nullptr, nullptr, false, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
    if (!bRet)  {
        char *path = ::getenv("ConsoleLoggerHelper");
        if (path) {
            ::swprintf(cmdline, MAX_PATH, L"%ls %ls", path, config.logger_name);
            bRet = ::CreateProcessW(nullptr, nullptr, nullptr, nullptr, false, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
        }
        if (!bRet) {
            ::MessageBoxW(nullptr, L"Helper executable not found", L"ConsoleLogger failed", MB_ICONERROR);
            ::CloseHandle(m_hConsole);
            m_hConsole = INVALID_HANDLE_VALUE;
            return -1;
        }
    }


    BOOL bConnected = ::ConnectNamedPipe(m_hConsole, nullptr) ?
        TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    // ����ʧ��
    if (!bConnected) {
        ::MessageBoxW(nullptr, L"ConnectNamedPipe failed", L"ConsoleLogger failed", MB_ICONERROR);
        ::CloseHandle(m_hConsole);
        m_hConsole = INVALID_HANDLE_VALUE;
        return -1;
    }

    DWORD cbWritten;

    // ����

    wchar_t buffer[128];
    // ���ͱ���
    if (!lpszWindowTitle) lpszWindowTitle = m_name + 9;
    ::swprintf(buffer, lengthof(buffer), L"TITLE: %ls\r\n", lpszWindowTitle);
    auto len_in_byte = ::wcslen(buffer) * sizeof(wchar_t);
    ::WriteFile(m_hConsole, buffer, len_in_byte, &cbWritten, nullptr);
    if (cbWritten != len_in_byte) {
        ::MessageBoxW(nullptr, L"WriteFile failed(1)", L"ConsoleLogger failed", MB_ICONERROR);
        ::DisconnectNamedPipe(m_hConsole);
        ::CloseHandle(m_hConsole);
        m_hConsole = INVALID_HANDLE_VALUE;
        return -1;
    }

    // ����λ��
    if (config.position_xy != -1) {
        ::swprintf(buffer, lengthof(buffer), L"POS: %d\r\n", config.position_xy);
        len_in_byte = ::wcslen(buffer) * sizeof(wchar_t);
        ::WriteFile(m_hConsole, buffer, len_in_byte, &cbWritten, nullptr);
        if (cbWritten != len_in_byte) {
            ::MessageBoxW(nullptr, L"WriteFile failed(1.1)", L"ConsoleLogger failed", MB_ICONERROR);
            ::DisconnectNamedPipe(m_hConsole);
            ::CloseHandle(m_hConsole);
            m_hConsole = INVALID_HANDLE_VALUE;
            return -1;
        }
    }
    // ��������
    if (config.atribute) {
        ::swprintf(buffer, lengthof(buffer), L"ATTR: %d\r\n", config.atribute);
        len_in_byte = ::wcslen(buffer) * sizeof(wchar_t);
        ::WriteFile(m_hConsole, buffer, len_in_byte, &cbWritten, nullptr);
        if (cbWritten != len_in_byte) {
            ::MessageBoxW(nullptr, L"WriteFile failed(1.2)", L"ConsoleLogger failed", MB_ICONERROR);
            ::DisconnectNamedPipe(m_hConsole);
            ::CloseHandle(m_hConsole);
            m_hConsole = INVALID_HANDLE_VALUE;
            return -1;
        }
    }

    // ���ͻ�������С
    if (config.buffer_size_x != -1 && config.buffer_size_y != -1)  {
        ::swprintf(buffer, lengthof(buffer), L"BUFFER-SIZE: %dx%d\r\n", config.buffer_size_x, config.buffer_size_y);
        len_in_byte = ::wcslen(buffer) * sizeof(wchar_t);
        ::WriteFile(m_hConsole, buffer, len_in_byte, &cbWritten, nullptr);
        if (cbWritten != len_in_byte) {
            ::MessageBoxW(nullptr, L"WriteFile failed(2)", L"ConsoleLogger failed", MB_ICONERROR);
            ::DisconnectNamedPipe(m_hConsole);
            ::CloseHandle(m_hConsole);
            m_hConsole = INVALID_HANDLE_VALUE;
            return -1;
        }
    }

    // ���ͷ
    if (false)  {
        ::DisconnectNamedPipe(m_hConsole);
        ::CloseHandle(m_hConsole);
        m_hConsole = INVALID_HANDLE_VALUE;
        return -1;
    }

    // �������

    buffer[0] = 0;
    ::WriteFile(m_hConsole, buffer, 2, &cbWritten, nullptr);
    if (cbWritten != 2) {
        ::MessageBoxW(nullptr, L"WriteFile failed(3)", L"ConsoleLogger failed", MB_ICONERROR);
        ::DisconnectNamedPipe(m_hConsole);
        ::CloseHandle(m_hConsole);
        m_hConsole = INVALID_HANDLE_VALUE;
        return -1;
    }
    return 0;
}

// --------------  CUIDefaultConfigure ------------
#ifdef LONGUI_WITH_DEFAULT_CONFIG
auto LongUI::CUIDefaultConfigure::ChooseAdapter(IDXGIAdapter1 * adapters[], size_t const length) noexcept -> size_t {
    // ���Կ����� 
#ifdef LONGUI_NUCLEAR_FIRST
    for (size_t i = 0; i < length; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapters[i]->GetDesc1(&desc);
        if (!::wcsncmp(L"NVIDIA", desc.Description, 6))
            return i;
    }
#endif
    return length;
}

// CUIDefaultConfigure ��ʾ������Ϣ
auto LongUI::CUIDefaultConfigure::ShowError(const wchar_t * str_a, const wchar_t* str_b) noexcept -> void {
    assert(str_a && "bad argument!");
    if(!str_b) str_b = L"Error!";
    ::MessageBoxW(::GetForegroundWindow(), str_a, str_b, MB_ICONERROR);
}

#ifdef _DEBUG
// ��������ַ���
auto LongUI::CUIDefaultConfigure::OutputDebugStringW(
    DebugStringLevel level, const wchar_t * string, bool flush) noexcept -> void {
    auto& console = this->consoles[level];
    // ��Ч�ʹ���
    if (!console) {
        this->CreateConsole(level);
    }
    // ��Ч�����
    if (console) {
        console.Output(string, flush);
    }
}

void LongUI::CUIDefaultConfigure::CreateConsole(DebugStringLevel level) noexcept {
    CUIConsole::Config config;
    config.x = 5;
    config.y = int16_t(level) * 128;
    switch (level)
    {
    case LongUI::DLevel_None:
        break;
    case LongUI::DLevel_Log:
        break;
    case LongUI::DLevel_Hint:
        break;
    case LongUI::DLevel_Warning:
        config.atribute = FOREGROUND_RED | FOREGROUND_GREEN;
        break;
    case LongUI::DLevel_Error:
    case LongUI::DLevel_Fatal:
        config.atribute = FOREGROUND_RED;
        break;
    }
    assert(level < LongUI::DLEVEL_SIZE);
    // ����
    const wchar_t* strings[LongUI::DLEVEL_SIZE] = {
        L"None      Console", 
        L"Log       Console", 
        L"Hint      Console", 
        L"Warning   Console", 
        L"Error     Console", 
        L"Fatal     Console"
    };

    this->consoles[level].Create(strings[level], config);
}

#endif
#endif

//////////////////////////////////////////
// Video
//////////////////////////////////////////
#ifdef LONGUI_VIDEO_IN_MF
// CUIVideoComponent �¼�֪ͨ
HRESULT LongUI::CUIVideoComponent::EventNotify(DWORD event, DWORD_PTR param1, DWORD param2) noexcept {
    switch (event)
    {
    case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
        m_bEOS = false;
        break;
    case MF_MEDIA_ENGINE_EVENT_CANPLAY:
        this->Play();
        break;
    case MF_MEDIA_ENGINE_EVENT_PLAY:
        m_bPlaying = true;
        break;
    case MF_MEDIA_ENGINE_EVENT_PAUSE:
        m_bPlaying = false;
        break;
    case MF_MEDIA_ENGINE_EVENT_ENDED:
        m_bPlaying = false;
        m_bEOS = true;
        break;
    case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
        break;
    case MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE:
        ::SetEvent(reinterpret_cast<HANDLE>(param1));
        break;
    case MF_MEDIA_ENGINE_EVENT_ERROR:
    {
        auto err = MF_MEDIA_ENGINE_ERR(param1);
        auto hr = HRESULT(param2);
        int a = 9;
    }
        break;
    }
    return S_OK;
}

HRESULT LongUI::CUIVideoComponent::Init() noexcept {
    HRESULT hr = S_OK;
    IMFAttributes* attributes = nullptr;
    // ����MF����
    if (SUCCEEDED(hr)) {
        hr = ::MFCreateAttributes(&attributes, 1);
    }
    // ��������: DXGI������
    if (SUCCEEDED(hr)) {
        hr = attributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, UIManager_MFDXGIDeviceManager);
    }
    // ��������: �¼�֪ͨ
    if (SUCCEEDED(hr)) {
        hr = attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, this);
    }
    // ��������: �����ʽ
    if (SUCCEEDED(hr)) {
        hr = attributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM);
    }
    // ����ý������
    if (SUCCEEDED(hr)) {
        constexpr DWORD flags = MF_MEDIA_ENGINE_WAITFORSTABLE_STATE;
        hr = UIManager_MFMediaEngineClassFactory->CreateInstance(flags, attributes, &m_pMediaEngine);
    }
    // ��ȡEx��
    /*if (SUCCEEDED(hr)) {
        hr = m_pMediaEngine->QueryInterface(LongUI_IID_PV_ARGS(m_pEngineEx));
    }*/
    assert(SUCCEEDED(hr));
    ::SafeRelease(attributes);
    return hr;
}

// �ؽ�
HRESULT LongUI::CUIVideoComponent::Recreate(ID2D1RenderTarget* target) noexcept {
    ::SafeRelease(m_pRenderTarget);
    ::SafeRelease(m_pTargetSurface);
    ::SafeRelease(m_pDrawSurface);

    assert(target && "bad argument");
    m_pRenderTarget = ::SafeAcquire(target);
    return this->recreate_surface();
}

// ��Ⱦ
void LongUI::CUIVideoComponent::Render(D2D1_RECT_F* dst) noexcept {
    const MFARGB bkColor = { 0,0,0,0 };
    assert(m_pMediaEngine);
    // ������Ч
    if (!m_pDrawSurface) {
        this->recreate_surface();
    }
    // ������Ч
    if (m_pDrawSurface) {
        LONGLONG pts;
        if ((m_pMediaEngine->OnVideoStreamTick(&pts)) == S_OK) {
            D3D11_TEXTURE2D_DESC desc;
            m_pTargetSurface->GetDesc(&desc);
            m_pMediaEngine->TransferVideoFrame(m_pTargetSurface, nullptr, &dst_rect, &bkColor);
            m_pDrawSurface->CopyFromBitmap(nullptr, m_pSharedSurface, nullptr);
        }
        D2D1_RECT_F src = { 0.f, 0.f,  float(dst_rect.right), float(dst_rect.bottom) };
        m_pRenderTarget->DrawBitmap(m_pDrawSurface, dst, 1.f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &src
            );
    }
}


// CUIVideoComponent ���캯��
LongUI::CUIVideoComponent::CUIVideoComponent() noexcept {
    force_cast(dst_rect) = { 0 };
}

// CUIVideoComponent ��������
LongUI::CUIVideoComponent::~CUIVideoComponent() noexcept {
    if (m_pMediaEngine) {
        m_pMediaEngine->Shutdown();
    }
    ::SafeRelease(m_pMediaEngine);
    //::SafeRelease(m_pEngineEx);
    ::SafeRelease(m_pTargetSurface);
    ::SafeRelease(m_pSharedSurface);
    ::SafeRelease(m_pDrawSurface);
    ::SafeRelease(m_pRenderTarget);
}

#define MakeAsUnit(a) (((a) + (LongUITargetBitmapUnitSize-1)) / LongUITargetBitmapUnitSize * LongUITargetBitmapUnitSize)


// �ؽ�����
HRESULT LongUI::CUIVideoComponent::recreate_surface() noexcept {
    // ��Ч�����
    DWORD w, h; HRESULT hr = S_FALSE;
    if (this->HasVideo() && SUCCEEDED(hr = m_pMediaEngine->GetNativeVideoSize(&w, &h))) {
        force_cast(dst_rect.right) = w;
        force_cast(dst_rect.bottom) = h;
        // ��ȡ�淶��С
        w = MakeAsUnit(w); h = MakeAsUnit(h);
        // �����ش�С
        D2D1_SIZE_U size = m_pDrawSurface ? m_pDrawSurface->GetPixelSize() : D2D1::SizeU();
        // �ؽ�����
        if (w > size.width || h > size.height) {
            size = { w, h };
            ::SafeRelease(m_pTargetSurface);
            ::SafeRelease(m_pSharedSurface);
            ::SafeRelease(m_pDrawSurface);
            IDXGISurface* surface = nullptr;
#if 0
            D3D11_TEXTURE2D_DESC desc = {
                w, h, 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, {1, 0}, D3D11_USAGE_DEFAULT, 
                D3D11_BIND_RENDER_TARGET, 0, 0
            };
            hr = UIManager_D3DDevice->CreateTexture2D(&desc, nullptr, &m_pTargetSurface);
            // ��ȡDxgi����
            if (SUCCEEDED(hr)) {
                hr = m_pTargetSurface->QueryInterface(LongUI_IID_PV_ARGS(surface));
            }
            // ��Dxgi���洴��λͼ
            if (SUCCEEDED(hr)) {
                hr = UIManager_RenderTaget->CreateBitmapFromDxgiSurface(
                    surface, nullptr, &m_pDrawSurface
                    );
            }
#else
            // ����D2Dλͼ
            D2D1_BITMAP_PROPERTIES1 prop = {
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.f, 96.f,
                D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_TARGET, nullptr
            };
            hr = UIManager_RenderTaget->CreateBitmap(size, nullptr, size.width * 4, &prop, &m_pSharedSurface);
            // ��ȡDxgi����
            if (SUCCEEDED(hr)) {
                hr = m_pSharedSurface->GetSurface(&surface);
            }
            // ��ȡD3D11 2D����
            if (SUCCEEDED(hr)) {
                hr = surface->QueryInterface(LongUI_IID_PV_ARGS(m_pTargetSurface));
            }
            // �����̻�λͼ
            if (SUCCEEDED(hr)) {
                prop.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
                hr = UIManager_RenderTaget->CreateBitmap(size, nullptr, size.width * 4, &prop, &m_pDrawSurface);
            }
#endif
            ::SafeRelease(surface);
        }
    }
    return hr;
}

#endif



// -----------------------------



// ��
#define EZ_PI 3.1415296F
// ����֮һ��
#define EZ_PI_2 1.5707963F

// ��������
float inline __fastcall BounceEaseOut(float p) noexcept {
    if (p < 4.f / 11.f) {
        return (121.f * p * p) / 16.f;
    }
    else if (p < 8.f / 11.f) {
        return (363.f / 40.f * p * p) - (99.f / 10.f * p) + 17.f / 5.f;
    }
    else if (p < 9.f / 10.f) {
        return (4356.f / 361.f * p * p) - (35442.f / 1805.f * p) + 16061.f / 1805.f;
    }
    else {
        return (54.f / 5.f * p * p) - (513.f / 25.f * p) + 268.f / 25.f;
    }
}


// CUIAnimation ��������
float __fastcall LongUI::EasingFunction(AnimationType type, float p) noexcept {
    assert((p >= 0.f && p <= 1.f) && "bad argument");
    switch (type)
    {
    default:
        assert(!"type unknown");
        __fallthrough;
    case LongUI::AnimationType::Type_LinearInterpolation:
        // ���Բ�ֵ     f(x) = x
        return p;
    case LongUI::AnimationType::Type_QuadraticEaseIn:
        // ƽ�ν���     f(x) = x^2
        return p * p;
    case LongUI::AnimationType::Type_QuadraticEaseOut:
        // ƽ�ν���     f(x) =  -x^2 + 2x
        return -(p * (p - 2.f));
    case LongUI::AnimationType::Type_QuadraticEaseInOut:
        // ƽ�γ���
        // [0, 0.5)     f(x) = (1/2)((2x)^2)
        // [0.5, 1.f]   f(x) = -(1/2)((2x-1)*(2x-3)-1) ; 
        return p < 0.5f ? (p * p * 2.f) : ((-2.f * p * p) + (4.f * p) - 1.f);
    case LongUI::AnimationType::Type_CubicEaseIn:
        // ���ν���     f(x) = x^3;
        return p * p * p;
    case LongUI::AnimationType::Type_CubicEaseOut:
        // ���ν���     f(x) = (x - 1)^3 + 1
    {
        register float f = p - 1.f;
        return f * f * f + 1.f;
    }
    case LongUI::AnimationType::Type_CubicEaseInOut:
        // ���γ���
        // [0, 0.5)     f(x) = (1/2)((2x)^3) 
        // [0.5, 1.f]   f(x) = (1/2)((2x-2)^3 + 2) 
        if (p < 0.5f) {
            return p * p * p * 2.f;
        }
        else {
            register float f = (2.f * p) - 2.f;
            return 0.5f * f * f * f + 1.f;
        }
    case LongUI::AnimationType::Type_QuarticEaseIn:
        // �Ĵν���     f(x) = x^4
    {
        register float f = p * p;
        return f * f;
    }
    case LongUI::AnimationType::Type_QuarticEaseOut:
        // �Ĵν���     f(x) = 1 - (x - 1)^4
    {
        register float f = (p - 1.f); f *= f;
        return 1.f - f * f;
    }
    case LongUI::AnimationType::Type_QuarticEaseInOut:
        // �Ĵγ���
        // [0, 0.5)     f(x) = (1/2)((2x)^4)
        // [0.5, 1.f]   f(x) = -(1/2)((2x-2)^4 - 2)
        if (p < 0.5f) {
            register float f = p * p;
            return 8.f * f * f;
        }
        else {
            register float f = (p - 1.f); f *= f;
            return 1.f - 8.f * f * f;
        }
    case LongUI::AnimationType::Type_QuinticEaseIn:
        // ��ν���     f(x) = x^5
    {
        register float f = p * p;
        return f * f * p;
    }
    case LongUI::AnimationType::Type_QuinticEaseOut:
        // ��ν���     f(x) = (x - 1)^5 + 1
    {
        register float f = (p - 1.f);
        return f * f * f * f * f + 1.f;
    }
    case LongUI::AnimationType::Type_QuinticEaseInOut:
        // ��γ���
        // [0, 0.5)     f(x) = (1/2)((2x)^5) 
        // [0.5, 1.f]   f(x) = (1/2)((2x-2)^5 + 2)
        if (p < 0.5) {
            register float f = p * p;
            return 16.f * f * f * p;
        }
        else {
            register float f = ((2.f * p) - 2.f);
            return  f * f * f * f * f * 0.5f + 1.f;
        }
    case LongUI::AnimationType::Type_SineEaseIn:
        // ���ҽ���     
        return ::sin((p - 1.f) * EZ_PI_2) + 1.f;
    case LongUI::AnimationType::Type_SineEaseOut:
        // ���ҽ���     
        return ::sin(p * EZ_PI_2);
    case LongUI::AnimationType::Type_SineEaseInOut:
        // ���ҳ���     
        return 0.5f * (1.f - ::cos(p * EZ_PI));
    case LongUI::AnimationType::Type_CircularEaseIn:
        // ����Բ��
        return 1.f - ::sqrt(1.f - (p * p));
    case LongUI::AnimationType::Type_CircularEaseOut:
        // ����Բ��
        return ::sqrt((2.f - p) * p);
    case LongUI::AnimationType::Type_CircularEaseInOut:
        // Բ������
        if (p < 0.5f) {
            return 0.5f * (1.f - ::sqrt(1.f - 4.f * (p * p)));
        }
        else {
            return 0.5f * (::sqrt(-((2.f * p) - 3.f) * ((2.f * p) - 1.f)) + 1.f);
        }
    case LongUI::AnimationType::Type_ExponentialEaseIn:
        // ָ������     f(x) = 2^(10(x - 1))
        return (p == 0.f) ? (p) : (::pow(2.f, 10.f * (p - 1.f)));
    case LongUI::AnimationType::Type_ExponentialEaseOut:
        // ָ������     f(x) =  -2^(-10x) + 1
        return (p == 1.f) ? (p) : (1.f - ::powf(2.f, -10.f * p));
    case LongUI::AnimationType::Type_ExponentialEaseInOut:
        // ָ������
        // [0,0.5)      f(x) = (1/2)2^(10(2x - 1)) 
        // [0.5,1.f]    f(x) = -(1/2)*2^(-10(2x - 1))) + 1 
        if (p == 0.0f || p == 1.0f) return p;
        if (p < 0.5f) {
            return 0.5f * ::powf(2.f, (20.f * p) - 10.f);
        }
        else {
            return -0.5f * ::powf(2.f, (-20.f * p) + 1.f) + 1.f;
        }
    case LongUI::AnimationType::Type_ElasticEaseIn:
        // ���Խ���
        return ::sin(13.f * EZ_PI_2 * p) * ::pow(2.f, 10.f * (p - 1.f));
    case LongUI::AnimationType::Type_ElasticEaseOut:
        // ���Խ���
        return ::sin(-13.f * EZ_PI_2 * (p + 1.f)) * ::powf(2.f, -10.f * p) + 1.f;
    case LongUI::AnimationType::Type_ElasticEaseInOut:
        // ���Գ���
        if (p < 0.5f) {
            return 0.5f * ::sin(13.f * EZ_PI_2 * (2.f * p)) * ::pow(2.f, 10.f * ((2.f * p) - 1.f));
        }
        else {
            return 0.5f * (::sin(-13.f * EZ_PI_2 * ((2.f * p - 1.f) + 1.f)) * ::pow(2.f, -10.f * (2.f * p - 1.f)) + 2.f);
        }
    case LongUI::AnimationType::Type_BackEaseIn:
        // ���˽���
        return  p * p * p - p * ::sin(p * EZ_PI);
    case LongUI::AnimationType::Type_BackEaseOut:
        // ���˽���
    {
        register float f = (1.f - p);
        return 1.f - (f * f * f - f * ::sin(f * EZ_PI));
    }
    case LongUI::AnimationType::Type_BackEaseInOut:
        // ���˳���
        if (p < 0.5f) {
            register float f = 2.f * p;
            return 0.5f * (f * f * f - f * ::sin(f * EZ_PI));
        }
        else {
            register float f = (1.f - (2 * p - 1.f));
            return 0.5f * (1.f - (f * f * f - f * ::sin(f * EZ_PI))) + 0.5f;
        }
    case LongUI::AnimationType::Type_BounceEaseIn:
        // ��������
        return 1.f - ::BounceEaseOut(1.f - p);
    case LongUI::AnimationType::Type_BounceEaseOut:
        // ��������
        return ::BounceEaseOut(p);
    case LongUI::AnimationType::Type_BounceEaseInOut:
        // ��������
        if (p < 0.5f) {
            return 0.5f * (1.f - ::BounceEaseOut(1.f - (p*2.f)));
        }
        else {
            return 0.5f * ::BounceEaseOut(p * 2.f - 1.f) + 0.5f;
        }
    }
}
