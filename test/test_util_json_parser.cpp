#include "test.hpp"

#include <realm/util/json_parser.hpp>
#include <realm/util/bson/bson.hpp>
#include <realm/mixed.hpp>
#include <iostream>
#include <chrono>

// #include <valgrind/callgrind.h>

#ifndef CALLGRIND_START_INSTRUMENTATION
#define CALLGRIND_START_INSTRUMENTATION
#endif

#ifndef CALLGRIND_STOP_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif

// valgrind --tool=callgrind --instr-atstart=no realm-tests

using namespace realm;
using namespace realm::util;

static const char g_basic_object[] = "{\n"
                                     "    \"access\": [\"download\", \"upload\"],\n"
                                     "    \"timestamp\": 1455530614,\n"
                                     "    \"expires\": null,\n"
                                     "    \"app_id\": \"io.realm.Test\"\n"
                                     "}";

using ET = JSONParser::EventType;

namespace {
static const char g_events_test[] =
    "   {\"a\":\"b\",\t\"b\"    :[],\"c\": {\"d\":null,\"e\":123.13,\"f\": -199,\"g\":-2.3e9},\"h\":\"\\u00f8\"}";
static const JSONParser::EventType g_expected_events[] = {
    ET::object_begin, ET::string,       ET::string,       ET::string,         ET::array_begin,
    ET::array_end,    ET::string,       ET::object_begin, ET::string,         ET::null,
    ET::string,       ET::number_float, ET::string,       ET::number_integer, ET::string,
    ET::number_float, ET::object_end,   ET::string,       ET::string,         ET::object_end};
} // anonymous namespace


TEST(JSONParser_Basic)
{
    enum {
        initial,
        in_object,
        get_access,
        access_elements,
        get_timestamp,
        get_expires,
        get_app_id,
    } state = initial;
    std::vector<char> buffer;
    util::Optional<int64_t> timestamp;
    util::Optional<double> expires;
    util::Optional<std::string> app_id;
    std::vector<std::string> access;

    auto read_string_into_buffer = [&](const JSONParser::Event& event) -> StringData {
        CHECK(event.type == JSONParser::EventType::string);
        buffer = event.unescape_string();
        return StringData(buffer.data(), buffer.size());
    };

    JSONParser parser{[&](auto& event) -> std::error_condition {
        switch (state) {
            case initial: {
                if (event.type == JSONParser::EventType::object_begin)
                    state = in_object;
                else
                    return JSONParser::Error::unexpected_token;
                break;
            }
            case in_object: {
                if (event.type == JSONParser::EventType::string) {
                    StringData key = read_string_into_buffer(event);
                    if (key == "access")
                        state = get_access;
                    else if (key == "timestamp")
                        state = get_timestamp;
                    else if (key == "expires")
                        state = get_expires;
                    else if (key == "app_id")
                        state = get_app_id;
                    else {
                        throw std::runtime_error("Unknown key");
                    }
                    break;
                }
                else if (event.type == JSONParser::EventType::object_end) {
                    break;
                }
                else
                    return JSONParser::Error::unexpected_token;
            }
            case get_access: {
                if (event.type == JSONParser::EventType::array_begin)
                    state = access_elements;
                else
                    return JSONParser::Error::unexpected_token;
                break;
            }
            case access_elements: {
                if (event.type == JSONParser::EventType::array_end)
                    state = in_object;
                else if (event.type == JSONParser::EventType::string)
                    access.push_back(read_string_into_buffer(event));
                else
                    return JSONParser::Error::unexpected_token;
                break;
            }
            case get_timestamp: {
                if (event.type == JSONParser::EventType::number_integer) {
                    timestamp = event.integer;
                    state = in_object;
                    break;
                }
                return JSONParser::Error::unexpected_token;
            }
            case get_expires: {
                if (event.type == JSONParser::EventType::null) {
                    state = in_object;
                    break;
                }
                return JSONParser::Error::unexpected_token;
            }
            case get_app_id: {
                if (event.type == JSONParser::EventType::string) {
                    app_id = std::string(read_string_into_buffer(event));
                    state = in_object;
                    break;
                }
                return JSONParser::Error::unexpected_token;
            }
        }
        return std::error_condition{};
    }};
    auto ec = parser.parse(g_basic_object);
    CHECK(!ec);
    CHECK_EQUAL(state, in_object);
    CHECK_EQUAL(*timestamp, 1455530614);
    CHECK(!expires);
    CHECK_EQUAL(*app_id, "io.realm.Test");
    CHECK_EQUAL(access.size(), 2);
    CHECK_EQUAL(access[0], "download");
    CHECK_EQUAL(access[1], "upload");
}

TEST(JSONParser_UnescapeString)
{
    JSONParser::Event event(JSONParser::EventType::string);
    event.range = "\"Hello,\\\\ World.\\n8\\u00b0C\\u00F8\""; // includes surrounding double quotes
    std::vector<char> buffer = event.unescape_string();
    CHECK_EQUAL(StringData(buffer.data(), buffer.size()), "Hello,\\ World.\n8°Cø");

    static const char* escaped[] = {
        "\"\\u0abg\"",        // invalid sequence
        "\"\\u0041\"",        // ASCII 'A'
        "\"\\u05d0\"",        // Hebrew 'alef'
        "\"\\u2f08\"",        // Kangxi (Chinese) 'man'
        "\"\\u4eba\"",        // CJK Unified Ideograph 'man'
        "\"\\ufffd\"",        // Replacement character
        "\"\\ud87e\\udd10\"", // Emoji 'zipper-mouth face' (surrogate pair)
    };
    static const char* expected[] = {
        "\\u0abg", "A",    "א", "⼈",
        "人", // NOTE! This character looks identical to the one above, but is a different codepoint.
        "�",       "🤐",
    };

    for (size_t i = 0; i != sizeof(escaped) / sizeof(escaped[0]); ++i) {
        event.range = escaped[i];
        auto buf = event.unescape_string();
        CHECK_EQUAL(StringData(buf.data(), buf.size()), expected[i]);
    }

    static const char* invalid_surrogate_pairs[] = {
        "\"\\ud800a\"", // high surrogate followed by non-surrogate
        "\"\\udc00\"",  // low surrogate with no preceding high surrogate
    };

    for (size_t i = 0; i < sizeof(invalid_surrogate_pairs) / sizeof(invalid_surrogate_pairs[0]); ++i) {
        const char* str = invalid_surrogate_pairs[i];
        event.range = str;
        auto buf = event.unescape_string();
        CHECK_EQUAL(StringData(buf.data(), buf.size()), StringData(str + 1, strlen(str) - 2));
    }
}

TEST(JSONParser_Events)
{
    size_t i = 0;
    JSONParser([&](auto&& event) noexcept {
        if (event.type != g_expected_events[i]) {
            CHECK(event.type == g_expected_events[i]);
            std::cerr << "Event did not match: " << event << " (at " << i << ")\n";
        }
        ++i;
        return std::error_condition{};
    }).parse(g_events_test);
    CHECK_EQUAL(i, sizeof(g_expected_events) / sizeof(g_expected_events[0]));
}

TEST(JSONParser_PropagateError)
{
    auto ec = JSONParser([&](auto&& event) noexcept {
                  if (event.type == ET::null) {
                      return std::error_condition{std::errc::argument_out_of_domain}; // just anything
                  }
                  return std::error_condition{};
              }).parse(g_events_test);
    CHECK(ec);
    CHECK(ec == std::errc::argument_out_of_domain);
}

TEST(JSONParser_Whitespace)
{
    JSONParser invalid_whitespace_parser([](auto&&) noexcept {
        return std::error_condition{};
    });
    std::error_condition ec;

    static const char initial_whitespace[] = "  \t{}";
    ec = invalid_whitespace_parser.parse(initial_whitespace);
    CHECK(!ec);

    // std::isspace considers \f and \v whitespace, but the JSON standard doesn't.
    static const char invalid_whitespace_f[] = "{\"a\":\f1}";
    ec = invalid_whitespace_parser.parse(invalid_whitespace_f);
    CHECK(ec == JSONParser::Error::unexpected_token);

    static const char invalid_whitespace_v[] = "{\"a\":\v2}";
    ec = invalid_whitespace_parser.parse(invalid_whitespace_v);
    CHECK(ec == JSONParser::Error::unexpected_token);
}

TEST(JSONParser_PrimitiveDocuments)
{
    // JSON specifies that any object can be the document root.

    std::error_condition ec;

    static const char int_root[] = "123";
    ec = JSONParser([&](auto&& event) noexcept {
             CHECK_EQUAL(event.type, JSONParser::EventType::number_integer);
             CHECK_EQUAL(event.integer, 123);
             return std::error_condition{};
         }).parse(int_root);
    CHECK(!ec);

    static const char number_root[] = "123.0";
    ec = JSONParser([&](auto&& event) noexcept {
             CHECK_EQUAL(event.type, JSONParser::EventType::number_float);
             CHECK_EQUAL(event.number, 123.0);
             return std::error_condition{};
         }).parse(number_root);
    CHECK(!ec);

    static const char string_root[] = "\"\\u00f8\"";
    ec = JSONParser([&](auto&& event) noexcept {
             CHECK_EQUAL(event.type, JSONParser::EventType::string);
             auto buffer = event.unescape_string();
             CHECK_EQUAL(StringData(buffer.data(), buffer.size()), "ø");
             return std::error_condition{};
         }).parse(string_root);
    CHECK(!ec);

    static const char bool_root[] = "false";
    ec = JSONParser([&](auto&& event) noexcept {
             CHECK_EQUAL(event.type, JSONParser::EventType::boolean);
             CHECK(!event.boolean);
             return std::error_condition{};
         }).parse(bool_root);
    CHECK(!ec);

    static const char null_root[] = "null";
    ec = JSONParser([&](auto&& event) noexcept {
             CHECK_EQUAL(event.type, JSONParser::EventType::null);
             return std::error_condition{};
         }).parse(null_root);
    CHECK(!ec);

    static const char invalid_root[] = "blah";
    ec = JSONParser([&](auto&&) noexcept {
             return std::error_condition{};
         }).parse(invalid_root);
    CHECK(ec == JSONParser::Error::unexpected_token);
}

TEST(JSONParser_ArrayDocument)
{
    std::error_condition ec;

    static const char array_root[] = "[]";
    ec = JSONParser([](auto&&) noexcept {
             return std::error_condition{};
         }).parse(array_root);
    CHECK(!ec);

    static const char invalid_array_root[] = "[";
    ec = JSONParser([](auto&&) noexcept {
             return std::error_condition{};
         }).parse(invalid_array_root);
    CHECK(ec == JSONParser::Error::unexpected_end_of_stream);
}

TEST(JSONParser_StringTermination)
{
    static const char string_root[] = "\"\\\\\\\"\"";
    auto ec = JSONParser([&](auto&& event) noexcept {
                  CHECK_EQUAL(event.type, JSONParser::EventType::string);
                  CHECK_EQUAL(event.escaped_string_value(), "\\\\\\\"");
                  return std::error_condition{};
              }).parse(string_root);
    CHECK(!ec);
}
