/*************************************************************************
 *
 * Copyright 2016 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#include <realm/array_string.hpp>
#include <realm/impl/array_writer.hpp>
#include <realm/spec.hpp>
#include <realm/mixed.hpp>

using namespace realm;

ArrayString::ArrayString(Allocator& a)
    : m_alloc(a)
{
    m_arr = new (&m_storage) ArrayStringShort(a, true);
}

void ArrayString::create()
{
    static_cast<ArrayStringShort*>(m_arr)->create();
}

void ArrayString::init_from_mem(MemRef mem) noexcept
{
    char* header = mem.get_addr();

    ArrayParent* parent = m_arr->get_parent();
    size_t ndx_in_parent = m_arr->get_ndx_in_parent();

    bool long_strings = Array::get_hasrefs_from_header(header);
    if (!long_strings) {
        // Small strings
        bool is_small = Array::get_wtype_from_header(header) == Array::wtype_Multiply;
        if (is_small) {
            auto arr = new (&m_storage) ArrayStringShort(m_alloc, m_nullable);
            arr->init_from_mem(mem);
            m_type = Type::small_strings;
        }
        else {
            auto arr = new (&m_storage) Array(m_alloc);
            arr->init_from_mem(mem);
            // The context flag is used to indicate interned strings vs old enum strings
            // (in conjunction with has_refs() == false)
            if (arr->get_context_flag_from_header(arr->get_header())) {
                // init for new interned strings (replacing old enum strings)
                m_type = Type::interned_strings;
                // consider if we want this invariant: REALM_ASSERT_DEBUG(m_string_interner);
            }
            else {
                // init for old enum strings
                m_string_enum_values = std::make_unique<ArrayString>(m_alloc);
                ArrayParent* p;
                REALM_ASSERT(m_spec != nullptr);
                REALM_ASSERT(m_col_ndx != realm::npos);
                ref_type r = m_spec->get_enumkeys_ref(m_col_ndx, p);
                m_string_enum_values->init_from_ref(r);
                m_string_enum_values->set_parent(p, m_col_ndx);
                m_type = Type::enum_strings;
            }
        }
    }
    else {
        bool is_big = Array::get_context_flag_from_header(header);
        if (!is_big) {
            auto arr = new (&m_storage) ArraySmallBlobs(m_alloc);
            arr->init_from_mem(mem);
            m_type = Type::medium_strings;
        }
        else {
            auto arr = new (&m_storage) ArrayBigBlobs(m_alloc, m_nullable);
            arr->init_from_mem(mem);
            m_type = Type::big_strings;
        }
    }
    m_arr->set_parent(parent, ndx_in_parent);
}

void ArrayString::init_from_parent()
{
    ref_type ref = m_arr->get_ref_from_parent();
    init_from_ref(ref);
}

void ArrayString::destroy() noexcept
{
    if (m_arr->is_attached()) {
        Array::destroy_deep(m_arr->get_ref(), m_alloc);
        detach();
    }
}

void ArrayString::detach() noexcept
{
    m_arr->detach();
    // Make sure the object is in a state like right after construction
    // Next call must be to create()
    m_arr = new (&m_storage) ArrayStringShort(m_alloc, true);
    m_type = Type::small_strings;
}

size_t ArrayString::size() const
{
    switch (m_type) {
        case Type::small_strings:
            return static_cast<ArrayStringShort*>(m_arr)->size();
        case Type::medium_strings:
            return static_cast<ArraySmallBlobs*>(m_arr)->size();
        case Type::big_strings:
            return static_cast<ArrayBigBlobs*>(m_arr)->size();
        case Type::enum_strings:
        case Type::interned_strings:
            return static_cast<Array*>(m_arr)->size();
    }
    return {};
}

void ArrayString::add(StringData value)
{
    switch (upgrade_leaf(value.size())) {
        case Type::small_strings:
            static_cast<ArrayStringShort*>(m_arr)->add(value);
            break;
        case Type::medium_strings:
            static_cast<ArraySmallBlobs*>(m_arr)->add_string(value);
            break;
        case Type::big_strings:
            static_cast<ArrayBigBlobs*>(m_arr)->add_string(value);
            break;
        case Type::enum_strings:
        case Type::interned_strings: {
            auto a = static_cast<Array*>(m_arr);
            size_t ndx = a->size();
            a->add(0);
            set(ndx, value);
            break;
        }
    }
}

void ArrayString::set(size_t ndx, StringData value)
{
    switch (upgrade_leaf(value.size())) {
        case Type::small_strings:
            static_cast<ArrayStringShort*>(m_arr)->set(ndx, value);
            break;
        case Type::medium_strings:
            static_cast<ArraySmallBlobs*>(m_arr)->set_string(ndx, value);
            break;
        case Type::big_strings:
            static_cast<ArrayBigBlobs*>(m_arr)->set_string(ndx, value);
            break;
        case Type::interned_strings: {
            auto id = m_string_interner->intern(value);
            static_cast<Array*>(m_arr)->set(ndx, id);
            break;
        }
        case Type::enum_strings: {
            size_t sz = m_string_enum_values->size();
            size_t res = m_string_enum_values->find_first(value, 0, sz);
            if (res == realm::not_found) {
                m_string_enum_values->add(value);
                res = sz;
            }
            static_cast<Array*>(m_arr)->set(ndx, res);
            break;
        }
    }
}

void ArrayString::insert(size_t ndx, StringData value)
{
    switch (upgrade_leaf(value.size())) {
        case Type::small_strings:
            static_cast<ArrayStringShort*>(m_arr)->insert(ndx, value);
            break;
        case Type::medium_strings:
            static_cast<ArraySmallBlobs*>(m_arr)->insert_string(ndx, value);
            break;
        case Type::big_strings:
            static_cast<ArrayBigBlobs*>(m_arr)->insert_string(ndx, value);
            break;
        case Type::enum_strings: {
            static_cast<Array*>(m_arr)->insert(ndx, 0);
            set(ndx, value);
            break;
        }
        case Type::interned_strings: {
            static_cast<Array*>(m_arr)->insert(ndx, 0);
            set(ndx, value);
            break;
        }
    }
}

StringData ArrayString::get(size_t ndx) const
{
    switch (m_type) {
        case Type::small_strings:
            return static_cast<ArrayStringShort*>(m_arr)->get(ndx);
        case Type::medium_strings:
            return static_cast<ArraySmallBlobs*>(m_arr)->get_string(ndx);
        case Type::big_strings:
            return static_cast<ArrayBigBlobs*>(m_arr)->get_string(ndx);
        case Type::enum_strings: {
            size_t index = size_t(static_cast<Array*>(m_arr)->get(ndx));
            return m_string_enum_values->get(index);
        }
        case Type::interned_strings: {
            size_t id = size_t(static_cast<Array*>(m_arr)->get(ndx));
            return m_string_interner->get(id);
        }
    }
    return {};
}

StringData ArrayString::get_legacy(size_t ndx) const
{
    switch (m_type) {
        case Type::small_strings:
            return static_cast<ArrayStringShort*>(m_arr)->get(ndx);
        case Type::medium_strings:
            return static_cast<ArraySmallBlobs*>(m_arr)->get_string_legacy(ndx);
        case Type::big_strings:
            return static_cast<ArrayBigBlobs*>(m_arr)->get_string(ndx);
        case Type::enum_strings: {
            size_t index = size_t(static_cast<Array*>(m_arr)->get(ndx));
            return m_string_enum_values->get(index);
        }
        case Type::interned_strings: {
            size_t id = size_t(static_cast<Array*>(m_arr)->get(ndx));
            return m_string_interner->get(id);
        }
    }
    return {};
}

Mixed ArrayString::get_any(size_t ndx) const
{
    return Mixed(get(ndx));
}

bool ArrayString::is_null(size_t ndx) const
{
    switch (m_type) {
        case Type::small_strings:
            return static_cast<ArrayStringShort*>(m_arr)->is_null(ndx);
        case Type::medium_strings:
            return static_cast<ArraySmallBlobs*>(m_arr)->is_null(ndx);
        case Type::big_strings:
            return static_cast<ArrayBigBlobs*>(m_arr)->is_null(ndx);
        case Type::enum_strings: {
            size_t id = size_t(static_cast<Array*>(m_arr)->get(ndx));
            return m_string_enum_values->is_null(id);
        }
        case Type::interned_strings: {
            size_t id = size_t(static_cast<Array*>(m_arr)->get(ndx));
            return id == 0;
        }
    }
    return {};
}

void ArrayString::erase(size_t ndx)
{
    switch (m_type) {
        case Type::small_strings:
            static_cast<ArrayStringShort*>(m_arr)->erase(ndx);
            break;
        case Type::medium_strings:
            static_cast<ArraySmallBlobs*>(m_arr)->erase(ndx);
            break;
        case Type::big_strings:
            static_cast<ArrayBigBlobs*>(m_arr)->erase(ndx);
            break;
        case Type::interned_strings:
        case Type::enum_strings:
            static_cast<Array*>(m_arr)->erase(ndx);
            break;
    }
}

void ArrayString::move(ArrayString& dst, size_t ndx)
{
    size_t sz = size();
    for (size_t i = ndx; i < sz; i++) {
        dst.add(get(i));
    }

    switch (m_type) {
        case Type::small_strings:
            static_cast<ArrayStringShort*>(m_arr)->truncate(ndx);
            break;
        case Type::medium_strings:
            static_cast<ArraySmallBlobs*>(m_arr)->truncate(ndx);
            break;
        case Type::big_strings:
            static_cast<ArrayBigBlobs*>(m_arr)->truncate(ndx);
            break;
        case Type::enum_strings:
            // this operation will never be called for enumerated columns
            REALM_UNREACHABLE();
            break;
        case Type::interned_strings:
            // unimplemented
            REALM_ASSERT(false);
            break;
    }
}

void ArrayString::clear()
{
    switch (m_type) {
        case Type::small_strings:
            static_cast<ArrayStringShort*>(m_arr)->clear();
            break;
        case Type::medium_strings:
            static_cast<ArraySmallBlobs*>(m_arr)->clear();
            break;
        case Type::big_strings:
            static_cast<ArrayBigBlobs*>(m_arr)->clear();
            break;
        case Type::enum_strings:
        case Type::interned_strings:
            static_cast<Array*>(m_arr)->clear();
            break;
    }
}

size_t ArrayString::find_first(StringData value, size_t begin, size_t end) const noexcept
{
    switch (m_type) {
        case Type::small_strings:
            return static_cast<ArrayStringShort*>(m_arr)->find_first(value, begin, end);
        case Type::medium_strings: {
            BinaryData as_binary(value.data(), value.size());
            return static_cast<ArraySmallBlobs*>(m_arr)->find_first(as_binary, true, begin, end);
            break;
        }
        case Type::big_strings: {
            BinaryData as_binary(value.data(), value.size());
            return static_cast<ArrayBigBlobs*>(m_arr)->find_first(as_binary, true, begin, end);
            break;
        }
        case Type::enum_strings: {
            size_t sz = m_string_enum_values->size();
            size_t res = m_string_enum_values->find_first(value, 0, sz);
            if (res != realm::not_found) {
                return static_cast<Array*>(m_arr)->find_first(res, begin, end);
            }
            break;
        }
        case Type::interned_strings: {
            // we need a way to avoid this lookup for each leaf array. The lookup must appear
            // higher up the call stack and passed down.
            auto id = m_string_interner->lookup(value);
            if (id) {
                return static_cast<Array*>(m_arr)->find_first(*id, begin, end);
            }
            break;
        }
    }
    return not_found;
}

namespace {

template <class T>
inline StringData get_string(const T* arr, size_t ndx)
{
    return arr->get_string(ndx);
}

template <>
inline StringData get_string(const ArrayStringShort* arr, size_t ndx)
{
    return arr->get(ndx);
}

template <class T, class U>
size_t lower_bound_string(const T* arr, U value)
{
    size_t i = 0;
    size_t sz = arr->size();
    while (0 < sz) {
        size_t half = sz / 2;
        size_t mid = i + half;
        auto probe = get_string(arr, mid);
        if (probe < value) {
            i = mid + 1;
            sz -= half + 1;
        }
        else {
            sz = half;
        }
    }
    return i;
}
} // namespace

size_t ArrayString::lower_bound(StringData value)
{
    switch (m_type) {
        case Type::small_strings:
            return lower_bound_string(static_cast<ArrayStringShort*>(m_arr), value);
        case Type::medium_strings:
            return lower_bound_string(static_cast<ArraySmallBlobs*>(m_arr), value);
        case Type::big_strings:
            return lower_bound_string(static_cast<ArrayBigBlobs*>(m_arr), value);
        case Type::enum_strings:
            break;
        case Type::interned_strings:
            REALM_UNREACHABLE();
            break;
    }
    return realm::npos;
}

ArrayString::Type ArrayString::upgrade_leaf(size_t value_size)
{
    if (m_type == Type::big_strings)
        return Type::big_strings;

    if (m_type == Type::enum_strings)
        return Type::enum_strings;

    if (m_type == Type::interned_strings)
        return Type::interned_strings;

    if (m_type == Type::medium_strings) {
        if (value_size <= medium_string_max_size)
            return Type::medium_strings;

        // Upgrade root leaf from medium to big strings
        auto string_medium = static_cast<ArraySmallBlobs*>(m_arr);
        ArrayBigBlobs big_blobs(m_alloc, true);
        big_blobs.create(); // Throws

        size_t n = string_medium->size();
        for (size_t i = 0; i < n; i++) {
            big_blobs.add_string(string_medium->get_string(i)); // Throws
        }
        auto parent = string_medium->get_parent();
        auto ndx_in_parent = string_medium->get_ndx_in_parent();
        string_medium->destroy();

        auto arr = new (&m_storage) ArrayBigBlobs(m_alloc, true);
        arr->init_from_mem(big_blobs.get_mem());
        arr->set_parent(parent, ndx_in_parent);
        arr->update_parent();

        m_type = Type::big_strings;
        return Type::big_strings;
    }

    // m_type == Type::small
    if (value_size <= small_string_max_size)
        return Type::small_strings;

    if (value_size <= medium_string_max_size) {
        // Upgrade root leaf from small to medium strings
        auto string_short = static_cast<ArrayStringShort*>(m_arr);
        ArraySmallBlobs string_long(m_alloc);
        string_long.create(); // Throws

        size_t n = string_short->size();
        for (size_t i = 0; i < n; i++) {
            string_long.add_string(string_short->get(i)); // Throws
        }
        auto parent = string_short->get_parent();
        auto ndx_in_parent = string_short->get_ndx_in_parent();
        string_short->destroy();

        auto arr = new (&m_storage) ArraySmallBlobs(m_alloc);
        arr->init_from_mem(string_long.get_mem());
        arr->set_parent(parent, ndx_in_parent);
        arr->update_parent();

        m_type = Type::medium_strings;
    }
    else {
        // Upgrade root leaf from small to big strings
        auto string_short = static_cast<ArrayStringShort*>(m_arr);
        ArrayBigBlobs big_blobs(m_alloc, true);
        big_blobs.create(); // Throws

        size_t n = string_short->size();
        for (size_t i = 0; i < n; i++) {
            big_blobs.add_string(string_short->get(i)); // Throws
        }
        auto parent = string_short->get_parent();
        auto ndx_in_parent = string_short->get_ndx_in_parent();
        string_short->destroy();

        auto arr = new (&m_storage) ArrayBigBlobs(m_alloc, true);
        arr->init_from_mem(big_blobs.get_mem());
        arr->set_parent(parent, ndx_in_parent);
        arr->update_parent();

        m_type = Type::big_strings;
    }

    return m_type;
}

void ArrayString::verify() const
{
#ifdef REALM_DEBUG
    switch (m_type) {
        case Type::small_strings:
            static_cast<ArrayStringShort*>(m_arr)->verify();
            break;
        case Type::medium_strings:
            static_cast<ArraySmallBlobs*>(m_arr)->verify();
            break;
        case Type::big_strings:
            static_cast<ArrayBigBlobs*>(m_arr)->verify();
            break;
        case Type::enum_strings:
        case Type::interned_strings:
            static_cast<Array*>(m_arr)->verify();
            break;
    }
#endif
}

ref_type ArrayString::write(_impl::ArrayWriterBase& out, StringInterner* interner)
{
    REALM_ASSERT(interner);
    // we have to write out all, modified or not, to match the total cleanup
    Array interned(Allocator::get_default());
    auto sz = size();
    interned.create(NodeHeader::type_Normal, true, sz);
    for (size_t i = 0; i < sz; ++i) {
        interned.set(i, interner->intern(get(i)));
    }
    auto retval = interned.write(out, false, false, out.compress);
    interned.destroy();
    return retval;
    // return m_arr->write(out, true, false, false);
}
