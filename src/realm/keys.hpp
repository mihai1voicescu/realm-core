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

#ifndef REALM_KEYS_HPP
#define REALM_KEYS_HPP

#include <realm/util/to_string.hpp>
#include <realm/column_type.hpp>
#include <ostream>
#include <vector>

namespace realm {

class ConstObj;
class Obj;

struct TableKey {
    static constexpr uint32_t null_value = uint32_t(-1) >> 1; // free top bit
    constexpr TableKey() noexcept
        : value(null_value)
    {
    }
    explicit TableKey(uint32_t val) noexcept
        : value(val)
    {
    }
    TableKey& operator=(uint32_t val) noexcept
    {
        value = val;
        return *this;
    }
    bool operator==(const TableKey& rhs) const noexcept
    {
        return value == rhs.value;
    }
    bool operator!=(const TableKey& rhs) const noexcept
    {
        return value != rhs.value;
    }
    bool operator<(const TableKey& rhs) const noexcept
    {
        return value < rhs.value;
    }
    bool operator>(const TableKey& rhs) const noexcept
    {
        return value > rhs.value;
    }

    explicit operator bool() const noexcept
    {
        return value != null_value;
    }
    uint32_t value;
};


inline std::ostream& operator<<(std::ostream& os, TableKey tk)
{
    os << "TableKey(" << tk.value << ")";
    return os;
}

namespace util {

inline std::string to_string(TableKey tk)
{
    return to_string(tk.value);
}
}

class TableVersions : public std::vector<std::pair<TableKey, uint64_t>> {
public:
    TableVersions()
    {
    }
    TableVersions(TableKey key, uint64_t version)
    {
        emplace_back(key, version);
    }
    bool operator==(const TableVersions& other) const;
};

struct ColKey {
    struct Idx {
        unsigned val;
    };

    constexpr ColKey() noexcept
        : value(uint64_t(-1) >> 1) // free top bit
    {
    }
    constexpr explicit ColKey(int64_t val) noexcept
        : value(val)
    {
    }
    constexpr ColKey(Idx index, ColumnType type, ColumnAttrMask attrs, unsigned tag) noexcept
        : ColKey((index.val & 0xFFFFUL) | ((type & 0x3FUL) << 16) | ((attrs.m_value & 0xFFUL) << 22) |
                 ((tag & 0xFFFFFFFFUL) << 30))
    {
    }
    bool is_nullable() const
    {
        return get_attrs().test(col_attr_Nullable);
    }
    bool is_list() const
    {
        return get_attrs().test(col_attr_List);
    }
    bool is_dictionary()
    {
        return get_attrs().test(col_attr_Dictionary);
    }
    bool is_collection()
    {
        return get_attrs().test(col_attr_Collection);
    }
    ColKey& operator=(int64_t val) noexcept
    {
        value = val;
        return *this;
    }
    bool operator==(const ColKey& rhs) const noexcept
    {
        return value == rhs.value;
    }
    bool operator!=(const ColKey& rhs) const noexcept
    {
        return value != rhs.value;
    }
    bool operator<(const ColKey& rhs) const noexcept
    {
        return value < rhs.value;
    }
    bool operator>(const ColKey& rhs) const noexcept
    {
        return value > rhs.value;
    }
    explicit operator bool() const noexcept
    {
        return value != ColKey().value;
    }
    Idx get_index() const noexcept
    {
        return Idx{static_cast<unsigned>(value) & 0xFFFFU};
    }
    ColumnType get_type() const noexcept
    {
        return ColumnType((static_cast<unsigned>(value) >> 16) & 0x3F);
    }
    ColumnAttrMask get_attrs() const noexcept
    {
        return ColumnAttrMask((static_cast<unsigned>(value) >> 22) & 0xFF);
    }
    unsigned get_tag() const noexcept
    {
        return (value >> 30) & 0xFFFFFFFFUL;
    }
    int64_t value;
};

inline std::ostream& operator<<(std::ostream& os, ColKey ck)
{
    os << "ColKey(" << ck.value << ")";
    return os;
}

struct ObjKey {
    constexpr ObjKey() noexcept
        : value(-1)
    {
    }
    explicit constexpr ObjKey(int64_t val) noexcept
        : value(val)
    {
    }
    bool is_unresolved() const
    {
        return value <= -2;
    }
    ObjKey get_unresolved() const
    {
        return ObjKey(-2 - value);
    }
    ObjKey& operator=(int64_t val) noexcept
    {
        value = val;
        return *this;
    }
    bool operator==(const ObjKey& rhs) const noexcept
    {
        return value == rhs.value;
    }
    bool operator!=(const ObjKey& rhs) const noexcept
    {
        return value != rhs.value;
    }
    bool operator<(const ObjKey& rhs) const noexcept
    {
        return value < rhs.value;
    }
    bool operator<=(const ObjKey& rhs) const noexcept
    {
        return value <= rhs.value;
    }
    bool operator>(const ObjKey& rhs) const noexcept
    {
        return value > rhs.value;
    }
    bool operator>=(const ObjKey& rhs) const noexcept
    {
        return value >= rhs.value;
    }
    explicit operator bool() const noexcept
    {
        return value != -1;
    }
    int64_t value;

private:
    // operator bool will enable casting to integer. Prevent this.
    operator int64_t() const = delete;
};

inline std::ostream& operator<<(std::ostream& ostr, ObjKey key)
{
    ostr << "ObjKey(" << key.value << ")";
    return ostr;
}

class ObjKeys : public std::vector<ObjKey> {
public:
    ObjKeys(const std::vector<int64_t>& init)
    {
        reserve(init.size());
        for (auto i : init) {
            emplace_back(i);
        }
    }
    ObjKeys()
    {
    }
};

struct ObjLink {
public:
    ObjLink() {}
    ObjLink(TableKey table_key, ObjKey obj_key)
        : m_obj_key(obj_key)
        , m_table_key(table_key)
    {
    }
    explicit operator bool() const
    {
        return bool(m_table_key) && bool(m_obj_key);
    }
    bool is_null() const
    {
        return !bool(*this);
    }
    }
    bool operator==(const ObjLink& other) const
    {
        return m_obj_key == other.m_obj_key && m_table_key == other.m_table_key;
    }
    bool operator!=(const ObjLink& other) const
    {
        return m_obj_key != other.m_obj_key || m_table_key != other.m_table_key;
    }
    bool operator<(const ObjLink& rhs) const
    {
        return m_table_key < rhs.m_table_key || m_obj_key < rhs.m_obj_key;
    }
    bool operator>(const ObjLink& rhs) const
    {
        return m_table_key > rhs.m_table_key || m_obj_key > rhs.m_obj_key;
    }
    TableKey get_table_key() const
    {
        return m_table_key;
    }
    ObjKey get_obj_key() const
    {
        return m_obj_key;
    }

private:
    // Having ObjKey first ensures that there will be no uninitialized space
    // in the first 12 bytes. This is important when generating a hash
    ObjKey m_obj_key;
    TableKey m_table_key;
};

inline std::ostream& operator<<(std::ostream& os, ObjLink link)
{
    os << '{' << link.get_table_key() << ',' << link.get_obj_key() << '}';
    return os;
}
constexpr ObjKey null_key;

namespace util {

inline std::string to_string(ColKey ck)
{
    return to_string(ck.value);
}

} // namespace util

} // namespace realm


namespace std {

template <>
struct hash<realm::ObjKey> {
    size_t operator()(realm::ObjKey key) const
    {
        return std::hash<uint64_t>{}(key.value);
    }
};

} // namespace std


#endif
