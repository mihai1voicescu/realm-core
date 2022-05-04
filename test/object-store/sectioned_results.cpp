////////////////////////////////////////////////////////////////////////////
//
// Copyright 2022 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#include <catch2/catch.hpp>

#include "util/test_file.hpp"

#include <realm/object-store/impl/realm_coordinator.hpp>
#include <realm/object-store/list.hpp>
#include <realm/object-store/results.hpp>
#include <realm/object-store/sectioned_results.hpp>

#include <realm/util/any.hpp>

using namespace realm;
using namespace realm::util;

namespace realm::sectioned_results_fixtures {

template <PropertyType prop_type, typename T>
struct Base {
    using Type = T;

    static PropertyType property_type()
    {
        return prop_type;
    }
};

struct Int : Base<PropertyType::Int, int64_t> {
    static std::vector<int64_t> values()
    {
        return {1, 2, 3, 4, 5, 1, 2, 3, 4, 5};
    }

    static std::vector<int64_t> expected_unsorted()
    {
        return {2, 4, 2, 4, 1, 3, 5, 1, 3, 5};
    }

    static std::vector<int64_t> expected_sorted()
    {
        return {2, 2, 4, 4, 1, 1, 3, 3, 5, 5};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {0, 1};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Section odd and even numbers.
        if (value.is_null()) {
            return "nulls";
        }
        return value.get_int() % 2;
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct Bool : Base<PropertyType::Bool, bool> {
    static std::vector<bool> values()
    {
        return {true, false, true, false};
    }

    static std::vector<bool> expected_unsorted()
    {
        return {false, false, true, true};
    }

    static std::vector<bool> expected_sorted()
    {
        return {false, false, true, true};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {false, true};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Section true from false
        if (value.is_null()) {
            return "nulls";
        }
        return value.get_bool();
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct Float : Base<PropertyType::Float, float> {
    static std::vector<float> values()
    {
        return {1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 1.1f, 2.2f, 3.3f, 4.4f, 5.5f};
    }

    static std::vector<float> expected_unsorted()
    {
        return {2.2f, 4.4f, 2.2f, 4.4f, 1.1f, 3.3f, 5.5f, 1.1f, 3.3f, 5.5f};
    }

    static std::vector<float> expected_sorted()
    {
        return {2.2f, 2.2f, 4.4f, 4.4f, 1.1f, 1.1f, 3.3f, 3.3f, 5.5f, 5.5f};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {0, 1};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Section odd and even numbers.
        if (value.is_null()) {
            return "nulls";
        }
        return int(value.get_float()) % 2;
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct Double : Base<PropertyType::Double, double> {
    static std::vector<double> values()
    {
        return {1.1, 2.2, 3.3, 4.4, 5.5, 1.2, 2.3, 3.4, 4.5, 5.6};
    }

    static std::vector<double> expected_unsorted()
    {
        return {2.2, 4.4, 2.3, 4.5, 1.1, 3.3, 5.5, 1.2, 3.4, 5.6};
    }

    static std::vector<double> expected_sorted()
    {
        return {2.2, 2.3, 4.4, 4.5, 1.1, 1.2, 3.3, 3.4, 5.5, 5.6};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {0, 1};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Section odd and even numbers.
        if (value.is_null()) {
            return "nulls";
        }
        return int(value.get_double()) % 2;
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct String : Base<PropertyType::String, StringData> {
    using Boxed = std::string;
    static std::vector<StringData> values()
    {
        return {"apple",  "banana",  "cherry",   "dragon fruit",   "elderberry",
                "apples", "bananas", "cherries", "dragon fruit's", "elderberries"};
    }

    static std::vector<StringData> expected_unsorted()
    {
        return {"apple",    "apples",       "banana",         "bananas",    "cherry",
                "cherries", "dragon fruit", "dragon fruit's", "elderberry", "elderberries"};
    }

    static std::vector<StringData> expected_sorted()
    {
        return {"apple",  "apples",       "banana",         "bananas",      "cherries",
                "cherry", "dragon fruit", "dragon fruit's", "elderberries", "elderberry"};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {"a", "b", "c", "d", "e"};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Return first char of string.
        if (value.is_null()) {
            return "nulls";
        }
        auto str = value.get_string();
        return str.size() > 0 ? str.prefix(1) : str;
    }

    static size_t expected_size()
    {
        return 5;
    }
};

struct Binary : Base<PropertyType::Data, BinaryData> {
    using Boxed = std::string;
    static std::vector<BinaryData> values()
    {
        return {BinaryData("a", 1), BinaryData("aa", 2), BinaryData("b", 1), BinaryData("bb", 2),
                BinaryData("c", 1), BinaryData("cc", 2), BinaryData("d", 1), BinaryData("dd", 2),
                BinaryData("e", 1), BinaryData("ee", 2)};
    }

    static std::vector<BinaryData> expected_unsorted()
    {
        return {BinaryData("a", 1),  BinaryData("b", 1),  BinaryData("c", 1),  BinaryData("d", 1),
                BinaryData("e", 1),  BinaryData("aa", 2), BinaryData("bb", 2), BinaryData("cc", 2),
                BinaryData("dd", 2), BinaryData("ee", 2)};
    }

    static std::vector<BinaryData> expected_sorted()
    {
        return {BinaryData("a", 1),  BinaryData("b", 1),  BinaryData("c", 1),  BinaryData("d", 1),
                BinaryData("e", 1),  BinaryData("aa", 2), BinaryData("bb", 2), BinaryData("cc", 2),
                BinaryData("dd", 2), BinaryData("ee", 2)};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {1, 2};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Seperate by size of data
        if (value.is_null()) {
            return "nulls";
        }
        return (int)value.get_binary().size();
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct Date : Base<PropertyType::Date, Timestamp> {
    static std::vector<Timestamp> values()
    {
        return {Timestamp(1, 1),  Timestamp(20, 2), Timestamp(3, 1),  Timestamp(40, 2), Timestamp(5, 1),
                Timestamp(10, 2), Timestamp(2, 1),  Timestamp(30, 2), Timestamp(4, 1),  Timestamp(50, 2)};
    }

    static std::vector<Timestamp> expected_unsorted()
    {
        return {Timestamp(20, 2), Timestamp(40, 2), Timestamp(10, 2), Timestamp(30, 2), Timestamp(50, 2),
                Timestamp(1, 1),  Timestamp(3, 1),  Timestamp(5, 1),  Timestamp(2, 1),  Timestamp(4, 1)};
    }

    static std::vector<Timestamp> expected_sorted()
    {
        return {Timestamp(10, 2), Timestamp(20, 2), Timestamp(30, 2), Timestamp(40, 2), Timestamp(50, 2),
                Timestamp(1, 1),  Timestamp(2, 1),  Timestamp(3, 1),  Timestamp(4, 1),  Timestamp(5, 1)};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {"Bar", "Foo"};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Seperate by size of data
        if (value.is_null()) {
            return "nulls";
        }
        return value.get_timestamp().get_seconds() < 10 ? "Foo" : "Bar";
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct MixedVal : Base<PropertyType::Mixed, realm::Mixed> {
    static std::vector<realm::Mixed> values()
    {
        return {Mixed{realm::UUID()},      Mixed{int64_t(1)},
                Mixed{util::none},         Mixed{"hello world"},
                Mixed{Timestamp(1, 1)},    Mixed{Decimal128("300")},
                Mixed{double(2.2)},        Mixed{float(3.3)},
                Mixed{BinaryData("a", 1)}, Mixed{ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb")}};
    }

    static std::vector<realm::Mixed> expected_unsorted()
    {
        return {Mixed{realm::UUID()},
                Mixed{"hello world"},
                Mixed{Timestamp(1, 1)},
                Mixed{BinaryData("a", 1)},
                Mixed{ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb")},
                Mixed{util::none},
                Mixed{int64_t(1)},
                Mixed{Decimal128("300")},
                Mixed{double(2.2)},
                Mixed{float(3.3)}};
    }

    static std::vector<realm::Mixed> expected_sorted()
    {
        return {Mixed{BinaryData("a", 1)}, Mixed{"hello world"},
                Mixed{Timestamp(1, 1)},    Mixed{ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb")},
                Mixed{realm::UUID()},      Mixed{util::none},
                Mixed{int64_t(1)},         Mixed{double(2.2)},
                Mixed{float(3.3)},         Mixed{Decimal128("300")}};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {"Alphanumeric", "Empty", "Numerics"};
    }

    static Mixed comparison_value(Mixed value)
    {
        if (value.is_null()) {
            return "Empty";
        }
        // Seperate numeric from non numeric
        return Mixed::is_numeric(value.get_type()) ? "Numerics" : "Alphanumeric";
    }

    static PropertyType property_type()
    {
        return PropertyType::Mixed | PropertyType::Nullable;
    }

    static size_t expected_size()
    {
        return 3;
    }
};

struct OID : Base<PropertyType::ObjectId, ObjectId> {
    static std::vector<ObjectId> values()
    {
        return {ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"), ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"),
                ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"),
                ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"),
                ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"), ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"),
                ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb")};
    }

    static std::vector<ObjectId> expected_unsorted()
    {
        return {ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"),
                ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"),
                ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"),
                ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"),
                ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa")};
    }

    static std::vector<ObjectId> expected_sorted()
    {
        return {ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"),
                ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"),
                ObjectId("bbbbbbbbbbbbbbbbbbbbbbbb"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"),
                ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"),
                ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa"), ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa")};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {"Bar", "Foo"};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Seperate by sections containing the same ObjectId's
        if (value.is_null()) {
            return "nulls";
        }
        return value.get_object_id() == ObjectId("aaaaaaaaaaaaaaaaaaaaaaaa") ? "Foo" : "Bar";
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct UUID : Base<PropertyType::UUID, realm::UUID> {
    static std::vector<realm::UUID> values()
    {
        return {
            realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"),
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"),
            realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"), realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"),
            realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"), realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"),
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"),
        };
    }

    static std::vector<realm::UUID> expected_unsorted()
    {
        return {
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"),
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"),
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"),
            realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"),
            realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962")};
    }

    static std::vector<realm::UUID> expected_sorted()
    {
        return {
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"),
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"),
            realm::UUID("1b241101-a2b3-4255-8caf-4136c566a999"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"),
            realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"),
            realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962"), realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962")};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {"Bar", "Foo"};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Seperate by sections containing the same UUID's
        if (value.is_null()) {
            return "nulls";
        }
        return value.get_uuid() == realm::UUID("1a241101-e2bb-4255-8caf-4136c566a962") ? "Foo" : "Bar";
    }

    static size_t expected_size()
    {
        return 2;
    }
};

struct Decimal : Base<PropertyType::Decimal, Decimal128> {
    static std::vector<Decimal128> values()
    {
        return {
            Decimal128("876.54e32"), Decimal128("123.45e6"), Decimal128("876.54e32"), Decimal128("123.45e6"),
            Decimal128("876.54e32"), Decimal128("123.45e6"), Decimal128("876.54e32"), Decimal128("123.45e6"),
            Decimal128("876.54e32"), Decimal128("123.45e6"),
        };
    }

    static std::vector<Decimal128> expected_unsorted()
    {
        return {Decimal128("876.54e32"), Decimal128("876.54e32"), Decimal128("876.54e32"), Decimal128("876.54e32"),
                Decimal128("876.54e32"), Decimal128("123.45e6"),  Decimal128("123.45e6"),  Decimal128("123.45e6"),
                Decimal128("123.45e6"),  Decimal128("123.45e6")};
    }

    static std::vector<Decimal128> expected_sorted()
    {
        return {Decimal128("876.54e32"), Decimal128("876.54e32"), Decimal128("876.54e32"), Decimal128("876.54e32"),
                Decimal128("876.54e32"), Decimal128("123.45e6"),  Decimal128("123.45e6"),  Decimal128("123.45e6"),
                Decimal128("123.45e6"),  Decimal128("123.45e6")};
    }

    static std::vector<Mixed> expected_keys()
    {
        return {false, true};
    }

    static Mixed comparison_value(Mixed value)
    {
        // Seperate smaller values
        if (value.is_null()) {
            return "nulls";
        }
        return value.get_decimal() < Decimal128("876.54e32");
    }

    static size_t expected_size()
    {
        return 2;
    }
};

template <typename BaseT>
struct BoxedOptional : BaseT {
    using Type = util::Optional<typename BaseT::Type>;
    using Boxed = Type;
    static PropertyType property_type()
    {
        return BaseT::property_type() | PropertyType::Nullable;
    }
    static std::vector<Type> values()
    {
        std::vector<Type> ret;
        for (auto v : BaseT::values())
            ret.push_back(Type(v));
        ret.push_back(util::none);
        return ret;
    }

    static size_t expected_size()
    {
        return BaseT::expected_size() + 1;
    }

    static std::vector<Type> expected_unsorted()
    {
        std::vector<Type> ret;
        for (auto v : BaseT::expected_unsorted())
            ret.push_back(Type(v));
        ret.push_back(util::none);
        return ret;
    }

    static std::vector<Type> expected_sorted()
    {
        std::vector<Type> ret;
        for (auto v : BaseT::expected_sorted())
            ret.push_back(Type(v));
        ret.push_back(util::none);
        return ret;
    }

    static std::vector<Mixed> expected_keys()
    {
        auto exp_keys = BaseT::expected_keys();
        exp_keys.push_back("nulls");
        return exp_keys;
    }
};

template <typename BaseT>
struct UnboxedOptional : BaseT {
    enum { is_optional = true };
    static PropertyType property_type()
    {
        return BaseT::property_type() | PropertyType::Nullable;
    }
    static auto values() -> decltype(BaseT::values())
    {
        auto ret = BaseT::values();
        if constexpr (std::is_same_v<BaseT, sectioned_results_fixtures::Decimal>) {
            // The default Decimal128 ctr is 0, but we want a null value
            ret.push_back(Decimal128(realm::null()));
        }
        else {
            ret.push_back(typename BaseT::Type());
        }
        return ret;
    }

    static size_t expected_size()
    {
        return BaseT::expected_size() + 1;
    }

    static auto expected_unsorted() -> decltype(BaseT::values())
    {
        auto ret = BaseT::expected_unsorted();
        if constexpr (std::is_same_v<BaseT, sectioned_results_fixtures::Decimal>) {
            // The default Decimal128 ctr is 0, but we want a null value
            ret.push_back(Decimal128(realm::null()));
        }
        else {
            ret.push_back(typename BaseT::Type());
        }
        return ret;
    }

    static auto expected_sorted() -> decltype(BaseT::values())
    {
        auto ret = BaseT::expected_sorted();
        if constexpr (std::is_same_v<BaseT, sectioned_results_fixtures::Decimal>) {
            // The default Decimal128 ctr is 0, but we want a null value
            ret.push_back(Decimal128(realm::null()));
        }
        else {
            ret.push_back(typename BaseT::Type());
        }
        return ret;
    }

    static std::vector<Mixed> expected_keys()
    {
        auto exp_keys = BaseT::expected_keys();
        exp_keys.push_back("nulls");
        return exp_keys;
    }
};
} // namespace realm::sectioned_results_fixtures

TEST_CASE("sectioned results", "[sectioned_results]") {
    _impl::RealmCoordinator::assert_no_open_realms();

    InMemoryTestFile config;
    config.automatic_change_notifications = false;

    auto r = Realm::get_shared_realm(config);
    r->update_schema({
        {"object",
         {{"name_col", PropertyType::String},
          {"int_col", PropertyType::Int},
          {"array_string_col", PropertyType::String | PropertyType::Array},
          {"array_int_col", PropertyType::Int | PropertyType::Array}}},
    });

    auto coordinator = _impl::RealmCoordinator::get_coordinator(config.path);
    auto table = r->read_group().get_table("class_object");
    auto name_col = table->get_column_key("name_col");
    auto int_col = table->get_column_key("int_col");
    auto array_string_col = table->get_column_key("array_string_col");

    r->begin_transaction();
    auto o1 = table->create_object();
    o1.set(name_col, "banana");
    o1.set(int_col, 3);
    auto o2 = table->create_object();
    o2.set(name_col, "apricot");
    o2.set(int_col, 2);
    auto o3 = table->create_object();
    o3.set(name_col, "apple");
    o3.set(int_col, 1);
    auto o4 = table->create_object();
    o4.set(name_col, "orange");
    o4.set(int_col, 2);
    auto o5 = table->create_object();
    o5.set(name_col, "apples");
    o5.set(int_col, 3);
    r->commit_transaction();

    Results results(r, table);
    auto sorted = results.sort({{"name_col", true}});
    int algo_run_count = 0;
    auto sectioned_results = sorted.sectioned_results([&algo_run_count](Mixed value, SharedRealm realm) {
        algo_run_count++;
        auto obj = Object(realm, value.get_link());
        auto v = obj.get_column_value<StringData>("name_col");
        return v.prefix(1);
    });
    REQUIRE(algo_run_count == 5);

    SECTION("sorts results correctly") {
        REQUIRE(sectioned_results.size() == 3);
        REQUIRE(sectioned_results[0].size() == 3);
        REQUIRE(sectioned_results[1].size() == 1);
        REQUIRE(sectioned_results[2].size() == 1);
        REQUIRE(algo_run_count == 5);
        std::vector<std::string> expected{"apple", "apples", "apricot", "banana", "orange"};

        int count = 0;
        for (size_t i = 0; i < sectioned_results.size(); i++) {
            auto section = sectioned_results[i];
            for (size_t y = 0; y < section.size(); y++) {
                auto val = Object(r, section[y].get_link()).get_column_value<StringData>("name_col");
                REQUIRE(expected[count] == val);
                count++;
            }
        }
        REQUIRE(algo_run_count == 5);
        REQUIRE(count == 5);
    }

    SECTION("sorts results correctly after update") {
        REQUIRE(sectioned_results.size() == 3);
        REQUIRE(sectioned_results[0].size() == 3);
        REQUIRE(sectioned_results[1].size() == 1);
        REQUIRE(sectioned_results[2].size() == 1);
        REQUIRE(algo_run_count == 5);
        // reset the callback count as it will need to run once we add new objects;
        algo_run_count = 0;

        coordinator->on_change();
        r->begin_transaction();
        table->create_object().set(name_col, "safari");
        table->create_object().set(name_col, "mail");
        table->create_object().set(name_col, "car");
        table->create_object().set(name_col, "stocks");
        table->create_object().set(name_col, "cake");
        r->commit_transaction();

        REQUIRE(sectioned_results.size() == 6);
        REQUIRE(algo_run_count == 10);
        std::vector<std::string> expected{"apple", "apples", "apricot", "banana", "cake",
                                          "car",   "mail",   "orange",  "safari", "stocks"};

        int count = 0;
        for (size_t i = 0; i < sectioned_results.size(); i++) {
            auto section = sectioned_results[i];
            for (size_t y = 0; y < section.size(); y++) {
                auto val = Object(r, section[y].get_link()).get_column_value<StringData>("name_col");
                REQUIRE(expected[count] == val);
                count++;
            }
        }
        REQUIRE(algo_run_count == 10);
        REQUIRE(count == 10);
    }

    SECTION("FirstLetter builtin with link") {
        auto sr = sorted.sectioned_results(Results::SectionedResultsOperator::FirstLetter,
                                           util::Optional<StringData>("name_col"));

        REQUIRE(sr.size() == 3);
        REQUIRE(sr[0].size() == 3);
        REQUIRE(sr[1].size() == 1);
        REQUIRE(sr[2].size() == 1);

        std::vector<std::string> expected{"apple", "apples", "apricot", "banana", "orange"};

        std::vector<std::string> expected_keys{"a", "b", "o"};

        int section_count = 0;
        int element_count = 0;
        for (size_t i = 0; i < sr.size(); i++) {
            auto section = sr[i];
            REQUIRE(section.key().get_string() == expected_keys[section_count]);
            section_count++;
            for (size_t y = 0; y < section.size(); y++) {
                auto val = Object(r, section[y].get_link()).get_column_value<StringData>("name_col");
                REQUIRE(expected[element_count] == val);
                element_count++;
            }
        }
        REQUIRE(section_count == 3);
        REQUIRE(element_count == 5);

        algo_run_count = 0;
        // Insert empty string
        coordinator->on_change();
        r->begin_transaction();
        table->create_object().set(name_col, "");
        r->commit_transaction();

        expected.insert(expected.begin(), 1, "");
        expected_keys.insert(expected_keys.begin(), 1, "");

        section_count = 0;
        element_count = 0;
        for (size_t i = 0; i < sr.size(); i++) {
            auto section = sr[i];
            REQUIRE(section.key().get_string() == expected_keys[section_count]);
            section_count++;
            for (size_t y = 0; y < section.size(); y++) {
                auto val = Object(r, section[y].get_link()).get_column_value<StringData>("name_col");
                REQUIRE(expected[element_count] == val);
                element_count++;
            }
        }
        REQUIRE(section_count == 4);
        REQUIRE(element_count == 6);
    }

    SECTION("FirstLetter builtin with primitive") {
        r->begin_transaction();
        auto o1 = table->create_object();
        auto str_list = o1.get_list<StringData>(array_string_col);
        str_list.add("apple");
        str_list.add("apples");
        str_list.add("apricot");
        str_list.add("banana");
        str_list.add("orange");
        r->commit_transaction();
        List lst(r, o1, array_string_col);
        auto sr = lst.as_results().sectioned_results(Results::SectionedResultsOperator::FirstLetter);

        REQUIRE(sr.size() == 3);
        REQUIRE(sr[0].size() == 3);
        REQUIRE(sr[1].size() == 1);
        REQUIRE(sr[2].size() == 1);

        std::vector<std::string> expected{"apple", "apples", "apricot", "banana", "orange"};

        std::vector<std::string> expected_keys{"a", "b", "o"};

        int section_count = 0;
        int element_count = 0;
        for (size_t i = 0; i < sr.size(); i++) {
            auto section = sr[i];
            REQUIRE(section.key().get_string() == expected_keys[section_count]);
            section_count++;
            for (size_t y = 0; y < section.size(); y++) {
                auto val = section[y].get_string();
                REQUIRE(expected[element_count] == val);
                element_count++;
            }
        }
        REQUIRE(section_count == 3);
        REQUIRE(element_count == 5);

        // Insert empty string
        coordinator->on_change();
        r->begin_transaction();
        lst.add(StringData(""));
        r->commit_transaction();

        expected.insert(expected.begin(), 1, "");
        expected_keys.insert(expected_keys.begin(), 1, "");

        section_count = 0;
        element_count = 0;
        for (size_t i = 0; i < sr.size(); i++) {
            auto section = sr[i];
            REQUIRE(section.key().get_string() == expected_keys[section_count]);
            section_count++;
            for (size_t y = 0; y < section.size(); y++) {
                auto val = section[y].get_string();
                REQUIRE(expected[element_count] == val);
                element_count++;
            }
        }
        REQUIRE(section_count == 4);
        REQUIRE(element_count == 6);
    }

    SECTION("notifications") {
        SectionedResultsChangeSet changes;
        auto token =
            sectioned_results.add_notification_callback([&](SectionedResultsChangeSet c, std::exception_ptr err) {
                REQUIRE_FALSE(err);
                changes = c;
            });

        coordinator->on_change();
        REQUIRE(algo_run_count == 5);
        algo_run_count = 0;

        // Insertions
        r->begin_transaction();
        auto o1 = table->create_object().set(name_col, "safari");
        auto o2 = table->create_object().set(name_col, "mail");
        auto o3 = table->create_object().set(name_col, "czar");
        auto o4 = table->create_object().set(name_col, "stocks");
        auto o5 = table->create_object().set(name_col, "cake");
        auto o6 = table->create_object().set(name_col, "any");
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(algo_run_count == 11);

        REQUIRE(changes.insertions.size() == 4);
        // Section 0 is 'A'
        REQUIRE(changes.insertions[0].size() == 1);
        REQUIRE(changes.insertions[0][0] == 0);
        // Section 2 is 'C'
        REQUIRE(changes.insertions[2].size() == 2);
        REQUIRE(changes.insertions[2][0] == 0);
        REQUIRE(changes.insertions[2][1] == 1);
        // Section 3 is 'M'
        REQUIRE(changes.insertions[3].size() == 1);
        REQUIRE(changes.insertions[3][0] == 0);
        // Section 5 is 'S'
        REQUIRE(changes.insertions[5].size() == 2);
        REQUIRE(changes.insertions[5][0] == 0);
        REQUIRE(changes.insertions[5][1] == 1);
        REQUIRE(changes.modifications.empty());
        REQUIRE(changes.deletions.empty());

        algo_run_count = 0;
        // Modifications
        r->begin_transaction();
        o4.set(name_col, "stocksss");
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(changes.modifications.size() == 1);
        REQUIRE(changes.modifications[5][0] == 1);
        REQUIRE(changes.insertions.empty());
        REQUIRE(changes.deletions.empty());
        REQUIRE(algo_run_count == 11);

        algo_run_count = 0;
        // Deletions
        r->begin_transaction();
        table->remove_object(o3.get_key());
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(changes.deletions.size() == 1);
        REQUIRE(changes.deletions[2][0] == 1);
        REQUIRE(changes.insertions.empty());
        REQUIRE(changes.modifications.empty());
        REQUIRE(algo_run_count == 10);
    }

    SECTION("notifications on section") {
        auto section1 = sectioned_results[0];
        int section1_notification_calls = 0;
        SectionedResultsChangeSet section1_changes;
        auto token1 = section1.add_notification_callback([&](SectionedResultsChangeSet c, std::exception_ptr err) {
            REQUIRE_FALSE(err);
            section1_changes = c;
            ++section1_notification_calls;
        });

        auto section2 = sectioned_results[1];
        int section2_notification_calls = 0;
        SectionedResultsChangeSet section2_changes;
        auto token2 = section2.add_notification_callback([&](SectionedResultsChangeSet c, std::exception_ptr err) {
            REQUIRE_FALSE(err);
            section2_changes = c;
            ++section2_notification_calls;
        });

        coordinator->on_change();
        REQUIRE(algo_run_count == 5);
        algo_run_count = 0;

        // Insertions
        r->begin_transaction();
        auto o1 = table->create_object().set(name_col, "any");
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(algo_run_count == 6);

        REQUIRE(section1_notification_calls == 1);
        REQUIRE(section2_notification_calls == 0);
        REQUIRE(section1_changes.insertions.size() == 1);
        REQUIRE(section1_changes.insertions[0].size() == 1);
        REQUIRE(section1_changes.insertions[0][0] == 0);
        REQUIRE(section1_changes.modifications.empty());
        REQUIRE(section1_changes.deletions.empty());
        algo_run_count = 0;

        r->begin_transaction();
        auto o2 = table->create_object().set(name_col, "box");
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(section1_notification_calls == 1);
        REQUIRE(section2_notification_calls == 1);
        REQUIRE(section2_changes.insertions.size() == 1);
        REQUIRE(section2_changes.insertions[1].size() == 1);
        REQUIRE(section2_changes.insertions[1][0] == 1);
        REQUIRE(section2_changes.modifications.empty());
        REQUIRE(section2_changes.deletions.empty());
        REQUIRE(algo_run_count == 7);
        algo_run_count = 0;

        // Modifications
        r->begin_transaction();
        o1.set(name_col, "anyyy");
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(section1_notification_calls == 2);
        REQUIRE(section2_notification_calls == 1);
        REQUIRE(section1_changes.modifications.size() == 1);
        REQUIRE(section1_changes.modifications[0][0] == 0);
        REQUIRE(section1_changes.insertions.empty());
        REQUIRE(section1_changes.deletions.empty());
        REQUIRE(algo_run_count == 7);
        algo_run_count = 0;

        // Deletions
        r->begin_transaction();
        table->remove_object(o2.get_key());
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(section1_notification_calls == 2);
        REQUIRE(section2_notification_calls == 2);
        REQUIRE(section2_changes.deletions.size() == 1);
        REQUIRE(section2_changes.deletions[1][0] == 1);
        REQUIRE(section2_changes.insertions.empty());
        REQUIRE(section2_changes.modifications.empty());
        REQUIRE(algo_run_count == 6);
        algo_run_count = 0;

        r->begin_transaction();
        table->remove_object(o5.get_key());
        r->commit_transaction();
        advance_and_notify(*r);
        REQUIRE(section1_notification_calls == 3);
        REQUIRE(section2_notification_calls == 2);
        REQUIRE(section1_changes.deletions.size() == 1);
        REQUIRE(section1_changes.deletions[0][0] == 2);
        REQUIRE(section1_changes.insertions.empty());
        REQUIRE(section1_changes.modifications.empty());
        REQUIRE(algo_run_count == 5);
    }

    SECTION("snapshot") {
        auto sr_snapshot = sectioned_results.snapshot();

        REQUIRE(sr_snapshot.size() == 3);
        REQUIRE(sr_snapshot[0].size() == 3);
        REQUIRE(sr_snapshot[1].size() == 1);
        REQUIRE(sr_snapshot[2].size() == 1);
        REQUIRE(algo_run_count == 5);
        std::vector<std::string> expected{"apple", "apples", "apricot", "banana", "orange"};

        int count = 0;
        for (size_t i = 0; i < sr_snapshot.size(); i++) {
            auto section = sr_snapshot[i];
            for (size_t y = 0; y < section.size(); y++) {
                auto val = Object(r, section[y].get_link()).get_column_value<StringData>("name_col");
                REQUIRE(expected[count] == val);
                count++;
            }
        }
        REQUIRE(algo_run_count == 5);
        REQUIRE(count == 5);

        r->begin_transaction();
        table->create_object().set(name_col, "any");
        table->create_object().set(name_col, "zebra");
        r->commit_transaction();
        advance_and_notify(*r);

        // results should stay the same.
        count = 0;
        for (size_t i = 0; i < sr_snapshot.size(); i++) {
            auto section = sr_snapshot[i];
            for (size_t y = 0; y < section.size(); y++) {
                auto val = Object(r, section[y].get_link()).get_column_value<StringData>("name_col");
                REQUIRE(expected[count] == val);
                count++;
            }
        }
        REQUIRE(algo_run_count == 5);
        REQUIRE(count == 5);
    }
}

namespace cf = realm::sectioned_results_fixtures;

TEMPLATE_TEST_CASE("sectioned results primitive types", "[sectioned_results]", cf::MixedVal, cf::Int, cf::Bool,
                   cf::Float, cf::Double, cf::String, cf::Binary, cf::Date, cf::OID, cf::Decimal, cf::UUID,
                   cf::BoxedOptional<cf::Int>, cf::BoxedOptional<cf::Bool>, cf::BoxedOptional<cf::Float>,
                   cf::BoxedOptional<cf::Double>, cf::BoxedOptional<cf::OID>, cf::BoxedOptional<cf::UUID>,
                   cf::UnboxedOptional<cf::String>, cf::UnboxedOptional<cf::Binary>, cf::UnboxedOptional<cf::Date>,
                   cf::UnboxedOptional<cf::Decimal>)
{
    using T = typename TestType::Type;

    _impl::RealmCoordinator::assert_no_open_realms();

    InMemoryTestFile config;
    config.automatic_change_notifications = false;

    auto r = Realm::get_shared_realm(config);
    r->update_schema({
        {"object",
         {{"value_col", TestType::property_type()}, {"array_col", PropertyType::Array | TestType::property_type()}}},
    });

    auto coordinator = _impl::RealmCoordinator::get_coordinator(config.path);
    auto table = r->read_group().get_table("class_object");
    auto array_col = table->get_column_key("array_col");

    auto values = TestType::values();
    auto exp_keys = TestType::expected_keys();
    auto exp_values = TestType::expected_unsorted();
    auto exp_values_sorted = TestType::expected_sorted();

    r->begin_transaction();
    auto o = table->create_object();
    auto list = o.get_list<T>(array_col);
    for (size_t i = 0; i < values.size(); ++i) {
        list.add(T(values[i]));
    }
    r->commit_transaction();
    List lst(r, o, array_col);
    auto results = lst.as_results();
    auto algo_run_count = 0;

    SECTION("primitives section correctly unsorted") {
        auto sectioned_results = results.sectioned_results([&algo_run_count](Mixed value, SharedRealm) -> Mixed {
            algo_run_count++;
            return TestType::comparison_value(value);
        });
        REQUIRE(sectioned_results.size() == TestType::expected_size());
        auto results_idx = 0;
        for (size_t section_idx = 0; section_idx < sectioned_results.size(); section_idx++) {
            auto section = sectioned_results[section_idx];
            REQUIRE(exp_keys[section_idx] == section.key());
            for (size_t element_idx = 0; element_idx < section.size(); element_idx++) {
                auto element = sectioned_results[section_idx][element_idx];
                Mixed value = T(exp_values[results_idx]);
                REQUIRE(element == value);
                results_idx++;
            }
        }
        REQUIRE(algo_run_count == (int)exp_values.size());
    }

    SECTION("primitives section correctly with sort ascending") {
        auto sorted = results.sort({{"self", true}});
        auto sectioned_results = sorted.sectioned_results([&algo_run_count](Mixed value, SharedRealm) -> Mixed {
            algo_run_count++;
            return TestType::comparison_value(value);
        });
        REQUIRE(sectioned_results.size() == TestType::expected_size());
        auto size = sectioned_results.size();
        auto results_idx = 0;
        for (size_t section_idx = 0; section_idx < size; section_idx++) {
            auto section = sectioned_results[section_idx];
            REQUIRE(exp_keys[section_idx] == section.key());
            for (size_t element_idx = 0; element_idx < section.size(); element_idx++) {
                auto element = sectioned_results[section_idx][element_idx];
                Mixed value = T(exp_values_sorted[results_idx]);
                REQUIRE(element == value);
                results_idx++;
            }
        }
        REQUIRE(algo_run_count == (int)exp_values.size());
    }

    SECTION("primitives section correctly with sort decending") {
        auto sorted = results.sort({{"self", false}});
        auto sectioned_results = sorted.sectioned_results([&algo_run_count](Mixed value, SharedRealm) -> Mixed {
            algo_run_count++;
            return TestType::comparison_value(value);
        });
        std::reverse(exp_values_sorted.begin(), exp_values_sorted.end());
        std::reverse(exp_keys.begin(), exp_keys.end());
        REQUIRE(sectioned_results.size() == TestType::expected_size());
        auto size = sectioned_results.size();
        auto results_idx = 0;
        for (size_t section_idx = 0; section_idx < size; section_idx++) {
            auto section = sectioned_results[section_idx];
            REQUIRE(exp_keys[section_idx] == section.key());
            for (size_t element_idx = 0; element_idx < section.size(); element_idx++) {
                auto element = sectioned_results[section_idx][element_idx];
                Mixed value = T(exp_values_sorted[results_idx]);
                REQUIRE(element == value);
                results_idx++;
            }
        }
        REQUIRE(algo_run_count == (int)exp_values.size());
    }
}
