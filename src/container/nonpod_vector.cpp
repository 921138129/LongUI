#include <container/nonpod_vector.h>
#include <core/ui_malloc.h>

#include <cstdlib>
#include <cassert>
#include <algorithm>

#ifndef NDEBUG
#include <debugger/ui_debug.h>
#endif

#ifdef LUI_NONPOD_VECTOR
using namespace LongUI::NonPOD;

/// <summary>
/// Checks the aligned.
/// </summary>
/// <param name="ptr">The PTR.</param>
/// <returns></returns>
inline void detail::vector_base::check_aligned(const char* ptr) noexcept {
    assert(reinterpret_cast<uintptr_t>(ptr) % m_uAligned == 0);
}

/// <summary>
/// Frees the specified .
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
inline void detail::vector_base::free(char* p) noexcept {
    return LongUI::NormalFree(p);
}

/// <summary>
/// Mallocs the specified l.
/// </summary>
/// <param name="l">The l.</param>
/// <returns></returns>
inline char* detail::vector_base::malloc(size_t l) noexcept {
    return reinterpret_cast<char*>(LongUI::NormalAlloc(l));
}

/// <summary>
/// Reallocs the specified p.
/// </summary>
/// <param name="p">The p.</param>
/// <param name="l">The l.</param>
/// <returns></returns>
inline char* detail::vector_base::realloc(char* p, size_t l) noexcept {
    return reinterpret_cast<char*>(LongUI::NormalRealloc(p, l));
}

/// <summary>
/// Tries the realloc.
/// </summary>
/// <param name="p">The p.</param>
/// <param name="l">The l.</param>
/// <returns></returns>
inline char* detail::vector_base::try_realloc(char* p, size_t l) noexcept {
    // TODO: ʵ��try_realloc
    return reinterpret_cast<char*>(LongUI::NormalAlloc(l)); p;
}

PCN_NOINLINE
/// <summary>
/// Vectors the base.
/// </summary>
/// <param name="vt">The vt.</param>
/// <param name="len">The length.</param>
/// <param name="ex">The ex.</param>
/// <returns></returns>
detail::vector_base::vector_base(
    const vtable_helper* vt, uint16_t len, uint16_t ex) noexcept
    : m_pVTable(vt), m_uByteLen(len), m_uAligned(ex) {

}

PCN_NOINLINE
/// <summary>
/// Frees the data.
/// </summary>
/// <returns></returns>
void detail::vector_base::free_data() noexcept {
    // ��Ч����
    if (m_pData) {
        // �ͷŶ���
        this->release_objs(m_pData, m_uVecLen);
        // �ͷ��ڴ�
        this->free(m_pData);
    }
}

PCN_NOINLINE
/// <summary>
/// Vectors the base.
/// </summary>
/// <param name="x">The x.</param>
/// <returns></returns>
detail::vector_base::vector_base(vector_base && x) noexcept :
m_pVTable(x.m_pVTable),
m_pData(x.m_pData),
m_uVecLen(x.m_uVecLen),
m_uVecCap(x.m_uVecCap),
m_uByteLen(x.m_uByteLen),
m_uAligned(x.m_uAligned)
{
    x.m_pData = nullptr;
    x.m_uVecLen = 0;
    x.m_uVecCap = 0;
}

PCN_NOINLINE
/// <summary>
/// Vectors the base.
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
detail::vector_base::vector_base(const vector_base& x) noexcept :
m_pVTable(x.m_pVTable),
m_uByteLen(x.m_uByteLen),
m_uAligned(x.m_uAligned)
{
    // û�ж������踴��
    if (!x.m_uVecLen) return;
    // ���ƶ���
    const auto alllen = x.m_uByteLen * x.m_uVecCap;
    if (const auto ptr = this->malloc(alllen)) {
        // ������
        this->check_aligned(ptr);
        // д������
        m_pData = ptr;
        m_uVecCap = x.m_uVecCap;
        m_uVecLen = x.m_uVecLen;
        // ���ƶ���
        this->copy_objects(m_pData, x.m_pData, x.m_uVecLen);
    }
}


PCN_NOINLINE
/// <summary>
/// Ops the equal.
/// </summary>
/// <param name="x">The x.</param>
/// <returns></returns>
void detail::vector_base::op_equal(const vector_base& x) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    assert(m_uByteLen == x.m_uByteLen && "m_uByteLen must be same");
    // ���ȹ���?
    if (capacity() >= x.size()) {
        // �ͷŶ���
        this->release_objs(m_pData, m_uVecLen);
        // ���ƶ���
        this->copy_objects(m_pData, x.m_pData, x.m_uVecLen);
        // �޸ĳ���
        m_uVecLen = x.size();
        assert(!"NOT IMPL");
    }
    // ���¹���
    else {
        // �ͷ�
        this->free_data();
        ctor_dtor<vector_base>::create(this, x);
    }
}

PCN_NOINLINE
/// <summary>
/// Emplaces the back.
/// </summary>
/// <returns></returns>
char*detail::vector_base::emplace_back() noexcept {
    void* const func = m_pVTable->create_obj;
    this->push_back_help(nullptr, func);
    return is_ok() ? last_n() : nullptr;
}

PCN_NOINLINE
/// <summary>
/// Pushes the back copy.
/// </summary>
/// <param name="obj">The object.</param>
/// <returns></returns>
void detail::vector_base::push_back_copy(const char* obj) noexcept {
    void* const func = m_pVTable->copy_t_obj;
    this->push_back_help(const_cast<char*>(obj), func);
}

PCN_NOINLINE
/// <summary>
/// Pushes the back move.
/// </summary>
/// <param name="obj">The object.</param>
/// <returns></returns>
void detail::vector_base::push_back_move(char* obj) noexcept {
#ifndef NDEBUG
    LUIDebug(Warning) << "[Recommended Use] emplace back" << endl;
#endif
    void* const func = m_pVTable->move_t_obj;
    this->push_back_help(obj, func);
}

PCN_NOINLINE
/// <summary>
/// Pushes the back help.
/// </summary>
/// <param name="obj">The object.</param>
/// <param name="call">The call.</param>
/// <returns></returns>
void detail::vector_base::push_back_help(char* obj, void* func) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    assert(m_uVecLen <= m_uVecCap && "bad case");
    // ��������ռ�
    if (m_uVecLen == m_uVecCap) {
        // �������: ÿ�μ�һ��
        const uint32_t newlen = m_uVecLen + m_uVecLen / 2;
        // �������: 4��
        this->reserve(std::max(newlen, static_cast<uint32_t>(4)));
        //this->reserve(newlen);
        if (!is_ok()) return;
    }
    // ����/�ƶ�����
    const auto ptr = m_pData + m_uVecLen * m_uByteLen;
    // ����ת��
    union { 
        void* func_ptr; 
        void(*call)(char*, char*) noexcept;
        void(*call0)(char*) noexcept;
    };
    func_ptr = func;
    // ���ö�Ӧ���캯��
    obj ? call(ptr, obj) : call0(ptr);
    // +1s
    m_uVecLen++;
}

PCN_NOINLINE
/// <summary>
/// Pops the back.
/// </summary>
/// <returns></returns>
void detail::vector_base::pop_back() noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    // û�ж���?!
    assert(m_uVecLen && "no objects");
    const auto len = --m_uVecLen;
    const auto ptr = m_pData + len * m_uByteLen;
    m_pVTable->delete_obj(ptr);
}

PCN_NOINLINE
/// <summary>
/// Reserves the specified length.
/// </summary>
/// <param name="cap">The cap.</param>
/// <param name="offset">The offset.</param>
/// <returns></returns>
void detail::vector_base::reserve(uint32_t cap) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    // ����: �������������
    if (cap <= m_uVecCap) return;
    // ʹ��realloc����
    const auto old = m_pData;
    const auto len = m_uVecLen;
    const auto alllen = cap * m_uByteLen;
    const auto ptr = this->try_realloc(old, alllen);
    // ������
    this->check_aligned(ptr);
    // ����ʧ�� �ͷ�������
    if (!ptr) {
        m_pData = 0;
        m_uVecLen = 0;
        m_uVecCap = 0;
        goto release_old_data;
    }
    // ����ɹ�
    m_uVecCap = cap;
    // ��ͬ��ַ��Ҫ�ƶ�����
    if (ptr != old) {
        // �ƶ�ԭ��������������
        this->move_objects((m_pData = ptr), old, len);
        // �ͷž�����
    release_old_data:
        // �ͷ��϶���
        this->release_objs(old, len);
        // �ͷ��ڴ�
        this->free(old);
    }
}

PCN_NOINLINE
/// <summary>
/// Does the objects.
/// </summary>
/// <param name="obj">The object.</param>
/// <param name="func">The function.</param>
/// <param name="count">The count.</param>
/// <param name="bytelen">The bytelen.</param>
/// <returns></returns>
void detail::vector_base::do_objects(char* obj, void* func, uint32_t count, uint32_t bytelen) noexcept {
    // ����ת��
    union { void* ptr; void(*call)(char*) noexcept; }; ptr = func;
    // ÿ������
    for (decltype(m_uVecLen) i = 0; i != count; ++i) {
        call(obj + bytelen * i);
    }
}


PCN_NOINLINE
/// <summary>
/// Does the objobj.
/// </summary>
/// <param name="obj">The object.</param>
/// <param name="obj2">The obj2.</param>
/// <param name="func">The function.</param>
/// <param name="count">The count.</param>
/// <param name="bytelen">The bytelen.</param>
/// <returns></returns>
void detail::vector_base::do_objobj(
    char * obj, const char* obj2, void * func, 
    uint32_t count, uint32_t bytelen) noexcept {
    // ����ת��
    union { void* ptr; void(*call)(char*, const char*) noexcept; }; ptr = func;
    // ÿ������
    for (decltype(m_uVecLen) i = 0; i != count; ++i) {
        call(obj + bytelen * i, obj2 + bytelen * i);
    }
}

/// <summary>
/// Releases the objs.
/// </summary>
/// <param name="">The .</param>
/// <param name="count">The count.</param>
/// <returns></returns>
void detail::vector_base::release_objs(char* ptr, uint32_t count) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    this->do_objects(ptr, m_pVTable->delete_obj, count, m_uByteLen);
}

/// <summary>
/// Creates the objs.
/// </summary>
/// <param name="ptr">The PTR.</param>
/// <param name="count">The count.</param>
/// <returns></returns>
void detail::vector_base::create_objs(char* ptr, uint32_t count) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    this->do_objects(ptr, m_pVTable->create_obj, count, m_uByteLen);
}

/// <summary>
/// Moves the objects.
/// </summary>
/// <param name="to">To.</param>
/// <param name="from">From.</param>
/// <param name="count">The count.</param>
/// <returns></returns>
void detail::vector_base::move_objects(char* to, char* from, uint32_t count) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    assert(to != from && "cannot move self to self");
    this->do_objobj(to, from, m_pVTable->move_t_obj, count, m_uByteLen);
}

/// <summary>
/// Copies the objects.
/// </summary>
/// <param name="to">To.</param>
/// <param name="from">From.</param>
/// <param name="count">The count.</param>
/// <returns></returns>
void detail::vector_base::copy_objects(char* to, const char* from, uint32_t count) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    assert(to != from && "cannot copy self to self");
    this->do_objobj(to, from, m_pVTable->copy_t_obj, count, m_uByteLen);
}

PCN_NOINLINE
/// <summary>
/// Shrinks to fit.
/// </summary>
/// <returns></returns>
void detail::vector_base::shrink_to_fit() noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    // �ͷŶ����ڴ�
    if (m_uVecCap > m_uVecLen) {
        // �����ڴ治����ʧ��?
        const auto len = m_uVecLen * m_uByteLen;
        const auto ptr = this->realloc(m_pData, len);
        assert(ptr == m_pData);
    }
}

PCN_NOINLINE
/// <summary>
/// Erases the specified position.
/// </summary>
/// <param name="pos">The position.</param>
/// <param name="len">The length.</param>
/// <returns></returns>
void detail::vector_base::erase(uint32_t pos, uint32_t len) noexcept {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    assert(pos < m_uVecLen && (pos + len) <= m_uVecLen && "out of range");
    // λ�ò��ڷ�����
    if (pos >= m_uVecLen) return;
    // �������Χ
    const auto end_pos = std::min(pos + len, m_uVecLen);
    const auto reallen = end_pos - pos;
    const auto start = m_pData + pos * m_uByteLen;
    // �ͷŶ���
    this->release_objs(start, reallen);
    // ɾ���м��?
    if (end_pos != m_uVecLen) {
        const auto move_from = start + reallen * m_uByteLen;
        const auto move_count = m_uVecLen - end_pos;
        // ��������ƶ�����
        this->move_objects(start, move_from, move_count);
        // ɾ�������
        this->release_objs(move_from, move_count);
    }
    // ��������
    m_uVecLen -= reallen;
}


/// <summary>
/// Ends the n.
/// </summary>
/// <returns></returns>
auto detail::vector_base::end_c() const noexcept -> const char * {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    return m_pData + m_uByteLen * m_uVecLen;
}

/// <summary>
/// Lasts the c.
/// </summary>
/// <returns></returns>
auto detail::vector_base::last_c() const noexcept -> const char * {
    assert(m_uByteLen && "m_uByteLen cannot be 0");
    assert(m_uVecLen && "no object");
    return m_pData + m_uByteLen * (m_uVecLen - 1);
}

PCN_NOINLINE
/// <summary>
/// Resizes the specified length.
/// </summary>
/// <param name="len">The length.</param>
/// <returns></returns>
void detail::vector_base::resize(uint32_t len) noexcept {
    const auto this_len = m_uVecLen;
    // ����
    if (len > this_len) {
        // ��֤�ռ�
        this->reserve(len);
        // �ڴ治��
        if (!this->is_ok()) return;
        // ��������
        this->create_objs(m_pData + m_uByteLen * this_len, len - this_len);
    }
    // ��С
    else {
        this->release_objs(m_pData + len * m_uByteLen, this_len - len);
    }
    // �޸ĳ���
    m_uVecLen = len;
}


PCN_NOINLINE
/// <summary>
/// Clears this instance.
/// </summary>
/// <returns></returns>
void detail::vector_base::clear() noexcept {
    // ��Ч����
    if (m_pData) {
        // �ͷŶ���
        this->release_objs(m_pData, m_uVecLen);
        // ��0
        m_uVecLen = 0;
    }
}


PCN_NOINLINE
/// <summary>
/// Assigns the range.
/// </summary>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
/// <returns></returns>
void detail::vector_base::assign_range(const char* data, uint32_t len) noexcept {
    // ���
    this->clear();
    // ��鳤��
    this->reserve(len);
    // ��Ч
    if (this->is_ok()) {
        // ���ƶ���
        this->copy_objects(m_pData, data, len);
    }
}

PCN_NOINLINE
/// <summary>
/// Assigns the count.
/// </summary>
/// <param name="data">The data.</param>
/// <param name="count">The count.</param>
/// <returns></returns>
void detail::vector_base::assign_count(const char* data, uint32_t count) noexcept {
    // ���
    this->clear();
    // ��鳤��
    this->reserve(count);
    // ��Ч
    if (this->is_ok()) {
        const auto ptr0 = m_pData;
        const auto table = m_pVTable;
        // ���ƶ���
        for (uint32_t i = 0; i != count; ++i) {
            table->copy_t_obj(ptr0 + m_uByteLen * i, data);
        }
    }
}


//PCN_NOINLINE
///// <summary>
///// Emplaces the objs.
///// </summary>
///// <param name="pos">The position.</param>
///// <param name="len">The length.</param>
///// <returns></returns>
//char*detail::vector_base::emplace_objs(uint32_t pos, uint32_t len) noexcept {
//    void* const func = m_pVTable->create_obj;
//    this->insert_help(nullptr, pos, len, func);
//    return is_ok() ? m_pData + m_uByteLen * pos : nullptr;
//}

//PCN_NOINLINE
///// <summary>
///// Inserts the help.
///// </summary>
///// <param name="obj">The object.</param>
///// <param name="pos">The position.</param>
///// <param name="n">The n.</param>
///// <param name="func">The function.</param>
///// <returns></returns>
//void detail::vector_base::insert_help(char* obj, uint32_t pos, uint32_t n, void* func) noexcept {
//    assert(m_uByteLen && "m_uByteLen cannot be 0");
//    assert(n && "cannot insert 0 element");
//    // move ����ֻ��һλ
//#ifndef NDEBUG
//    if (func == m_pVTable->move_t_obj) {
//        assert(n == 1 && "move only one");
//    }
//#endif
//    // ����λ��
//    const auto this_len = m_uVecLen;
//    // Խ�����
//    assert(pos <= this_len && "out of range");
//    // XXX: ��������ᵼ�¶����ƶ�����?!
//
//    // ���Ȳ�������������
//    const uint32_t remain = m_uVecCap - m_uVecLen;
//    if (remain < n) this->reserve(size() + n);
//    // ������Ч
//    if (!is_ok()) return;
//    // �ƶ�����λ�ú�Ķ���
//
//    // ����ת��
//    union {
//        void* func_ptr;
//        void(*call)(char*, char*) noexcept;
//        void(*call0)(char*) noexcept;
//    };
//    func_ptr = func;
//    // ���ڲ�������
//    if (obj) {
//        // ȫ�����
//        const auto ptr = m_pData + m_uByteLen * pos;
//        for (uint32_t i = 0; i != n; ++i) {
//            call(ptr + i * m_uByteLen, obj);
//        }
//    }
//    else {
//        // ȫ�����
//        const auto ptr = m_pData + m_uByteLen * pos;
//        for (uint32_t i = 0; i != n; ++i) {
//            call0(ptr + i * m_uByteLen);
//        }
//    }
//    // +ns
//    m_uVecLen += n;
//}
#endif