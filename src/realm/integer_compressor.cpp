/*************************************************************************
 *
 * Copyright 2023 Realm Inc.
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

#include <realm/integer_compressor.hpp>
#include <realm/array.hpp>
#include <realm/integer_flex_compressor.hpp>
#include <realm/integer_packed_compressor.hpp>
#include <realm/array_with_find.hpp>
#include <realm/query_conditions.hpp>

#include <vector>
#include <algorithm>

using namespace realm;

static FlexCompressor s_flex;
static PackedCompressor s_packed;

template bool IntegerCompressor::find_all_packed<Equal>(const Array&, int64_t, size_t, size_t, size_t,
                                                        QueryStateBase*) const;
template bool IntegerCompressor::find_all_packed<NotEqual>(const Array&, int64_t, size_t, size_t, size_t,
                                                           QueryStateBase*) const;
template bool IntegerCompressor::find_all_packed<Greater>(const Array&, int64_t, size_t, size_t, size_t,
                                                          QueryStateBase*) const;
template bool IntegerCompressor::find_all_packed<Less>(const Array&, int64_t, size_t, size_t, size_t,
                                                       QueryStateBase*) const;

template bool IntegerCompressor::find_all_flex<Equal>(const Array&, int64_t, size_t, size_t, size_t,
                                                      QueryStateBase*) const;
template bool IntegerCompressor::find_all_flex<NotEqual>(const Array&, int64_t, size_t, size_t, size_t,
                                                         QueryStateBase*) const;
template bool IntegerCompressor::find_all_flex<Greater>(const Array&, int64_t, size_t, size_t, size_t,
                                                        QueryStateBase*) const;
template bool IntegerCompressor::find_all_flex<Less>(const Array&, int64_t, size_t, size_t, size_t,
                                                     QueryStateBase*) const;


struct IntegerCompressor::VTableForPacked {
    struct PopulatedVTablePacked : IntegerCompressor::VTable {
        PopulatedVTablePacked()
        {
            m_getter = &IntegerCompressor::get_packed;
            m_chunk_getter = &IntegerCompressor::get_chunk_packed;
            m_direct_setter = &IntegerCompressor::set_direct_packed;
            m_finder[cond_Equal] = &IntegerCompressor::find_all_packed<Equal>;
            m_finder[cond_NotEqual] = &IntegerCompressor::find_all_packed<NotEqual>;
            m_finder[cond_Less] = &IntegerCompressor::find_all_packed<Less>;
            m_finder[cond_Greater] = &IntegerCompressor::find_all_packed<Greater>;
        }
    };
    static const PopulatedVTablePacked vtable;
};

struct IntegerCompressor::VTableForFlex {
    struct PopulatedVTableFlex : IntegerCompressor::VTable {
        PopulatedVTableFlex()
        {
            m_getter = &IntegerCompressor::get_flex;
            m_chunk_getter = &IntegerCompressor::get_chunk_flex;
            m_direct_setter = &IntegerCompressor::set_direct_flex;
            m_finder[cond_Equal] = &IntegerCompressor::find_all_flex<Equal>;
            m_finder[cond_NotEqual] = &IntegerCompressor::find_all_flex<NotEqual>;
            m_finder[cond_Less] = &IntegerCompressor::find_all_flex<Less>;
            m_finder[cond_Greater] = &IntegerCompressor::find_all_flex<Greater>;
        }
    };
    static const PopulatedVTableFlex vtable;
};

const typename IntegerCompressor::VTableForPacked::PopulatedVTablePacked IntegerCompressor::VTableForPacked::vtable;
const typename IntegerCompressor::VTableForFlex::PopulatedVTableFlex IntegerCompressor::VTableForFlex::vtable;


template <typename T, typename... Arg>
inline void compress_array(const T& compressor, Array& arr, size_t byte_size, Arg&&... args)
{
    Allocator& allocator = arr.get_alloc();
    auto mem = allocator.alloc(byte_size);
    auto h = mem.get_addr();
    compressor.init_array(h, std::forward<Arg>(args)...);
    NodeHeader::set_capacity_in_header(byte_size, h);
    arr.init_from_mem(mem);
}

template <typename T, typename... Arg>
inline void copy_into_compressed_array(const T& compress_array, Arg&&... args)
{
    compress_array.copy_data(std::forward<Arg>(args)...);
}

bool IntegerCompressor::always_compress(const Array& origin, Array& arr, NodeHeader::Encoding encoding) const
{
    using Encoding = NodeHeader::Encoding;
    std::vector<int64_t> values;
    std::vector<size_t> indices;
    compress_values(origin, values, indices);
    if (!values.empty()) {
        size_t v_width, ndx_width;
        const uint8_t flags = NodeHeader::get_flags(origin.get_header());

        if (encoding == Encoding::Packed) {
            const auto packed_size = packed_disk_size(values, origin.size(), v_width);
            compress_array(s_packed, arr, packed_size, flags, v_width, origin.size());
            copy_into_compressed_array(s_packed, origin, arr);
        }
        else if (encoding == Encoding::Flex) {
            const auto flex_size = flex_disk_size(values, indices, v_width, ndx_width);
            compress_array(s_flex, arr, flex_size, flags, v_width, ndx_width, values.size(), indices.size());
            copy_into_compressed_array(s_flex, arr, values, indices);
        }
        else {
            REALM_UNREACHABLE();
        }
        return true;
    }
    return false;
}

bool IntegerCompressor::compress(const Array& origin, Array& arr) const
{
#if REALM_COMPRESS
    return always_compress(origin, arr, NodeHeader::Encoding::Flex);
#else
    std::vector<int64_t> values;
    std::vector<size_t> indices;
    compress_values(origin, values, indices);
    if (!values.empty()) {
        size_t v_width, ndx_width;
        const auto uncompressed_size = origin.get_byte_size();
        const auto packed_size = packed_disk_size(values, origin.size(), v_width);
        const auto flex_size = flex_disk_size(values, indices, v_width, ndx_width);
        // heuristic: only compress to packed if gain at least 12.5%
        const auto adjusted_packed_size = packed_size + packed_size / 8;
        // heuristic: only compress to flex if gain at least 25%
        const auto adjusted_flex_size = flex_size + flex_size / 4;
        if (adjusted_flex_size < adjusted_packed_size && adjusted_flex_size < uncompressed_size) {
            const uint8_t flags = NodeHeader::get_flags(origin.get_header());
            compress_array(s_flex, arr, flex_size, flags, v_width, ndx_width, values.size(), indices.size());
            copy_into_compressed_array(s_flex, arr, values, indices);
            return true;
        }
        else if (adjusted_packed_size < uncompressed_size) {
            const uint8_t flags = NodeHeader::get_flags(origin.get_header());
            compress_array(s_packed, arr, packed_size, flags, v_width, origin.size());
            copy_into_compressed_array(s_packed, origin, arr);
            return true;
        }
    }
    return false;
#endif
}

bool IntegerCompressor::decompress(Array& arr) const
{
    REALM_ASSERT_DEBUG(arr.is_attached());
    auto values_fetcher = [&arr, this]() {
        std::vector<int64_t> res;
        const auto sz = arr.size();
        res.reserve(sz);
        for (size_t i = 0; i < sz; ++i)
            res.push_back((this->*(m_vtable->m_getter))(i));
        return res;
    };
    const auto& values = values_fetcher();
    //  do the reverse of compressing the array
    REALM_ASSERT_DEBUG(!values.empty());
    using Encoding = NodeHeader::Encoding;
    const auto flags = NodeHeader::get_flags(arr.get_header());
    const auto size = values.size();
    const auto [min_v, max_v] = std::minmax_element(values.begin(), values.end());
    auto width = std::max(Array::bit_width(*min_v), Array::bit_width(*max_v));
    REALM_ASSERT_DEBUG(width == 0 || width == 1 || width == 2 || width == 4 || width == 8 || width == 16 ||
                       width == 32 || width == 64);
    auto byte_size = NodeHeader::calc_size(size, width, Encoding::WTypBits);
    byte_size += 64; // this is some slab allocator magic number, this padding is needed in order to account for bit
                     // width expansion.

    REALM_ASSERT_DEBUG(byte_size % 8 == 0); // nevertheless all the values my be aligned to 8

    auto& allocator = arr.get_alloc(); // get allocator

    // store tmp header and ref, because these will be deleted once the array is restored.
    auto old_ref = arr.get_ref();
    auto old_h = arr.get_header();

    const auto mem = allocator.alloc(byte_size);
    const auto header = mem.get_addr();
    init_header(header, Encoding::WTypBits, flags, width, values.size());
    NodeHeader::set_capacity_in_header(byte_size, header);
    arr.init_from_mem(mem);

    // this is copying the bits straight, without doing any COW.
    // Restoring the array is basically COW.
    const auto sz = values.size();
    for (size_t ndx = 0; ndx < sz; ++ndx)
        set(arr.m_data, width, ndx, values[ndx]);

    // very important: since the ref of the current array has changed, the parent must be informed.
    // Otherwise we will lose the link between parent array and child array.
    arr.update_parent();
    REALM_ASSERT_DEBUG(width == arr.get_width());
    REALM_ASSERT_DEBUG(arr.size() == values.size());

    // free memory no longer used. Very important to avoid to leak memory. Either in the slab or in the C++  heap.
    allocator.free_(old_ref, old_h);
    return true;
}

bool IntegerCompressor::init(const char* h)
{
    m_encoding = NodeHeader::get_encoding(h);
    if (!NodeHeader::wtype_is_extended(h))
        return false;

    if (is_packed()) {
        init_packed(h);
        m_vtable = &VTableForPacked::vtable;
    }
    else {
        init_flex(h);
        m_vtable = &VTableForFlex::vtable;
    }
    return true;
}

size_t IntegerCompressor::flex_disk_size(const std::vector<int64_t>& values, const std::vector<size_t>& indices,
                                         size_t& v_width, size_t& ndx_width) const
{
    const auto [min_value, max_value] = std::minmax_element(values.begin(), values.end());
    ndx_width = NodeHeader::unsigned_to_num_bits(values.size());
    v_width = std::max(Node::signed_to_num_bits(*min_value), Node::signed_to_num_bits(*max_value));
    REALM_ASSERT_DEBUG(v_width > 0);
    REALM_ASSERT_DEBUG(ndx_width > 0);
    return NodeHeader::calc_size(values.size(), indices.size(), v_width, ndx_width);
}

size_t IntegerCompressor::packed_disk_size(std::vector<int64_t>& values, size_t sz, size_t& v_width) const
{
    using Encoding = NodeHeader::Encoding;
    const auto [min_value, max_value] = std::minmax_element(values.begin(), values.end());
    v_width = std::max(Node::signed_to_num_bits(*min_value), Node::signed_to_num_bits(*max_value));
    REALM_ASSERT_DEBUG(v_width > 0);
    return NodeHeader::calc_size(sz, v_width, Encoding::Packed);
}

void IntegerCompressor::compress_values(const Array& arr, std::vector<int64_t>& values,
                                        std::vector<size_t>& indices) const
{
    // The main idea is to encode the values in flex format. If Packed is better it will chosen by
    // ArrayEncode::encode. The algorithm is O(n lg n), it gives us nice properties, but we could use an efficient
    // hash table and try to boost perf during insertion. The two formats are represented as following, the array is
    // mutated in either of these 2 formats:
    //  Packed: || node header || ..... values ..... ||
    //  Flex:   || node header || ..... values ..... || ..... indices ..... ||

    const auto sz = arr.size();
    REALM_ASSERT_DEBUG(sz > 0);
    values.reserve(sz);
    indices.reserve(sz);

    for (size_t i = 0; i < sz; ++i) {
        auto item = arr.get(i);
        values.push_back(item);
        REALM_ASSERT_DEBUG(values.back() == item);
    }

    std::sort(values.begin(), values.end());
    auto last = std::unique(values.begin(), values.end());
    values.erase(last, values.end());

    for (size_t i = 0; i < arr.size(); ++i) {
        auto pos = std::lower_bound(values.begin(), values.end(), arr.get(i));
        indices.push_back(std::distance(values.begin(), pos));
        REALM_ASSERT_DEBUG(values[indices[i]] == arr.get(i));
    }

#if REALM_DEBUG
    for (size_t i = 0; i < sz; ++i) {
        auto old_value = arr.get(i);
        auto new_value = values[indices[i]];
        REALM_ASSERT_DEBUG(new_value == old_value);
    }
#endif
    REALM_ASSERT_DEBUG(indices.size() == sz);
}

void IntegerCompressor::set(char* data, size_t w, size_t ndx, int64_t v) const
{
    if (w == 0)
        realm::set_direct<0>(data, ndx, v);
    else if (w == 1)
        realm::set_direct<1>(data, ndx, v);
    else if (w == 2)
        realm::set_direct<2>(data, ndx, v);
    else if (w == 4)
        realm::set_direct<4>(data, ndx, v);
    else if (w == 8)
        realm::set_direct<8>(data, ndx, v);
    else if (w == 16)
        realm::set_direct<16>(data, ndx, v);
    else if (w == 32)
        realm::set_direct<32>(data, ndx, v);
    else if (w == 64)
        realm::set_direct<64>(data, ndx, v);
    else
        REALM_UNREACHABLE();
}

int64_t IntegerCompressor::get_packed(size_t ndx) const
{
    return s_packed.get(*this, ndx);
}

int64_t IntegerCompressor::get_flex(size_t ndx) const
{
    return s_flex.get(*this, ndx);
}

void IntegerCompressor::get_chunk_packed(size_t ndx, int64_t res[8]) const
{
    s_packed.get_chunk(*this, ndx, res);
}

void IntegerCompressor::get_chunk_flex(size_t ndx, int64_t res[8]) const
{
    s_flex.get_chunk(*this, ndx, res);
}

void IntegerCompressor::set_direct_packed(size_t ndx, int64_t value) const
{
    s_packed.set_direct(*this, ndx, value);
}

void IntegerCompressor::set_direct_flex(size_t ndx, int64_t value) const
{
    s_flex.set_direct(*this, ndx, value);
}

template <typename Cond>
bool IntegerCompressor::find_all_packed(const Array& arr, int64_t value, size_t start, size_t end, size_t baseindex,
                                        QueryStateBase* state) const
{
    return s_packed.find_all<Cond>(arr, value, start, end, baseindex, state);
}

template <typename Cond>
bool IntegerCompressor::find_all_flex(const Array& arr, int64_t value, size_t start, size_t end, size_t baseindex,
                                      QueryStateBase* state) const
{
    return s_flex.find_all<Cond>(arr, value, start, end, baseindex, state);
}
