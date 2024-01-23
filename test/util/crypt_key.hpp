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

#ifndef REALM_TEST_UTIL_CRYPT_KEY_HPP
#define REALM_TEST_UTIL_CRYPT_KEY_HPP

#include <realm/util/file.hpp>

namespace realm {
namespace test_util {

/// Returns a non-null encryption key if encryption is enabled at compile-time
/// (REALM_ENABLE_ENCRYPTION), and either \a always is true or global mode
/// "always encrypt" is enabled. Otherwise it returns none. The global mode
/// "always encrypt" can be enabled by calling always_encrypt(), but pay
/// attention to the rules governing its use.
///
/// This function is thread-safe as long as there are no concurrent invocations
/// of always_encrypt().
std::optional<util::File::EncryptionKeyType> crypt_key(const char* raw_value, bool always = false);

inline std::optional<util::File::EncryptionKeyType> crypt_key(bool always = false)
{
    return crypt_key("1234567890123456789012345678901123456789012345678901234567890123", always);
}

/// Returns true if global mode "always encrypt" is enabled.
///
/// This function is thread-safe as long as there are no concurrent invocations
/// of always_encrypt().
bool is_always_encrypt_enabled();

/// Enable global mode "always encrypt".
///
/// This function is **not** thread-safe. If you call it, be sure to call it
/// prior to any invocation of crypt_key().
void enable_always_encrypt();
} // namespace test_util
} // namespace realm

#endif // REALM_TEST_UTIL_CRYPT_KEY_HPP
