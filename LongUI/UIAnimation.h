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

// longui namespace
namespace LongUI {
    // the type of aniamtion
    enum class AnimationType : uint32_t {
        Type_LinearInterpolation = 0,   // ���Բ�ֵ
        Type_QuadraticEaseIn,           // ƽ�������ֵ
        Type_QuadraticEaseOut,          // ƽ��������ֵ
        Type_QuadraticEaseInOut,        // ƽ�����뽥����ֵ
        Type_CubicEaseIn,               // ���������ֵ
        Type_CubicEaseOut,              // ����������ֵ
        Type_CubicEaseInOut,            // �������뽥����ֵ
        Type_QuarticEaseIn,             // �Ĵν����ֵ
        Type_QuarticEaseOut,            // �Ĵν�����ֵ
        Type_QuarticEaseInOut,          // �Ĵν��뽥����ֵ
        Type_QuinticEaseIn,             // ��ν����ֵ
        Type_QuinticEaseOut,            // ��ν�����ֵ
        Type_QuinticEaseInOut,          // ��ν��뽥����ֵ
        Type_SineEaseIn,                // ���ҽ����ֵ
        Type_SineEaseOut,               // ���ҽ�����ֵ
        Type_SineEaseInOut,             // ���ҽ��뽥����ֵ
        Type_CircularEaseIn,            // ����Բ����ֵ
        Type_CircularEaseOut,           // ����Բ����ֵ
        Type_CircularEaseInOut,         // Բ�����뽥����ֵ
        Type_ExponentialEaseIn,         // ָ�������ֵ
        Type_ExponentialEaseOut,        // ָ��������ֵ
        Type_ExponentialEaseInOut,      // ָ�����뽥����ֵ
        Type_ElasticEaseIn,             // ���Խ����ֵ
        Type_ElasticEaseOut,            // ���Խ�����ֵ
        Type_ElasticEaseInOut,          // ���Խ��뽥����ֵ
        Type_BackEaseIn,                // ���˽����ֵ
        Type_BackEaseOut,               // ���˽�����ֵ
        Type_BackEaseInOut,             // ���˽���������ֵ
        Type_BounceEaseIn,              // ���������ֵ
        Type_BounceEaseOut,             // ����������ֵ
        Type_BounceEaseInOut,           // �������뽥����ֵ
    };
    // easing func
    float __fastcall EasingFunction(AnimationType, float) noexcept;
    // UI Animation
    template<typename T>
    class CUIAnimation {
    public:
        // constructor
        CUIAnimation(AnimationType t) noexcept :type(t)  {};
        // destructor
        ~CUIAnimation() noexcept {}
        // update
        void __fastcall Updata(float t) noexcept {
            if (this->time <= 0.f) {
                this->value = this->end;
                return;
            }
            //
            this->value = LongUI::EasingFunction(this->type, this->time / this->duration)
                * (this->start - this->end) + this->end;
            //
            this->time -= t;
        }
    public:
        // the type
        AnimationType       type;
        // time index
        float               time = 0.0f;
        // duration time
        float               duration = 0.12f;
        // start
        T                   start ;
        // end
        T                   end ;
        // value
        T                   value;
    };

#define UIAnimation_Template_A      \
    register auto v = LongUI::EasingFunction(this->type, this->time / this->duration)
#define UIAnimation_Template_B(m)   \
    this->value.m = v * (this->start.m - this->end.m) + this->end.m;

    // for D2D1_COLOR_F or Float4
    template<> static
    void LongUI::CUIAnimation<D2D1_COLOR_F>::Updata(float t) noexcept {
        if (this->time <= 0.f) {
            this->value = this->end;
            return;
        }
        UIAnimation_Template_A;
        UIAnimation_Template_B(r);
        UIAnimation_Template_B(g);
        UIAnimation_Template_B(b);
        UIAnimation_Template_B(a);
        //
        this->time -= t;
    }

    // for D2D1_POINT_2F or Float2
    template<> static
        void LongUI::CUIAnimation<D2D1_POINT_2F>::Updata(float t) noexcept {
        if (this->time <= 0.f) {
            this->value = this->end;
            return;
        }
        UIAnimation_Template_A;
        UIAnimation_Template_B(x);
        UIAnimation_Template_B(y);
        //
        this->time -= t;
    }

    // for D2D1_MATRIX_3X2_F or Float6
    template<> static
        void LongUI::CUIAnimation<D2D1_MATRIX_3X2_F>::Updata(float t) noexcept {
        if (this->time <= 0.f) {
            this->value = this->end;
            return;
        }
        UIAnimation_Template_A;
        UIAnimation_Template_B(_11);
        UIAnimation_Template_B(_12);
        UIAnimation_Template_B(_21);
        UIAnimation_Template_B(_22);
        UIAnimation_Template_B(_31);
        UIAnimation_Template_B(_32);
        //
        this->time -= t;
    }
}